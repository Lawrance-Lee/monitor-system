#include "query_manager.h"

#include <ctime>
#include <iomanip>
#include <iostream>
#include <sstream>

namespace monitor {

QueryManager::QueryManager() = default;

QueryManager::~QueryManager() { Close(); }
bool QueryManager::Init(const std::string& host, const std::string& user,
                        const std::string& password,
                        const std::string& database) {
#ifdef ENABLE_MYSQL
  std::lock_guard<std::mutex> lock(mtx_);
  if (initialized_) {
    return true;
  }

  conn_ = mysql_init(nullptr);
  if (!conn_) {
    std::cerr << "QueryManager: mysql_init failed" << std::endl;
    return false;
  }

  if (!mysql_real_connect(conn_, host.c_str(), user.c_str(), password.c_str(),
                          database.c_str(), 0, nullptr, 0)) {
    std::cerr << "QueryManager: mysql_real_connect failed: "
              << mysql_error(conn_) << std::endl;
    mysql_close(conn_);
    conn_ = nullptr;
    return false;
  }
    // 设置字符集
  mysql_set_character_set(conn_, "utf8mb4");
  initialized_ = true;
  std::cout << "QueryManager: MySQL connection initialized" << std::endl;
  return true;
#else
  (void)host;
  (void)user;
  (void)password;
  (void)database;
  std::cerr << "QueryManager: MySQL support not enabled" << std::endl;
  return false;
#endif
}
void QueryManager::Close() {
#ifdef ENABLE_MYSQL
  std::lock_guard<std::mutex> lock(mtx_);
  if (conn_) {
    mysql_close(conn_);
    conn_ = nullptr;
  }
  initialized_ = false;
#endif
}
bool QueryManager::ValidateTimeRange(const TimeRange& range) const {
  return range.start_time <= range.end_time;
}
std::string QueryManager::FormatTime(
    const std::chrono::system_clock::time_point& tp) const {
  std::time_t t = std::chrono::system_clock::to_time_t(tp);
  std::tm tm_time;
  localtime_r(&t, &tm_time);
  char buf[32];
  std::strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", &tm_time);
  return std::string(buf);
}
std::chrono::system_clock::time_point QueryManager::ParseTime(
    const char* str) const {
  std::tm tm = {};
  std::istringstream ss(str);
  ss >> std::get_time(&tm, "%Y-%m-%d %H:%M:%S");
  return std::chrono::system_clock::from_time_t(std::mktime(&tm));
}
int QueryManager::GetTotalCount(const std::string& count_sql) {
#ifdef ENABLE_MYSQL
  if (mysql_query(conn_, count_sql.c_str()) != 0) {
    std::cerr << "QueryManager: count query failed: " << mysql_error(conn_)
              << std::endl;
    return 0;
  }

  MYSQL_RES* result = mysql_store_result(conn_);
  if (!result) {
    return 0;
  }

  int count = 0;
  MYSQL_ROW row = mysql_fetch_row(result);
  if (row && row[0]) {
    count = std::atoi(row[0]);
  }
  mysql_free_result(result);
  return count;
#else
  (void)count_sql;
  return 0;
#endif
}


}