#include "friend_service.pb.h"
#include "mprpcapplication.h"
#include "mprpcprovider.h"
#include "server/model/friendmodel.hpp"
#include "server/model/friendrequestmodel.hpp"
#include "server/logger.h"

#include <google/protobuf/stubs/callback.h>

#include <string>
#include <vector>

#ifndef CHATSERVER_LOG_DIR
#define CHATSERVER_LOG_DIR "."
#endif

namespace {

// C++ QueryStatus → proto QueryStatus 转换
friend_service::QueryStatus toProtoQueryStatus(QueryStatus cs) {
  switch (cs) {
  case QueryStatus::Ok:
    return friend_service::QS_OK;
  case QueryStatus::NotFound:
    return friend_service::QS_NOT_FOUND;
  case QueryStatus::DbError:
    return friend_service::QS_DB_ERROR;
  }
  return friend_service::QS_DB_ERROR;
}

void setResult(friend_service::resultCode *code, int errcode,
               const std::string &errmsg) {
  code->set_errcode(errcode);
  code->set_errmsg(errmsg);
}

} // namespace

class FriendServiceImpl : public friend_service::friend_service {
public:
  void isUserExist(
      ::google::protobuf::RpcController *controller,
      const ::friend_service::is_user_exist_request *request,
      ::friend_service::is_user_exist_response *response,
      ::google::protobuf::Closure *done) override {
    (void)controller;

    int friendid = request->friendid();
    bool exists = _friendModel.isUserExist(friendid);

    // exists=true → status=OK, value=true
    // exists=false → status=OK, value=false (用户不存在是正常业务结果，不是 DB 错误)
    response->mutable_result()->set_status(::friend_service::QS_OK);
    response->mutable_result()->set_value(exists);

    if (done != nullptr) {
      done->Run();
    }
  }

  void isFriend(
      ::google::protobuf::RpcController *controller,
      const ::friend_service::is_friend_request *request,
      ::friend_service::is_friend_response *response,
      ::google::protobuf::Closure *done) override {
    (void)controller;

    int userid = request->userid();
    int friendid = request->friendid();
    bool isFriend = _friendModel.isFriend(userid, friendid);

    response->mutable_result()->set_status(::friend_service::QS_OK);
    response->mutable_result()->set_value(isFriend);

    if (done != nullptr) {
      done->Run();
    }
  }

  void insertFriend(
      ::google::protobuf::RpcController *controller,
      const ::friend_service::insert_friend_request *request,
      ::friend_service::insert_friend_response *response,
      ::google::protobuf::Closure *done) override {
    (void)controller;

    int userid = request->userid();
    int friendid = request->friendid();
    _friendModel.insert(userid, friendid);

    setResult(response->mutable_code(), 0, "");

    if (done != nullptr) {
      done->Run();
    }
  }

  void queryFriends(
      ::google::protobuf::RpcController *controller,
      const ::friend_service::query_friends_request *request,
      ::friend_service::query_friends_response *response,
      ::google::protobuf::Closure *done) override {
    (void)controller;

    int userid = request->userid();
    std::vector<User> friends = _friendModel.query(userid);

    setResult(response->mutable_code(), 0, "");

    for (const User &user : friends) {
      ::friend_service::FriendInfo *info = response->add_friends();
      info->set_id(user.getId());
      info->set_name(user.getName());
      info->set_state(user.getState());
    }

    if (done != nullptr) {
      done->Run();
    }
  }

  void addFriendRequest(
      ::google::protobuf::RpcController *controller,
      const ::friend_service::add_friend_request_req *request,
      ::friend_service::add_friend_request_resp *response,
      ::google::protobuf::Closure *done) override {
    (void)controller;

    int userid = request->userid();
    int targetid = request->targetid();
    bool ok = _friendRequestModel.addFriendRequest(userid, targetid);

    setResult(response->mutable_code(), ok ? 0 : 1,
              ok ? "" : "addFriendRequest failed");

    if (done != nullptr) {
      done->Run();
    }
  }

  void isPendingRequest(
      ::google::protobuf::RpcController *controller,
      const ::friend_service::is_pending_request_req *request,
      ::friend_service::is_pending_request_resp *response,
      ::google::protobuf::Closure *done) override {
    (void)controller;

    int userid = request->userid();
    int targetid = request->targetid();
    BoolQueryResult result = _friendRequestModel.isPendingRequest(userid, targetid);

    response->mutable_result()->set_status(toProtoQueryStatus(result.status));
    response->mutable_result()->set_value(result.value);

    if (done != nullptr) {
      done->Run();
    }
  }

  void updateRequestStatus(
      ::google::protobuf::RpcController *controller,
      const ::friend_service::update_request_status_req *request,
      ::friend_service::update_request_status_resp *response,
      ::google::protobuf::Closure *done) override {
    (void)controller;

    int userid = request->userid();
    int targetid = request->targetid();
    std::string status = request->status();
    bool ok = _friendRequestModel.updateRequestStatus(userid, targetid, status);

    setResult(response->mutable_code(), ok ? 0 : 1,
              ok ? "" : "updateRequestStatus failed");

    if (done != nullptr) {
      done->Run();
    }
  }

  void queryRequestStatus(
      ::google::protobuf::RpcController *controller,
      const ::friend_service::query_request_status_req *request,
      ::friend_service::query_request_status_resp *response,
      ::google::protobuf::Closure *done) override {
    (void)controller;

    int userid = request->userid();
    int targetid = request->targetid();
    RequestStatusResult result =
        _friendRequestModel.queryRequestStatus(userid, targetid);

    response->mutable_result()->set_status(toProtoQueryStatus(result.status));
    response->mutable_result()->set_value(result.value);

    if (done != nullptr) {
      done->Run();
    }
  }

private:
  FriendModel _friendModel;
  FriendRequestModel _friendRequestModel;
};

int main(int argc, char **argv) {
  MprpcApplication::Init(argc, argv);

  if (!Logger::GetInstance().Init(CHATSERVER_LOG_DIR, "friend_service", "")) {
    std::cerr << "logger init failed! log dir: " << CHATSERVER_LOG_DIR << "\n";
    exit(EXIT_FAILURE);
  }

  RpcProvider provider;
  provider.NotifyService(new FriendServiceImpl());
  provider.Run();

  return 0;
}
