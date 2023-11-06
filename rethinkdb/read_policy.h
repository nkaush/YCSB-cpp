#ifndef YCSB_C_READ_POLICY_H_
#define YCSB_C_READ_POLICY_H_

#include <memory>
#include <random>

#include "rethinkdb.h"

namespace ycsbc {

class ReadPolicy {
  public:
    using ConnectionList = std::vector<std::unique_ptr<RethinkDB::Connection>>;

    ReadPolicy(ConnectionList&& cl) : cl_(std::forward<ConnectionList>(cl)) {}
    virtual ~ReadPolicy() {}
    virtual size_t Next() = 0;
    RethinkDB::Connection& GetNext() { return *cl_[Next()]; }
  protected:
    size_t num_connections() const { return cl_.size(); }
  private:
    ConnectionList cl_;
};

class RoundRoubinReadPolicy : public ReadPolicy {
  public:
    RoundRoubinReadPolicy(ReadPolicy::ConnectionList&& cl) 
      : ReadPolicy(std::forward<ReadPolicy::ConnectionList>(cl)), rri_(0) {}

    size_t Next() {
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
    
    size_t Next() {
      return rand() % num_connections();
    };
}

}  // namespace ycsbc

#endif  // YCSB_C_READ_POLICY_H_