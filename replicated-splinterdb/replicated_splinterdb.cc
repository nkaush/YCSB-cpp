#include "replicated_splinterdb.h"

#include "core/db_factory.h"

#define FIELD_DELIMITER '='

const std::string PROP_HOST = "replicated_splinterdb.host";
const std::string PROP_HOST_DEFAULT = "localhost";

const std::string PROP_PORT = "replicated_splinterdb.port";
const std::string PROP_PORT_DEFAULT = "10003";

namespace ycsbc {

void ReplicatedSplinterDB::Init() {
    const utils::Properties &props = *props_;
    std::string host = props.GetProperty(PROP_HOST, PROP_HOST_DEFAULT);
    std::string port_str = props.GetProperty(PROP_PORT, PROP_PORT_DEFAULT);
    std::cout << "Connecting to SplinterDB at " << host << ":" << port_str
                << std::endl;
    int unchecked_port = std::stoi(port_str);
    if (1 > unchecked_port || unchecked_port > 65535) {
        std::string msg = "invalid port number for host \"" + host +
                            "\": " + std::to_string(unchecked_port);
        throw std::runtime_error(msg);
    }
    uint16_t port = static_cast<uint16_t>(unchecked_port);
    client_ = std::make_shared<replicated_splinterdb::client>(host, port);
}

DB::Status ReplicatedSplinterDB::Read(const std::string &table,
                                      const std::string &key,
                                      const std::vector<std::string> *fields,
                                      std::vector<Field> &result) {
    std::vector<uint8_t> key_vec(key.begin(), key.end());
    auto [value, spl_rc] = client_->get(key_vec);

    if (spl_rc != 0) {
        std::cerr << "SplinterDB error (rc=" << spl_rc << ")" << std::endl;
        return DB::kError;
    }

    if (value.empty()) {
        return DB::kError;
    }

    auto it = std::find(value.begin(), value.end(), FIELD_DELIMITER);
    if (it == value.end()) {
        return DB::kError;
    }

    std::string field(value.begin(), it);
    std::string val(it + 1, value.end());
    result.emplace_back(field, val);

    return DB::kOK;
}

DB::Status ReplicatedSplinterDB::Insert(const std::string &table,
                                        const std::string &key,
                                        std::vector<DB::Field> &values) {
    std::vector<uint8_t> key_vec(key.begin(), key.end());
    if (values.size() != 1) {
        throw std::runtime_error("Insert: only one field supported!");
    }

    std::vector<uint8_t> value_vec;
    auto& [field, value] = values[0];
    value_vec.insert(value_vec.end(), field.begin(), field.end());
    value_vec.push_back(FIELD_DELIMITER);
    value_vec.insert(value_vec.end(), value.begin(), value.end());

    auto [spl_rc, raft_rc, msg] = client_->put(key_vec, value_vec);

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
    auto [spl_rc, raft_rc, msg] = client_->del(key_vec);

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

DB *NewReplicatedSplinterDB() {
  return new ReplicatedSplinterDB;
}

const bool registered = DBFactory::RegisterDB("replicated-splinterdb", NewReplicatedSplinterDB);


} // ycsbc