#include "rethinkdb_binding.h"

#include <memory>
#include <cstdio>

#include "core/db_factory.h"

using std::string;

const string PROP_HOSTS = "rethinkdb.hosts";
const string PROP_HOSTS_DEFAULT = "localhost";
const int DEFAULT_PORT = 28015;

const string DURABILITY_OPT_ARG = "durability";
const string PROP_DURABILITY = "rethinkdb.durability";
const string PROP_DURABILITY_DEFAULT = "soft";

const string READ_MODE_OPT_ARG = "read_mode";
const string PROP_READ_MODE = "rethinkdb.read_mode";
const string PROP_READ_MODE_DEFAULT = "outdated";
const std::array<string, 3> READ_MODES = {"single", "majority", "outdated"};

const string PROP_READ_POLICY = "rethinkdb.read_policy";
const string PROP_READ_POLICY_DEFAULT = "round_robin";

namespace ycsbc {

template<typename StringFunction>
static void SplitString(const string &str, char delimiter, StringFunction f) {
    std::size_t from = 0;
    for (std::size_t i = 0; i < str.size(); ++i) {
        if (str[i] == delimiter) {
            f(str, from, i);
            from = i + 1;
        }
    }

    if (from <= str.size()) {
        f(str, from, str.size());
    }
}

std::pair<string, int> ExtractHostAndPort(const std::string& endpoint) {
    size_t pos = endpoint.find(':');
    if (pos == string::npos) {
        return {endpoint, DEFAULT_PORT};
    }

    return {endpoint.substr(0, pos), std::stoi(endpoint.substr(pos+1))};
}

void RethinkDBBinding::Init() {
    const utils::Properties &props = *props_;

    string durability = props.GetProperty(PROP_DURABILITY, PROP_DURABILITY_DEFAULT);
    string read_mode = props.GetProperty(PROP_READ_MODE, PROP_READ_MODE_DEFAULT);
    string read_policy = props.GetProperty(PROP_READ_POLICY, PROP_READ_POLICY_DEFAULT);

    if (durability != "hard" && durability != "soft") {
        throw std::runtime_error("rethinkdb.durability must be one of [ hard | soft ]");
    }

    if (std::find(READ_MODES.begin(), READ_MODES.end(), read_mode) == READ_MODES.end()) {
        throw std::runtime_error("rethinkdb.read_mode must be one of [ single | majority | outdated ]");
    }

    string comma_sep_hosts = props.GetProperty(PROP_HOSTS, PROP_HOSTS_DEFAULT);
    ReadPolicy::ConnectionList cl;

    SplitString(comma_sep_hosts, ',', [&cl](const std::string& s, size_t pos, size_t len) {
        auto hp = ExtractHostAndPort({s, pos, len});
        cl.emplace_back(R::connect(hp.first, hp.second));
    });

    if (read_policy == "round_robin") {
        rp_ = std::make_unique<RoundRoubinReadPolicy>(std::move(cl));
    } else if (read_policy == "random") {

    }

    durability_ = R::Term(durability);
    read_mode_ = R::Term(read_mode);

    // try {
    //     conn_ = R::connect(host, port);
    // } catch (std::exception& e) {
    //     std::cout << e.what() << std::endl;
    //     exit(1);
    // }
}

DB::Status RethinkDBBinding::Read(const string &table,
                                  const string &key,
                                  const std::vector<string> *fields,
                                  std::vector<Field> &result) {
    R::Cursor cursor = R::table(table, {{READ_MODE_OPT_ARG, read_mode_}})
        .get(key)
        .run(rp_->GetNext());

    if (!cursor.is_single()) {
        throw std::runtime_error("Expected a single document");
    }

    R::Datum document = std::move(cursor).to_datum();

    if (fields == nullptr) {
        for (auto& [f, v] : document.extract_object()) {
            result.emplace_back(f, v.extract_string());
        }
    } else {
        for (const string& f : *fields) {
            result.emplace_back(f, document.extract_field(f).extract_string());
        }
    }

    return DB::kOK;
}

DB::Status RethinkDBBinding::Update(const string &table,
                                    const string &key,
                                    std::vector<DB::Field> &values) {
    std::map<string, R::Datum> to_update;
    for (auto& [f, v] : values) {
        to_update.emplace(f, R::Datum(v));
    }

    R::Datum result = R::table(table, {{READ_MODE_OPT_ARG, read_mode_}})
        .get(key)
        .update(to_update, {{DURABILITY_OPT_ARG, durability_}})
        .run(rp_->GetNext())
        .to_datum();

    auto inserted = result.extract_field("inserted").extract_number() == 0.0;
    auto deleted = result.extract_field("deleted").extract_number() == 0.0;
    auto errors = result.extract_field("errors").extract_number() == 0.0;

    return inserted && deleted && errors ? DB::kOK : DB::kError;
}

DB::Status RethinkDBBinding::Insert(const string &table,
                                    const string &key,
                                    std::vector<DB::Field> &values) {
    std::map<string, R::Datum> to_insert = {{"id", R::Datum(key)}};
    for (auto& [f, v] : values) {
        to_insert.emplace(f, R::Datum(v));
    }

    R::Datum result = R::table(table)
        .insert(to_insert, {{DURABILITY_OPT_ARG, durability_}})
        .run(rp_->GetNext())
        .to_datum();
    
    auto inserted = result.extract_field("inserted").extract_number();
    if (static_cast<int>(inserted) != 1) {
        return DB::kError;
    }

    return DB::kOK;
}

DB::Status RethinkDBBinding::Delete(const string &table,
                                        const string &key) {
    R::Datum result = R::table(table, {{READ_MODE_OPT_ARG, read_mode_}})
        .get(key)
        .delete_({{DURABILITY_OPT_ARG, durability_}})
        .run(rp_->GetNext())
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