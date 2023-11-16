#include "replicated_splinterdb.h"

#include "core/db_factory.h"

using read_algorithm = replicated_splinterdb::read_policy::algorithm;

const std::string PROP_HOST = "replicated_splinterdb.host";
const std::string PROP_HOST_DEFAULT = "localhost";

const std::string PROP_PORT = "replicated_splinterdb.port";
const std::string PROP_PORT_DEFAULT = "10003";

const std::string PROP_READ_POLICY = "replicated_splinterdb.read_policy";
const std::string PROP_READ_POLICY_DEFAULT = "round_robin";
const std::map<std::string, read_algorithm> READ_ALGORITHMS = {
    {"hash", read_algorithm::hash},
    {"round_robin", read_algorithm::round_robin},
    {"random_token", read_algorithm::random_token},
    {"random_uniform", read_algorithm::random_uniform}
};

namespace ycsbc {

void ReplicatedSplinterDB::Init() {
    const utils::Properties &props = *props_;
    std::string host = props.GetProperty(PROP_HOST, PROP_HOST_DEFAULT);
    std::string port_str = props.GetProperty(PROP_PORT, PROP_PORT_DEFAULT);
    std::string read_algo = 
        props.GetProperty(PROP_READ_POLICY, PROP_READ_POLICY_DEFAULT);

    auto rp = READ_ALGORITHMS.find(read_algo);
    if (rp == READ_ALGORITHMS.end()) {
        throw std::runtime_error("Invalid replicated_splinterdb.read_policy");
    }

    std::cout << "Connecting to SplinterDB cluster at " << host << ":" 
              << port_str << std::endl;

    uint16_t port = static_cast<uint16_t>(std::stoi(port_str));
    client_ = std::make_shared<replicated_splinterdb::client>(host, port, rp->second);
}

DB::Status ReplicatedSplinterDB::Read(const std::string &table,
                                      const std::string &key,
                                      const std::vector<std::string> *fields,
                                      std::vector<Field> &result) {
    auto [value, spl_rc] = client_->get(key);

    if (!value.empty()) {
        std::string field(value.begin(), value.end());
        result.emplace_back("", field);
    }

    return DB::kOK;
}

DB::Status ReplicatedSplinterDB::Insert(const std::string &table,
                                        const std::string &key,
                                        std::vector<DB::Field> &values) {
    if (values.size() != 1) {
        throw std::runtime_error("Insert: only one field supported!");
    }

    auto& [field, value] = values[0];
    auto [spl_rc, raft_rc, msg] = client_->put(key, value);

    if (raft_rc != 0) {
        std::cerr << "Replication error: " << msg << " (rc=" << raft_rc 
                    << ")" << std::endl;
        return DB::kError;
    }

    if (spl_rc != 0) {
        std::cerr << "SplinterDB error (rc=" << spl_rc << ")" << std::endl;
        return DB::kError;
    }

    return DB::kOK;
}

DB::Status ReplicatedSplinterDB::Delete(const std::string &table,
                                        const std::string &key) {
    std::vector<uint8_t> key_vec(key.begin(), key.end());
    auto [spl_rc, raft_rc, msg] = client_->del(key);

    if (raft_rc != 0) {
        std::cerr << "Replication error: " << msg << " (rc=" << raft_rc 
                    << ")" << std::endl;
        return DB::kError;
    }

    if (spl_rc != 0) {
        std::cerr << "SplinterDB error (rc=" << spl_rc << ")" << std::endl;
        return DB::kError;
    }

    return DB::kOK;
}

void ReplicatedSplinterDB::PostLoadCallback() {
    client_->trigger_cache_dumps("cachedump-load");
    client_->trigger_cache_clear();
}

DB *NewReplicatedSplinterDB() {
    return new ReplicatedSplinterDB;
}

const bool registered = DBFactory::RegisterDB("replicated-splinterdb", NewReplicatedSplinterDB);

} // ycsbc