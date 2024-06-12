#ifndef YCSB_C_READ_POLICY_H_
#define YCSB_C_READ_POLICY_H_

#include <memory>
#include <random>
#include <iostream>

#include "hash/MurmurHash3.h"
#include "rethinkdb.h"

#define MMHSEED 1234567890U

namespace ycsbc {

class ReadPolicy {
  public:
    using ConnectionList = std::vector<std::unique_ptr<RethinkDB::Connection>>;

    ReadPolicy(ConnectionList&& cl) : cl_(std::forward<ConnectionList>(cl)) {}
    virtual ~ReadPolicy() {}
    virtual size_t Next(const std::string& key) = 0;
    RethinkDB::Connection& GetNext(const std::string& key) { 
      size_t next = Next(key);
      return *cl_[next]; }
  protected:
    size_t num_connections() const { return cl_.size(); }
  private:
    ConnectionList cl_;
};

class RoundRoubinReadPolicy : public ReadPolicy {
  public:
    RoundRoubinReadPolicy(ReadPolicy::ConnectionList&& cl) 
      : ReadPolicy(std::forward<ReadPolicy::ConnectionList>(cl)), rri_(0) {}

    size_t Next(const std::string& key) {
      size_t ret = rri_;
      rri_ = (rri_ + 1) % num_connections();
      return ret;
    }
  private:
    size_t rri_;
};

class RandomReadPolicy : public ReadPolicy {
  public:
    RandomReadPolicy(ConnectionList&& cl) : ReadPolicy(std::forward<ReadPolicy::ConnectionList>(cl)) {}
    
    size_t Next(const std::string& key) {
      std::random_device rd;
      std::mt19937 mt(rd());
      int upper_bound = num_connections()-1;
      int lower_bound = 0;

      std::uniform_int_distribution<> distribution(lower_bound, upper_bound);
      int randnum = distribution(mt);
      return static_cast<size_t>(randnum);
    };
};

class HashReadPolicy : public ReadPolicy {
  public:
    HashReadPolicy(ConnectionList&& cl) : ReadPolicy(std::forward<ReadPolicy::ConnectionList>(cl)) {}

    size_t Next(const std::string& key) {
      uint32_t hash;

      MurmurHash3_x86_32(key.data(), static_cast<int>(key.size()), MMHSEED, &hash);
      return hash % num_connections();
    };
};

}  // namespace ycsbc

#endif  // YCSB_C_READ_POLICY_H_