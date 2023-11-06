#ifndef YCSB_C_READ_POLICY_H_
#define YCSB_C_READ_POLICY_H_

#include "rethinkdb.h"

namespace ycsbc {

class ReadPolicy {
  public:
    ReadPolicy() {}
    ~ReadPolicy() {}
    size_t Next() {}

  private:
    
};

}  // namespace ycsbc

#endif  // YCSB_C_READ_POLICY_H_