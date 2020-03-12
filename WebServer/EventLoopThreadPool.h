// @Author Lin Ya
// @Email xxbbb@vip.qq.com
#pragma once
#include <memory>
#include <vector>
#include "EventLoopThread.h"
#include "base/Logging.h"
#include "base/noncopyable.h"


class EventLoopThreadPool : noncopyable {
 public:
  EventLoopThreadPool(EventLoop* baseLoop, int numThreads);

  ~EventLoopThreadPool() { LOG << "~EventLoopThreadPool()"; }
  void start();

  EventLoop* getNextLoop();

 private:
  EventLoop* baseLoop_; // 指向事件循环
  bool started_;
  int numThreads_; // 创建的线程池线程数量
  int next_;
  std::vector<std::shared_ptr<EventLoopThread>> threads_; //存储线程篇的容器
  std::vector<EventLoop*> loops_;
};