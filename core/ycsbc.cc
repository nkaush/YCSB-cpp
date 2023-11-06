//
//  ycsbc.cc
//  YCSB-cpp
//
//  Copyright (c) 2020 Youngjae Lee <ls4154.lee@gmail.com>.
//  Copyright (c) 2014 Jinglei Ren <jinglei@ren.systems>.
//

#include <cstring>
#include <ctime>

#include <string>
#include <iostream>
#include <vector>
#include <thread>
#include <future>
#include <chrono>
#include <iomanip>

#include "utils.h"
#include "timer.h"
#include "client.h"
#include "measurements.h"
#include "core_workload.h"
#include "countdown_latch.h"
#include "db_factory.h"
#include "workload_factory.h"
#include "pure_insert_workload.h"

const std::string YAML_NAME_PROPERTY = "yamlname";

void UsageMessage(const char *command);
bool StrStartWith(const char *str, const char *pre);
void ParseCommandLine(int argc, const char *argv[], ycsbc::utils::Properties &props);
void SaveRunSummary(YAML::Node &node, ycsbc::utils::Properties &props, std::time_t &now);

void StatusThread(ycsbc::Measurements *measurements, CountDownLatch *latch, CountDownLatch *init_latch, int interval, int interval_us, const std::string &tracefilename) {
  using namespace std::chrono;
  init_latch->Await(); // wait for all client threads to finish initializing before start printing status
  time_point<system_clock> start = system_clock::now();
  bool done = false;

  std::ostream *os = tracefilename.empty() ? &std::cout : new std::ofstream(tracefilename);

  measurements->Start();
  while (1) {
    time_point<system_clock> now = system_clock::now();
    std::time_t now_c = system_clock::to_time_t(now);
    duration<double> elapsed_time = now - start;

    *os << std::put_time(std::localtime(&now_c), "%F %T") << ' '
              << static_cast<long long>(elapsed_time.count()) << " sec: ";

    *os << measurements->GetStatusMsg() << std::endl;

    if (done) {
      break;
    }
    if (interval > 0)
      done = latch->AwaitFor(interval);
    else if (interval_us > 0)
      done = latch->AwaitForUs(interval_us);
  };

  if (!tracefilename.empty()) {
    dynamic_cast<std::ofstream *>(os)->close();
    delete os;
  }
}

