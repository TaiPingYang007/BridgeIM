#include "../../../include/server/model/offlinemessagemodel.hpp"
#include "../../../include/common/db/DbSession.hpp"
#include "../../../include/common/db/QueryResult.hpp"
#include <sstream>

// 存储用户的离线消息
void OfflineMsgModel::insert(int userid, std::string msg) {
  std::ostringstream sql;
  sql << "insert into OfflineMessage value ('" << userid << "','" << msg
      << "')";
  DbSession session;
  session.update(sql.str());
}

// 删除用户的离线消息
void OfflineMsgModel::remove(int userid) {
  std::ostringstream sql;
  sql << "delete from OfflineMessage where userid = '" << userid << "'";
  DbSession session;
  session.update(sql.str());
}

// 查询用户的离线消息
std::vector<std::string> OfflineMsgModel::query(int userid) {
  std::ostringstream sql;
  sql << "select message from OfflineMessage where userid = '" << userid
      << "'";

  std::vector<std::string> vec;
  DbSession session;
  QueryResult result = session.query(sql.str());
  if (result.valid()) {
    while (result.next()) {
      vec.emplace_back(result.getString(0));
    }
  }
  return vec;
}
