// @Author Lin Ya
// @Email xxbbb@vip.qq.com
#pragma once
#include <pthread.h>
#include <stdio.h>
#include <string.h>
#include <string>
#include "LogStream.h"


class AsyncLogging;

class Logger {
 public:
  Logger(const char *fileName, int line);
  ~Logger();
  LogStream &stream() { return impl_.stream_; }

  static void setLogFileName(std::string fileName) { logFileName_ = fileName; }
  static std::string getLogFileName() { return logFileName_; }

 private:
  class Impl {
   public:
    Impl(const char *fileName, int line);
    void formatTime();

    LogStream stream_;
    int line_;
    std::string basename_;
  };
  Impl impl_;
  static std::string logFileName_; 
  //static 成员变量属于类，不属于某个具体的对象，即使创建多个对象，也只为其分配一份内存，
  //所有对象使用的都是这份内存中的数据。static成员变量的内存既不是在声明类的时候分配，
  //也不是在创建成员变量的时候分配，而是在（类外）初始化时分配。反过来说，没有类外初始化的
  //static成员变量不能使用。
};

#define LOG Logger(__FILE__, __LINE__).stream()
/*
这是一个无名对象，当时用"LOG << ..."时:
1. 构造Logger类型的临时对象，返回LogStream类型变量 
2. 调用LogStream重载的operator<<操作符，将数据写入到LogStream的Buffer中
3. 当前语句结束，Logger临时对象析构，调用Logger析构函数，将LogStream中的数据输出
*/

// __FILE__:返回所在的文件名
// __LINE__:返回所在的行号
// __func__:返回所在的函数名