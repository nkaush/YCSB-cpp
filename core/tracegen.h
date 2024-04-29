#pragma once

#include "db.h"
#include "properties.h"

#include <iostream>
#include <string>
#include <mutex>

namespace ycsbc {

class Tracegen : public DB {
 public:
  void Init();

  Status Read(const std::string &table, const std::string &key,
              const std::vector<std::string> *fields, std::vector<Field> &result);

  Status Scan(const std::string &table, const std::string &key, int len,
              const std::vector<std::string> *fields, std::vector<std::vector<Field>> &result);

  Status Update(const std::string &table, const std::string &key, std::vector<Field> &values);

  Status Insert(const std::string &table, const std::string &key, std::vector<Field> &values);

  Status Delete(const std::string &table, const std::string &key);

 private:
  std::ofstream f_;
};

DB *NewTracegen();

} // ycsbc
