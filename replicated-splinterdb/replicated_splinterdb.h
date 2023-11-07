#ifndef YCSB_C_REPLICATED_SPLINTERDB_H_
#define YCSB_C_REPLICATED_SPLINTERDB_H_

#include "core/db.h"

#include <algorithm>
#include <iostream>
#include <string>
#include <memory>
#include "core/properties.h"

#include "client/client.h"

namespace ycsbc {

class ReplicatedSplinterDB : public DB {
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
                  std::vector<Field> &values) override {
        return Insert(table, key, values);
    }

    Status Insert(const std::string &table, const std::string &key,
                  std::vector<Field> &values) override;

    Status Delete(const std::string &table, const std::string &key) override;

    void PostLoadCallback() override; 
  private:
    std::shared_ptr<replicated_splinterdb::client> client_;
};

} // ycsbc

#endif // YCSB_C_REDIS_DB_H_