int main(const int argc, const char *argv[]) {
  using namespace std::chrono;
  ycsbc::utils::Properties props;
  ParseCommandLine(argc, argv, props);

  const bool do_load = (props.GetProperty("doload", "false") == "true");
  const bool do_transaction = (props.GetProperty("dotransaction", "false") == "true");
  if (!do_load && !do_transaction) {
    std::cerr << "No operation to do" << std::endl;
    exit(1);
  }

  const int num_threads = stoi(props.GetProperty("threadcount", "1"));

  ycsbc::Measurements *measurements = ycsbc::CreateMeasurements(&props);
  if (measurements == nullptr) {
    std::cerr << "Unknown measurements name" << std::endl;
    exit(1);
  }

  std::vector<ycsbc::DB *> dbs;
  for (int i = 0; i < num_threads; i++) {
    ycsbc::DB *db = ycsbc::DBFactory::CreateDB(&props, measurements);
    if (db == nullptr) {
      std::cerr << "Unknown database name " << props["dbname"] << std::endl;
      exit(1);
    }
    dbs.push_back(db);
  }

  ycsbc::CoreWorkload *wl = ycsbc::WorkloadFactory::CreateWorkload(props);
  wl->Init(props);

  const bool show_status = (props.GetProperty("status", "false") == "true");
  const int status_interval = std::stoi(props.GetProperty("status.interval", "10"));
  const int status_interval_us = std::stoi(props.GetProperty("status.intervalus", "1000"));
  const std::string status_trace = props.GetProperty("status.trace", "");
  const int64_t sec_skip = stol(props.GetProperty(SKIP_SECOND_PROPERTY, "0"));

  // load phase
  if (do_load) {
    const int total_ops = stoi(props[ycsbc::CoreWorkload::RECORD_COUNT_PROPERTY]);

    CountDownLatch latch(num_threads), init_latch(num_threads);
    ycsbc::utils::Timer<double> timer;

    std::future<void> status_future;
    if (show_status) {
      status_future = std::async(std::launch::async, StatusThread,
                                 measurements, &latch, &init_latch, status_interval, status_interval_us, status_trace);
    }
    std::vector<std::future<int>> client_threads;
    for (int i = 0; i < num_threads; ++i) {
      int thread_ops = total_ops / num_threads;
      if (i < total_ops % num_threads) {
        thread_ops++;
      }
      client_threads.emplace_back(std::async(std::launch::async, ycsbc::ClientThread, dbs[i], wl, props, thread_ops, i,
                                             num_threads, true, true, !do_transaction || dbs[i]->ReInitBeforeTransaction(), &latch, &init_latch, sec_skip));
    }
    assert((int)client_threads.size() == num_threads);
    init_latch.Await();
    timer.Start();

    int sum = 0;
    for (auto &n : client_threads) {
      assert(n.valid());
      sum += n.get();
    }
    double runtime = timer.End() - sec_skip;

    if (show_status) {
      status_future.wait();
    }

    std::cout << "Load runtime(sec): " << runtime << std::endl;
    std::cout << "Load operations(ops): " << sum << std::endl;
    std::cout << "Load throughput(ops/sec): " << sum / runtime << std::endl;

    YAML::Node load_summary;
    std::time_t now_c = system_clock::to_time_t(system_clock::now());
    std::stringstream tstmp_s;
    tstmp_s << std::put_time(std::localtime(&now_c), "%F %T");
    load_summary["dbname"] = props["dbname"];
    load_summary["timestamp"] = tstmp_s.str();
    load_summary["recordcount"] = props.GetProperty(ycsbc::CoreWorkload::RECORD_COUNT_PROPERTY);
    load_summary["operationcount"] = props.GetProperty(ycsbc::CoreWorkload::OPERATION_COUNT_PROPERTY);
    load_summary["runtime"] = runtime;
    load_summary["operations"] = sum;
    load_summary["throughput"] = sum / runtime;
    load_summary["workload"] = props.GetProperty(ycsbc::WorkloadFactory::WORKLOAD_NAME_PROPERTY,
                                                ycsbc::WorkloadFactory::WORKLOAD_NAME_DEFAULT);
    measurements->Emit(load_summary);
    SaveRunSummary(load_summary, props, now_c, do_load);
  }
  // Output 'LOAD PHASE DONE' to logfile
  measurements->Reset();
  std::this_thread::sleep_for(std::chrono::seconds(stoi(props.GetProperty("sleepafterload", "0"))));

  // transaction phase
  if (do_transaction) {
    const int total_ops = stoi(props[ycsbc::CoreWorkload::OPERATION_COUNT_PROPERTY]);

    CountDownLatch latch(num_threads), init_latch(num_threads);
    ycsbc::utils::Timer<double> timer;

    std::future<void> status_future;
    if (show_status) {
      status_future = std::async(std::launch::async, StatusThread,
                                 measurements, &latch, &init_latch, status_interval, status_interval_us, status_trace);
    }
    std::vector<std::future<int>> client_threads;
    for (int i = 0; i < num_threads; ++i) {
      int thread_ops = total_ops / num_threads;
      if (i < total_ops % num_threads) {
        thread_ops++;
      }
      client_threads.emplace_back(std::async(std::launch::async, ycsbc::ClientThread, dbs[i], wl, props, thread_ops, i,
                                             num_threads, false, !do_load || dbs[i]->ReInitBeforeTransaction(), true, &latch, &init_latch, sec_skip));
    }
    assert((int)client_threads.size() == num_threads);
    init_latch.Await();
    timer.Start();

    int sum = 0;
    for (auto &n : client_threads) {
      assert(n.valid());
      sum += n.get();
    }
    double runtime = timer.End() - sec_skip;

    if (show_status) {
      status_future.wait();
    }

    std::cout << "Run runtime(sec): " << runtime << std::endl;
    std::cout << "Run operations(ops): " << sum << std::endl;
    std::cout << "Run throughput(ops/sec): " << sum / runtime << std::endl;

    YAML::Node run_summary;
    std::time_t now_c = system_clock::to_time_t(system_clock::now());
    std::stringstream tstmp_s;
    tstmp_s << std::put_time(std::localtime(&now_c), "%F %T");
    run_summary["dbname"] = props["dbname"];
    run_summary["timestamp"] = tstmp_s.str();
    run_summary["recordcount"] = props.GetProperty(ycsbc::CoreWorkload::RECORD_COUNT_PROPERTY);
    run_summary["operationcount"] = props.GetProperty(ycsbc::CoreWorkload::OPERATION_COUNT_PROPERTY);
    run_summary["runtime"] = runtime;
    run_summary["operations"] = sum;
    run_summary["throughput"] = sum / runtime;
    run_summary["workload"] = props.GetProperty(ycsbc::WorkloadFactory::WORKLOAD_NAME_PROPERTY,
                                                ycsbc::WorkloadFactory::WORKLOAD_NAME_DEFAULT);
    measurements->Emit(run_summary);
    SaveRunSummary(run_summary, props, now_c, false);
  }

  delete wl;

  for (int i = 0; i < num_threads; i++) {
    delete dbs[i];
  }
}

