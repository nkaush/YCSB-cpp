#include "tracegen.h"
#include "core/db_factory.h"

#include <atomic>
#include <filesystem>

using std::endl;

namespace ycsbc {

static std::atomic_int thread_counter = 0;

void Tracegen::Init() {
  std::cout << "here" << std::endl;
  std::filesystem::path dirpath = props_->GetProperty("dumpdir", "dump");
  int index = thread_counter.fetch_add(1);
  if (index == 0) {
    try {
      if (!std::filesystem::create_directory(dirpath)) {
        std::cerr << "Failed to create directory at " << dirpath << "\n";
      }
    } catch(const std::exception& e) {
        std::cerr << "Failed to create directory " << dirpath << ": " << e.what() << '\n';
    }
  }
  
  f_ = std::ofstream(dirpath / ("thread-" + std::to_string(index)));
}

DB::Status Tracegen::Read(const std::string &table, const std::string &key,
                         const std::vector<std::string> *fields, std::vector<Field> &result) {
  f_ << "R " << key << "\n";
  return kOK;
}

DB::Status Tracegen::Scan(const std::string &table, const std::string &key, int len,
                         const std::vector<std::string> *fields,
                         std::vector<std::vector<Field>> &result) {
  f_ << "S " << key << " " << len << "\n";
  return kOK;
}

DB::Status Tracegen::Update(const std::string &table, const std::string &key,
                           std::vector<Field> &values) {
  f_ << "U " << key << "\n";
  return kOK;
}

DB::Status Tracegen::Insert(const std::string &table, const std::string &key,
                           std::vector<Field> &values) {
  f_ << "I " << key << "\n";
  return kOK;
}

DB::Status Tracegen::Delete(const std::string &table, const std::string &key) {
  f_ << "D " << key << "\n";
  return kOK;
}

DB *NewTracegenDB() {
  return new Tracegen;
}

const bool registered = DBFactory::RegisterDB("tracegen", NewTracegenDB);

} // ycsbc
