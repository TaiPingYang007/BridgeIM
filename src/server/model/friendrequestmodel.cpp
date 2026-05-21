#include "../../../include/server/model/friendrequestmodel.hpp"
#include "../../../include/common/db/DbSession.hpp"
#include "../../../include/common/db/QueryResult.hpp"
#include "../../../include/server/logger.h"
#include <sstream>

// 添加好友请求操作
bool FriendRequestModel::addFriendRequest(int userid, int targetid) {
  // 1、组装sql语句
  std::ostringstream sql;
  sql << "insert into FriendRequest (requester_id, target_id,status) values ("
      << userid << "," << targetid << "," << "'pending');";

  DbSession session;
  if (session.update(sql.str())) {
    return true;
  }

  LOG_ERROR("%s:%d: insert FriendRequest failed error sql: %s",
            __FILE_NAME__, __LINE__, sql.str().c_str());
  return false;
}

// 获取好友请求状态 userid targetid
/*
    0：accpetfriend:1
    判断 1 是不是真的向 0 发送了好友请求
*/
BoolQueryResult FriendRequestModel::isPendingRequest(int userid, int targetid) {
  // 1、组装sql语句
  std::ostringstream sql;
  sql << "select status from FriendRequest where requester_id = " << userid
      << " and target_id = " << targetid << ";";

  DbSession session;
  QueryResult result = session.query(sql.str());
  if (!result.valid()) {
    LOG_ERROR("%s:%d: select FriendRequest failed error sql: %s",
              __FILE_NAME__, __LINE__, sql.str().c_str());
    return {QueryStatus::DbError, false};
  }

  if (result.next() && result.getString(0) == "pending") {
    return {QueryStatus::Ok, true};
  }

  return {QueryStatus::NotFound, false};
}

// 写入请求结果和处理事件
bool FriendRequestModel::updateRequestStatus(int userid, int targetid,
                                             std::string result) {
  // 1、组装sql语句
  std::ostringstream sql;
  if (result == "accepted" || result == "rejected") {
    sql << "update FriendRequest set status = '" << result
        << "', handled_at = now() where requester_id = " << userid
        << " and target_id = " << targetid << ";";
  } else if (result == "pending") {
    sql << "update FriendRequest set status = '" << result
        << "', handled_at = null where requester_id = " << userid
        << " and target_id = " << targetid << ";";
  } else {
    LOG_ERROR("%s:%d: invalid FriendRequest status: %s", __FILE_NAME__,
              __LINE__, result.c_str());
    return false;
  }

  DbSession session;
  if (session.update(sql.str())) {
    return true;
  }

  LOG_ERROR("%s:%d: insert FriendRequest failed error sql: %s",
            __FILE_NAME__, __LINE__, sql.str().c_str());
  return false;
}

// 查询请求状态
RequestStatusResult FriendRequestModel::queryRequestStatus(int userid,
                                                           int targetid) {
  std::ostringstream sql;
  sql << "select status from FriendRequest where requester_id = " << userid
      << " and target_id = " << targetid << ";";

  DbSession session;
  QueryResult result = session.query(sql.str());
  if (!result.valid()) {
    LOG_ERROR("%s:%d: select FriendRequest failed error sql: %s",
              __FILE_NAME__, __LINE__, sql.str().c_str());
    return {QueryStatus::DbError, ""};
  }

  if (!result.next()) {
    return {QueryStatus::NotFound, ""};
  }

  return {QueryStatus::Ok, result.getString(0)};
}
