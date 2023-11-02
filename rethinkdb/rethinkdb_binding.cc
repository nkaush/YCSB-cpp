#include "rethinkdb_binding.h"

#include <memory>
#include <cstdio>

#include "core/db_factory.h"

const std::string PROP_HOST = "rethinkdb.host";
const std::string PROP_HOST_DEFAULT = "localhost";

const std::string PROP_PORT = "rethinkdb.port";
const std::string PROP_PORT_DEFAULT = "28015";

const std::string DURABILITY_OPT_ARG = "durability";
const std::string PROP_DURABILITY = "rethinkdb.durability";
const std::string PROP_DURABILITY_DEFAULT = "soft";

const std::string READ_MODE_OPT_ARG = "read_mode";
const std::string PROP_READ_MODE = "rethinkdb.read_mode";
const std::string PROP_READ_MODE_DEFAULT = "outdated";

namespace ycsbc {

void RethinkDBBinding::Init() {
    const utils::Properties &props = *props_;

    std::string durability = props.GetProperty(PROP_DURABILITY, PROP_DURABILITY_DEFAULT);
    std::string read_mode = props.GetProperty(PROP_READ_MODE, PROP_READ_MODE_DEFAULT);

    if (durability != "hard" && durability != "soft") {
        throw std::runtime_error("rethinkdb.durability must be one of [ hard | soft ]");
    }

    if (read_mode != "single" && read_mode != "majority" && read_mode != "outdated") {
        throw std::runtime_error("rethinkdb.read_mode must be one of [ single | majority | outdated ]");
    }

    durability_ = R::Term(durability);
    read_mode_ = R::Term(read_mode);

    std::string host = props.GetProperty(PROP_HOST, PROP_HOST_DEFAULT);
    std::string port_str = props.GetProperty(PROP_PORT, PROP_PORT_DEFAULT);
    std::cout << "Connecting to RethinkDB cluster at " << host << ":" 
              << port_str << std::endl;

    int port = std::stoi(port_str);
    if (1 > port || port > 65535) {
        std::string msg = "invalid port number for host \"" + host +
                            "\": " + std::to_string(port);
        throw std::runtime_error(msg);
    }

    try {
        conn_ = R::connect(host, port);
    } catch (std::exception& e) {
        std::cout << e.what() << std::endl;
        exit(1);
    }
}

DB::Status RethinkDBBinding::Read(const std::string &table,
                                  const std::string &key,
                                  const std::vector<std::string> *fields,
                                  std::vector<Field> &result) {
    R::Cursor cursor = R::table(table, {{READ_MODE_OPT_ARG, read_mode_}})
        .get(key)
        .run(*conn_);

    if (!cursor.is_single()) {
        throw std::runtime_error("Expected a single document");
    }

    R::Datum document = std::move(cursor).to_datum();

    if (fields == nullptr) {
        for (auto& [f, v] : document.extract_object()) {
            result.emplace_back(f, v.extract_string());
        }
    } else {
        for (const std::string& f : *fields) {
            result.emplace_back(f, document.extract_field(f).extract_string());
        }
    }

    return DB::kOK;
}

DB::Status RethinkDBBinding::Update(const std::string &table,
                                    const std::string &key,
                                    std::vector<DB::Field> &values) {
    std::map<std::string, R::Datum> to_update;
    for (auto& [f, v] : values) {
        to_update.emplace(f, R::Datum(v));
    }

    R::Datum result = R::table(table, {{READ_MODE_OPT_ARG, read_mode_}})
        .get(key)
        .update(to_update, {{DURABILITY_OPT_ARG, durability_}})
        .run(*conn_)
        .to_datum();

    auto inserted = result.extract_field("inserted").extract_number() == 0.0;
    auto deleted = result.extract_field("deleted").extract_number() == 0.0;
    auto errors = result.extract_field("errors").extract_number() == 0.0;

    return inserted && deleted && errors ? DB::kOK : DB::kError;
}

DB::Status RethinkDBBinding::Insert(const std::string &table,
                                    const std::string &key,
                                    std::vector<DB::Field> &values) {
    std::map<std::string, R::Datum> to_insert = {{"id", R::Datum(key)}};
    for (auto& [f, v] : values) {
        to_insert.emplace(f, R::Datum(v));
    }

    R::Datum result = R::table(table)
        .insert(to_insert, {{DURABILITY_OPT_ARG, durability_}})
        .run(*conn_)
        .to_datum();
    
    auto inserted = result.extract_field("inserted").extract_number();
    if (static_cast<int>(inserted) != 1) {
        return DB::kError;
    }

    return DB::kOK;
}

DB::Status RethinkDBBinding::Delete(const std::string &table,
                                        const std::string &key) {
    R::Datum result = R::table(table, {{READ_MODE_OPT_ARG, read_mode_}})
        .get(key)
        .delete_({{DURABILITY_OPT_ARG, durability_}})
        .run(*conn_)
        .to_datum();
    
    auto deleted = result.extract_field("deleted").extract_number();
    if (static_cast<int>(deleted) != 1) {
        return DB::kError;
    }

    return DB::kOK;
}

DB *NewRethinkDB() {
    return new RethinkDBBinding;
}

const bool registered = DBFactory::RegisterDB("rethinkdb", NewRethinkDB);

} // ycsbc