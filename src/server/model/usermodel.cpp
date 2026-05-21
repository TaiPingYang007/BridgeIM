#include "../../../include/server/model/usermodel.hpp"
#include "../../../include/common/db/DbSession.hpp"
#include "../../../include/common/db/QueryResult.hpp"
#include <sstream>

// User表的增加方法
bool UserModel::insert(User &user)
{
  std::ostringstream sql;
  sql << "insert into User(name,password,state) values('"
      << user.getName() << "','" << user.getPassword() << "','"
      << user.getState() << "')";

  DbSession session;
  if (session.update(sql.str()))
  {
    // 获取插入成功的用户数据生成的主键id
    user.setId(session.lastInsertId()); // getconnection获取连接句柄 mysql_insert_id获取上一次插入操作生成的主键id
    return true;
  }
  return false;
}

// User表的查询方法
User UserModel::query(
    std::string name)
{ // 1、组装sql语句
  std::ostringstream sql;
  sql << "select * from User where name = '" << name << "'";

  // 2、获取连接句柄，执行sql语句
  DbSession session;
  QueryResult result = session.query(sql.str());
  if (result.valid())
  {
    // 获取查询结果
    if (result.next())
    {
      User user;
      user.setId(result.getInt(0));
      user.setName(result.getString(1));
      user.setPassword(result.getString(2));
      user.setState(result.getString(3));

      return user;
    }
  }
  return User();
}

// User表的查询方法
User UserModel::query(int id)
{
  // 1、组装sql语句
  std::ostringstream sql;
  sql << "select * from User where id = '" << id << "'";

  // 2、获取连接句柄，执行sql语句
  DbSession session;
  QueryResult result = session.query(sql.str());
  if (result.valid())
  {
    if (result.next())
    {
      User user;
      user.setId(result.getInt(0));
      user.setName(result.getString(1));
      user.setState(result.getString(3));
      return user;
    }
  }
  return User();
}

// 更新用户状态信息
bool UserModel::updateState(User &user)
{
  // 1、组装sql语句
  std::ostringstream sql;
  sql << "update User set state = '" << user.getState()
      << "' where id = '" << user.getId() << "'";

  // 获取连接句柄，执行sql语句
  DbSession session;
  return session.update(sql.str());
}

// 重置用户的状态信息
void UserModel::resetState()
{
  // 1、组装sql语句
  std::ostringstream sql;
  sql << "update User set state = 'offline' where state = 'online'";

  // 2、获取连接句柄，执行sql语句
  DbSession session;
  session.update(sql.str());
}
