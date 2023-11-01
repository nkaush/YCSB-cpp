#ifndef YCSB_C_REPLICATED_SPLINTERDB_H_
#define YCSB_C_REPLICATED_SPLINTERDB_H_

#include "core/db.h"

#include <algorithm>
#include <iostream>
#include <string>
#include <memory>
#include "core/properties.h"

#include "rethinkdb.h"

namespace R = RethinkDB;

namespace ycsbc {

class RethinkDBBinding : public DB {
  public:
    void Init() override;

    Status Read(const std::string &table, const std::string &key,
                const std::vector<std::string> *fields,
                std::vector<Field> &result) override;

    Status Scan(const std::string &table, const std::string &key,
                int len, const std::vector<std::string> *fields,
                std::vector<std::vector<Field>> &result) override {
        throw std::runtime_error("Scan: function not implemented!");
    }

    Status Update(const std::string &table, const std::string &key,
                  std::vector<Field> &values) override;

    Status Insert(const std::string &table, const std::string &key,
                  std::vector<Field> &values) override;

    Status Delete(const std::string &table, const std::string &key) override;

  private:
    std::unique_ptr<R::Connection> conn_;
    R::Term durability_ = R::Term("soft");
    R::Term read_mode_ = R::Term("outdated");
};

} // ycsbc

#endif // YCSB_C_REDIS_DB_H_
