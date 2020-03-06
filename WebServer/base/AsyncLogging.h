// @Author Lin Ya
// @Email xxbbb@vip.qq.com
#pragma once
#include <functional>
#include <string>
#include <vector>
#include "CountDownLatch.h"
#include "LogStream.h"
#include "MutexLock.h"
#include "Thread.h"
#include "noncopyable.h"


class AsyncLogging : noncopyable {
 public:
  AsyncLogging(const std::string basename, int flushInterval = 2);
  ~AsyncLogging() {
    if (running_) stop();
  }
  void append(const char* logline, int len);

  void start() {
    running_ = true;
    thread_.start();
    latch_.wait();
  }

  void stop() {
    running_ = false;
    cond_.notify();
    thread_.join();
  }

 private:
  void threadFunc();
  typedef FixedBuffer<kLargeBuffer> Buffer;
  typedef std::vector<std::shared_ptr<Buffer>> BufferVector;
  typedef std::shared_ptr<Buffer> BufferPtr;
  const int flushInterval_;//冲刷间隔
  bool running_;//确定日志文件是否再运行，条件变量的循环的判断条件
  std::string basename_;//日志文件头部文件名
  Thread thread_;//为添加日志单独建一个线程
  MutexLock mutex_;//互斥量，线程安全的添加日志。
  Condition cond_;//条件变量，当有日志添加进来的时候通知。当日志缓冲区没准备好的时候等待
  BufferPtr currentBuffer_;//日志缓冲区
  BufferPtr nextBuffer_;//下一块日志缓冲区
  BufferVector buffers_;//已更换下来的日志文件的存储区
  CountDownLatch latch_;//
};