void ParseCommandLine(int argc, const char *argv[], ycsbc::utils::Properties &props) {
  int argindex = 1;
  while (argindex < argc && StrStartWith(argv[argindex], "-")) {
    if (strcmp(argv[argindex], "-load") == 0) {
      props.SetProperty("doload", "true");
      argindex++;
    } else if (strcmp(argv[argindex], "-run") == 0 || strcmp(argv[argindex], "-t") == 0) {
      props.SetProperty("dotransaction", "true");
      argindex++;
    } else if (strcmp(argv[argindex], "-threads") == 0) {
      argindex++;
      if (argindex >= argc) {
        UsageMessage(argv[0]);
        std::cerr << "Missing argument value for -threads" << std::endl;
        exit(0);
      }
      props.SetProperty("threadcount", argv[argindex]);
      argindex++;
    } else if (strcmp(argv[argindex], "-db") == 0) {
      argindex++;
      if (argindex >= argc) {
        UsageMessage(argv[0]);
        std::cerr << "Missing argument value for -db" << std::endl;
        exit(0);
      }
      props.SetProperty("dbname", argv[argindex]);
      argindex++;
    } else if (strcmp(argv[argindex], "-P") == 0) {
      argindex++;
      if (argindex >= argc) {
        UsageMessage(argv[0]);
        std::cerr << "Missing argument value for -P" << std::endl;
        exit(0);
      }
      std::string filename(argv[argindex]);
      std::ifstream input(argv[argindex]);
      try {
        props.Load(input);
      } catch (const std::string &message) {
        std::cerr << message << std::endl;
        exit(0);
      }
      input.close();
      argindex++;
    } else if (strcmp(argv[argindex], "-p") == 0) {
      argindex++;
      if (argindex >= argc) {
        UsageMessage(argv[0]);
        std::cerr << "Missing argument value for -p" << std::endl;
        exit(0);
      }
      std::string prop(argv[argindex]);
      size_t eq = prop.find('=');
      if (eq == std::string::npos) {
        std::cerr << "Argument '-p' expected to be in key=value format "
                     "(e.g., -p operationcount=99999)" << std::endl;
        exit(0);
      }
      props.SetProperty(ycsbc::utils::Trim(prop.substr(0, eq)),
                        ycsbc::utils::Trim(prop.substr(eq + 1)));
      argindex++;
    } else if (strcmp(argv[argindex], "-s") == 0) {
      props.SetProperty("status", "true");
      argindex++;
    } else {
      UsageMessage(argv[0]);
      std::cerr << "Unknown option '" << argv[argindex] << "'" << std::endl;
      exit(0);
    }
  }

  if (argindex == 1 || argindex != argc) {
    UsageMessage(argv[0]);
    exit(0);
  }
}

void UsageMessage(const char *command) {
  std::cout <<
      "Usage: " << command << " [options]\n"
      "Options:\n"
      "  -load: run the loading phase of the workload\n"
      "  -t: run the transactions phase of the workload\n"
      "  -run: same as -t\n"
      "  -threads n: execute using n threads (default: 1)\n"
      "  -db dbname: specify the name of the DB to use (default: basic)\n"
      "  -P propertyfile: load properties from the given file. Multiple files can\n"
      "                   be specified, and will be processed in the order specified\n"
      "  -p name=value: specify a property to be passed to the DB and workloads\n"
      "                 multiple properties can be specified, and override any\n"
      "                 values in the propertyfile\n"
      "  -s: print status every 10 seconds (use status.interval prop to override)"
      << std::endl;
}

void SaveRunSummary(YAML::Node &node, ycsbc::utils::Properties &props, std::time_t &now, const bool is_load) { // ADD A FLAG HERE FOR IS_LOAD OR IS_RUN PHASE COMPLETED
  std::string yaml_name = props.GetProperty(YAML_NAME_PROPERTY);
  std::string load_phase_str = "load_phase";
  std::string run_phase_str = "run_phase";

  if (yaml_name.empty()) {
    std::stringstream name_s;
    name_s << std::put_time(std::localtime(&now), "%Y%m%d%s");
    yaml_name.append(name_s.str());
  }
  if is_load {
    load_phase_str.append(yaml_name);
  } else {
    run_phase_str.append(yaml_name);
  }
  yaml_name.append(".yml");
  yaml_name = load_phase_str;
  std::ofstream yaml_file;
  yaml_file.open(yaml_name);
  yaml_file << node << std::endl;
  yaml_file.close();
}

inline bool StrStartWith(const char *str, const char *pre) {
  return strncmp(str, pre, strlen(pre)) == 0;
}

