// @Author Lin Ya
// @Email xxbbb@vip.qq.com
#pragma once
#include <memory>
#include <string>
#include "FileUtil.h"
#include "MutexLock.h"
#include "noncopyable.h"


// TODO 提供自动归档功能
class LogFile : noncopyable {
 public:
  // 每被append flushEveryN(默认1024)次，flush一下，会往文件写，只不过，文件也是带缓冲区的
  LogFile(const std::string& basename, int flushEveryN = 1024);
  ~LogFile();

  void append(const char* logline, int len);
  void flush();
  bool rollFile();

 private:
  void append_unlocked(const char* logline, int len);

  const std::string basename_; //日志文件名
  const int flushEveryN_;

  int count_;
  std::unique_ptr<MutexLock> mutex_; //指向互斥锁
  std::unique_ptr<AppendFile> file_; //指向日志文件
};