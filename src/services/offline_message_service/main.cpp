#include "offline_message.pb.h"
#include "mprpcapplication.h"
#include "mprpcprovider.h"
#include "server/model/offlinemessagemodel.hpp"
#include "server/logger.h"

#include <google/protobuf/stubs/callback.h>

#include <string>
#include <vector>

#ifndef CHATSERVER_LOG_DIR
#define CHATSERVER_LOG_DIR "."
#endif

namespace {

void setResult(offline_message::resultCode *code, int errcode,
               const std::string &errmsg) {
  code->set_errcode(errcode);
  code->set_errmsg(errmsg);
}

} // namespace

class OfflineMessageServiceImpl : public offline_message::offline_message_service {
public:
  void insert(::google::protobuf::RpcController *controller,
              const ::offline_message::offline_message_insert_request *request,
              ::offline_message::offline_message_insert_response *response,
              ::google::protobuf::Closure *done) override {
    (void)controller;

    int userid = request->userid();
    std::string msg = request->msg();

    _offlineMsgModel.insert(userid, msg);
    setResult(response->mutable_code(), 0, "");

    if (done != nullptr) {
      done->Run();
    }
  }

  void query(::google::protobuf::RpcController *controller,
             const ::offline_message::offline_message_query_request *request,
             ::offline_message::offline_message_query_response *response,
             ::google::protobuf::Closure *done) override {
    (void)controller;

    int userid = request->userid();
    std::vector<std::string> messages = _offlineMsgModel.query(userid);

    offline_message::offlineMessage *offlineMsg = response->mutable_messages();
    for (const auto &msg : messages) {
      offlineMsg->add_msg(msg);
    }

    setResult(response->mutable_code(), 0, "");
    if (done != nullptr) {
      done->Run();
    }
  }

  void remove(::google::protobuf::RpcController *controller,
              const ::offline_message::offline_message_remove_request *request,
              ::offline_message::offline_message_remove_response *response,
              ::google::protobuf::Closure *done) override {
    (void)controller;

    int userid = request->userid();
    _offlineMsgModel.remove(userid);
    setResult(response->mutable_code(), 0, "");
    if (done != nullptr) {
      done->Run();
    }
  }

private:
  OfflineMsgModel _offlineMsgModel;
};

int main(int argc, char **argv) {
  MprpcApplication::Init(argc, argv);

  if (!Logger::GetInstance().Init(CHATSERVER_LOG_DIR, "offline_msg_service", "")) {
    std::cerr << "logger init failed! log dir: " << CHATSERVER_LOG_DIR << "\n";
    exit(EXIT_FAILURE);
  }

  RpcProvider provider;
  provider.NotifyService(new OfflineMessageServiceImpl());
  provider.Run();

  return 0;
}
