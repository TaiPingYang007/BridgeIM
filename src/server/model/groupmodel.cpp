#include "../../../include/server/model/groupmodel.hpp"
#include "../../../include/common/db/DbSession.hpp"
#include "../../../include/common/db/QueryResult.hpp"
#include "../../../include/server/logger.h"
#include <sstream>

// 判断群组是否已经存在
BoolQueryResult GroupModel::isGroupExist(std::string name) {
  std::ostringstream sql;
  sql << "select * from AllGroup where groupname = '" << name << "'";

  DbSession session;
  QueryResult result = session.query(sql.str());
  if (!result.valid()) {
    LOG_ERROR("%s:%d: select AllGroup failed in isGroupExist(name=%s)",
              __FILE_NAME__, __LINE__, name.c_str());
    return {QueryStatus::DbError, false};
  }

  return result.next() ? BoolQueryResult{QueryStatus::Ok, true}
                       : BoolQueryResult{QueryStatus::NotFound, false};
}

BoolQueryResult GroupModel::isGroupExist(int groupid) {
  std::ostringstream sql;
  sql << "select id from AllGroup where id = " << groupid;

  DbSession session;
  QueryResult result = session.query(sql.str());
  if (!result.valid()) {
    LOG_ERROR("%s:%d: select AllGroup failed in isGroupExist(id=%d)",
              __FILE_NAME__, __LINE__, groupid);
    return {QueryStatus::DbError, false};
  }

  return result.next() ? BoolQueryResult{QueryStatus::Ok, true}
                       : BoolQueryResult{QueryStatus::NotFound, false};
}

// 创建群组
bool GroupModel::createGroup(Group &group) {
  std::ostringstream sql;
  sql << "insert into AllGroup (groupname , groupdesc) value ('"
      << group.getName() << "','" << group.getDesc() << "')";
  DbSession session;
  if (session.update(sql.str())) {
    group.setId(session.lastInsertId());
    return true;
  }
  return false;
}

// 加入群组
void GroupModel::addGroup(int userid, int groupid, std::string role) {
  std::ostringstream sql;
  sql << "insert into GroupUser value (" << groupid << "," << userid << ",'"
      << role << "')";
  DbSession session;
  session.update(sql.str());
}

// 判断是不是已经在群组中
BoolQueryResult GroupModel::isUserInGroup(int userid, int groupid) {
  std::ostringstream sql;
  sql << "select * from GroupUser where userid = '" << userid
      << "' and groupid = '" << groupid << "'";

  DbSession session;
  QueryResult result = session.query(sql.str());
  if (!result.valid()) {
    LOG_ERROR("%s:%d: select GroupUser failed in isUserInGroup(userid=%d, groupid=%d)",
              __FILE_NAME__, __LINE__, userid, groupid);
    return {QueryStatus::DbError, false};
  }

  return result.next() ? BoolQueryResult{QueryStatus::Ok, true}
                       : BoolQueryResult{QueryStatus::NotFound, false};
}

// 获取群主id
IntQueryResult GroupModel::queryGroupOwnerId(int groupid) {
  std::ostringstream sql;
  sql << "select userid from GroupUser where groupid = " << groupid
      << " and grouprole = 'creator'";

  DbSession session;
  QueryResult result = session.query(sql.str());
  if (!result.valid()) {
    return {QueryStatus::DbError, -1};
  }

  if (!result.next()) {
    return {QueryStatus::NotFound, -1};
  }

  return {QueryStatus::Ok, result.getInt(0)};
}

// 获取群组名
std::string GroupModel::queryGroupName(int groupid) {
  std::string groupname = "";
  std::ostringstream sql;
  sql << "select groupname from AllGroup where id = " << groupid;

  DbSession session;
  QueryResult result = session.query(sql.str());
  if (result.valid() && result.next()) {
    groupname = result.getString(0);
  }
  return groupname;
}

// 查询用户所在群组信息
std::vector<Group> GroupModel::queryGroup(int userid) {
  std::ostringstream sql;
  sql << "select a.id,a.groupname,a.groupdesc from AllGroup a inner join "
         "GroupUser b on a.id = b.groupid where b.userid = "
      << userid;

  std::vector<Group> groupVec;
  DbSession session;
  QueryResult result = session.query(sql.str());
  if (result.valid()) {
    while (result.next()) {
      Group group;
      group.setId(result.getInt(0));
      group.setName(result.getString(1));
      group.setDesc(result.getString(2));
      groupVec.emplace_back(group);
    }
  }

  for (Group &group : groupVec) {
    std::ostringstream memberSql;
    memberSql << "select a.id,a.name,a.state,b.grouprole from User a inner join "
                 "GroupUser b on a.id = b.userid where b.groupid = "
              << group.getId();
    DbSession memberSession;
    QueryResult memberResult = memberSession.query(memberSql.str());
    if (memberResult.valid()) {
      while (memberResult.next()) {
        GroupUser user;
        user.setId(memberResult.getInt(0));
        user.setName(memberResult.getString(1));
        user.setState(memberResult.getString(2));
        user.setRole(memberResult.getString(3));
        group.getUsers().emplace_back(user);
      }
    }
  }
  return groupVec;
}

// 根据指定的groupid查询群组用户id列表，除userid自己，主要用户群聊业务给群组其他成员群发消息
std::vector<int> GroupModel::queryGroupUsers(int userid, int groupid) {
  std::ostringstream sql;
  sql << "select userid from GroupUser where groupid = " << groupid
      << " and userid != " << userid;
  std::vector<int> idVec;
  DbSession session;
  QueryResult result = session.query(sql.str());
  if (result.valid()) {
    while (result.next()) {
      idVec.emplace_back(result.getInt(0));
    }
  }
  return idVec;
}
