#include "../../../include/server/model/grouprequestmodel.hpp"
#include "../../../include/common/db/DbSession.hpp"
#include "../../../include/common/db/QueryResult.hpp"
#include "../../../include/server/logger.h"
#include <sstream>

// 向群组请求表添加群组请求
bool GroupRequestModel::addGroupRequest(int userid, int groupid) {
  // 1、组装sql语句
  std::ostringstream sql;
  sql << "insert into GroupRequest (requester_id, group_id, status) values ("
      << userid << "," << groupid << ", 'pending');";

  DbSession session;
  if (session.update(sql.str())) {
    return true;
  }

  LOG_ERROR("%s:%d: insert GroupRequest failed error sql: %s",
            __FILE_NAME__, __LINE__, sql.str().c_str());
  return false;
}

// 判断群组请求状态是不是pending
bool GroupRequestModel::isPendingRequest(int userid, int groupid) {
  // 1、组装sql语句
  std::ostringstream sql;
  sql << "select status from GroupRequest where requester_id = " << userid
      << " and group_id = " << groupid << ";";

  DbSession session;
  QueryResult result = session.query(sql.str());
  if (!result.valid()) {
    LOG_ERROR("%s:%d: select GroupRequest failed error sql: %s",
              __FILE_NAME__, __LINE__, sql.str().c_str());
    return false;
  }

  return result.next() && result.getString(0) == "pending";
}
// 更新群组请求状态
bool GroupRequestModel::updateRequestStatus(int userid, int groupid,
                                            std::string result) {
  // 1、组装sql语句
  std::ostringstream sql;
  // 如果是更新为accepted和rejected，需要修改时间
  if (result == "accepted" || result == "rejected") {
    sql << "update GroupRequest set status = '" << result
        << "', handled_at = now() where requester_id = " << userid
        << " and group_id = " << groupid << ";";
  } else if (result == "pending") {

    sql << "update GroupRequest set status = '" << result
        << "', handled_at = null where requester_id = " << userid
        << " and group_id = " << groupid << ";";
  } else {
    LOG_ERROR("%s:%d: invalid GroupRequest status: %s", __FILE_NAME__,
              __LINE__, result.c_str());
    return false;
  }

  DbSession session;
  if (session.update(sql.str())) {
    return true;
  }

  LOG_ERROR("%s:%d: update GroupRequest failed error sql: %s",
            __FILE_NAME__, __LINE__, sql.str().c_str());
  return false;
}
// 查询群组请求状态
RequestStatusResult GroupRequestModel::queryRequestStatus(int userid, int groupid) {
  // 1、组装sql语句
  std::ostringstream sql;
  sql << "select status from GroupRequest where requester_id = " << userid
      << " and group_id = " << groupid << ";";

  DbSession session;
  QueryResult result = session.query(sql.str());
  if (!result.valid()) {
    LOG_ERROR("%s:%d: select GroupRequest failed error sql: %s",
              __FILE_NAME__, __LINE__, sql.str().c_str());
    return {QueryStatus::DbError, ""};
  }

  if (!result.next()) {
    return {QueryStatus::NotFound, ""};
  }

  return {QueryStatus::Ok, result.getString(0)};
}
