#include "../../../include/server/model/friendmodel.hpp"
#include "../../../include/common/db/DbSession.hpp"
#include "../../../include/common/db/QueryResult.hpp"
#include <sstream>

// 判断好友是否存在
bool FriendModel::isUserExist(int friendid)
{
  std::ostringstream sql;
  sql << "select * from User where id = " << friendid;

  DbSession session;
  QueryResult result = session.query(sql.str());
  return result.valid() && result.next();
}

// 判断是否已经是好友了（防止重复添加）
bool FriendModel::isFriend(int userid, int friendid)
{
  std::ostringstream sql;
  sql << "select * from Friend where userid = " << userid
      << " and friendid = " << friendid;

  DbSession session;
  QueryResult result = session.query(sql.str());
  return result.valid() && result.next();
}

// 添加好友关系
void FriendModel::insert(int id, int friendid)
{
  std::ostringstream sql;
  sql << "insert into Friend value ('" << id << "','" << friendid << "')";

  DbSession session;
  session.update(sql.str());
}

// 返回用户的好友列表
std::vector<User> FriendModel::query(int userid)
{
  std::vector<User> vec;

  std::ostringstream sql;
  sql << "select User.id,User.name,User.state from User inner join "
         "Friend on User.id = Friend.friendid where Friend.userid = "
      << userid;

  DbSession session;
  QueryResult result = session.query(sql.str());
  if (result.valid())
  {
    while (result.next())
    {
      User user;
      user.setId(result.getInt(0));
      user.setName(result.getString(1));
      user.setState(result.getString(2));
      vec.emplace_back(user);
    }
  }
  return vec;
}
