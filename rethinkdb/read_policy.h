#ifndef YCSB_C_READ_POLICY_H_
#define YCSB_C_READ_POLICY_H_

#include <memory>

#include "rethinkdb.h"

namespace ycsbc {

class ReadPolicy {
  public:
    using ConnectionList = std::vector<std::unique_ptr<RethinkDB::Connection>>;

    ReadPolicy(ConnectionList&& cl) : cl_(cl) {}
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
    std::vector<std::unique_ptr<RethinkDB::Connection>> conns_;
    size_t rri_;
};

}  // namespace ycsbc

#endif  // YCSB_C_READ_POLICY_H_