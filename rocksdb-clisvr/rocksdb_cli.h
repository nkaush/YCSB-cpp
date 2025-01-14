#ifndef YCSB_C_ROCKSDB_CLI_H_
#define YCSB_C_ROCKSDB_CLI_H_

#include "core/db.h"
#include "core/properties.h"
#include "rpc.h"

namespace ycsbc {

class RocksdbCli : public DB {
  friend void rpc_cont_func(void *context, void *tag);

 public:
  RocksdbCli() : complete_(false) {}
  ~RocksdbCli() override;
  void Init() override;
  void Cleanup() override;

  Status Read(const std::string &table, const std::string &key, const std::vector<std::string> *fields,
              std::vector<Field> &result) override;
  Status Scan(const std::string &table, const std::string &key, int record_count,
              const std::vector<std::string> *fields, std::vector<std::vector<Field>> &result) override;
  Status Update(const std::string &table, const std::string &key, std::vector<Field> &values) override;
  Status Insert(const std::string &table, const std::string &key, std::vector<Field> &values) override {
    return Update(table, key, values);
  }
  Status Delete(const std::string &table, const std::string &key) override;

  bool ReInitBeforeTransaction() override { return true; }

 protected:
  void pollForRpcComplete();
  void notifyRpcComplete();

 public:
  /**
   * key format:
   * | len | key |
   */
  static size_t SerializeKey(const std::string &key, char *data);

  /**
   * value format:
   * | field0_len | field0 | value0_len | value0 |...
   */
  static size_t SerializeRow(const std::vector<Field> &values, char *data);
  static void DeserializeRow(std::vector<Field> &values, const char *p, const char *lim);
  static void DeserializeRow(std::vector<Field> &values, const std::string &data);

  /**
   * value vector format:
   * | value0_len | value0 | value1_len | value1 |...
   */
  static void DeserializeRowVector(std::vector<std::vector<Field> > &values, const char *p, const char *lim);

 protected:
  static erpc::Nexus *nexus_;
  erpc::Rpc<erpc::CTransport> *rpc_;
  int session_num_;

  erpc::MsgBuffer req_;
  erpc::MsgBuffer resp_;
  static std::atomic<uint8_t> global_rpc_id_;

  bool complete_;
};

}  // namespace ycsbc

#endif
