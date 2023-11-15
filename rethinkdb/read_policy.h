#ifndef YCSB_C_READ_POLICY_H_
#define YCSB_C_READ_POLICY_H_

#include <memory>
#include <random>
#include <functional>

#include "hash/MurmurHash3.h"
#include "rethinkdb.h"

namespace ycsbc {

class ReadPolicy {
  public:
    using ConnectionList = std::vector<std::unique_ptr<RethinkDB::Connection>>;

    ReadPolicy(ConnectionList&& cl) : cl_(std::forward<ConnectionList>(cl)) {}
    virtual ~ReadPolicy() {}
    virtual size_t Next(const std::string& key) = 0;
    RethinkDB::Connection& GetNext(const std::string& key) { return *cl_[Next(key)]; }
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
      std::random_device rd;  // a seed source for the random number engine
      std::mt19937 gen(rd()); // mersenne_twister_engine seeded with rd()
      std::uniform_int_distribution<> distrib(0, num_connections());
      return distrib(gen);
    };
};

class HashReadPolicy : public ReadPolicy {
  public:
    HashReadPolicy(ConnectionList&& cl) : ReadPolicy(std::forward<ReadPolicy::ConnectionList>(cl)) {}
    
    size_t Next(const std::string& key) {
      size_t hash_result;

      MurmurHash3_x86_32(&key, 32, 0, &hash_result);
      return hash_result % num_connections();
    };
  private:
    std::hash<std::string> hash_function_;
};

}  // namespace ycsbc

#endif  // YCSB_C_READ_POLICY_H_