// @Author Lin Ya
// @Email xxbbb@vip.qq.com
#include "EventLoop.h"
#include <sys/epoll.h>
#include <sys/eventfd.h>
#include <iostream>
#include "Util.h"
#include "base/Logging.h"

using namespace std;
// __thread变量每一个线程有一份独立实体，各个线程的值互不干扰。
// 线程局部变量，实质是线程内部的全局变量
// 这个变量记录本线程持有的EventLoop的指针
// 一个线程最多持有一个EventLoop，所以创建EventLoop时检查该指针即可
__thread EventLoop* t_loopInThisThread = 0;

int createEventfd() {
  int evtfd = eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
  if (evtfd < 0) {
    LOG << "Failed in eventfd";
    abort();
  }
  return evtfd;
}

EventLoop::EventLoop()
    : looping_(false), 
      poller_(new Epoll()), // 创建Epoll对象，并建立epoll实例
      wakeupFd_(createEventfd()), // 创建一个事件对象，得到其文件描述符fd，用来实现线程间的通知，
      quit_(false),
      eventHandling_(false),
      callingPendingFunctors_(false),
      threadId_(CurrentThread::tid()), //当前线程ID
      pwakeupChannel_(new Channel(this, wakeupFd_)) { //创建一个pwakeupChannel_，用事件对象的文件描述符初始化，来处理线程间的通信
  if (t_loopInThisThread) {
    // LOG << "Another EventLoop " << t_loopInThisThread << " exists in this
    // thread " << threadId_;
    // 如果t_loopInThisThread不为空，那么说明本线程已经开启了一个EventLoop
  } else {
    t_loopInThisThread = this;
  }
  // pwakeupChannel_->setEvents(EPOLLIN | EPOLLET | EPOLLONESHOT);
  pwakeupChannel_->setEvents(EPOLLIN | EPOLLET);
  pwakeupChannel_->setReadHandler(bind(&EventLoop::handleRead, this));
  pwakeupChannel_->setConnHandler(bind(&EventLoop::handleConn, this));
  poller_->epoll_add(pwakeupChannel_, 0);
}

void EventLoop::handleConn() {
  // poller_->epoll_mod(wakeupFd_, pwakeupChannel_, (EPOLLIN | EPOLLET |
  // EPOLLONESHOT), 0);
  updatePoller(pwakeupChannel_, 0);
}

EventLoop::~EventLoop() {
  // wakeupChannel_->disableAll();
  // wakeupChannel_->remove();
  close(wakeupFd_);
  t_loopInThisThread = NULL;
}

void EventLoop::wakeup() {
  uint64_t one = 1;
  ssize_t n = writen(wakeupFd_, (char*)(&one), sizeof one);
  if (n != sizeof one) {
    LOG << "EventLoop::wakeup() writes " << n << " bytes instead of 8";
  }
}

void EventLoop::handleRead() {
  uint64_t one = 1;
  ssize_t n = readn(wakeupFd_, &one, sizeof one);
  if (n != sizeof one) {
    LOG << "EventLoop::handleRead() reads " << n << " bytes instead of 8";
  }
  // pwakeupChannel_->setEvents(EPOLLIN | EPOLLET | EPOLLONESHOT);
  pwakeupChannel_->setEvents(EPOLLIN | EPOLLET);
}

void EventLoop::runInLoop(Functor&& cb) {
  if (isInLoopThread())
    cb();
  else
    queueInLoop(std::move(cb));
}

//向任务队列中天街任务
void EventLoop::queueInLoop(Functor&& cb) {
  {
    MutexLockGuard lock(mutex_);
    pendingFunctors_.emplace_back(std::move(cb));
  }
// 如果是跨线程或者EventLoop正在处理之前的IO任务，那么需要使用wakeup向eventfd写入数据，唤醒epoll
  if (!isInLoopThread() || callingPendingFunctors_) wakeup();
}

void EventLoop::loop() {
  assert(!looping_); //禁止重复开启loop
  assert(isInLoopThread()); //禁止跨线程
  looping_ = true;
  quit_ = false;
  // LOG_TRACE << "EventLoop " << this << " start looping";
  std::vector<SP_Channel> ret;
  while (!quit_) {
    // cout << "doing" << endl; //每次poll调用，就是一次重新填充ret的过程，所以需清空
    ret.clear();
    // 这一步的实质是进行epoll_wait调用，根据fd的返回事件，填充对应的Channel，以准备后面执行处理事件
    ret = poller_->poll(); // ret是需要处理的Channel
    // 开始处理回调函数
    eventHandling_ = true;
    for (auto& it : ret) it->handleEvents(); // 处理该Channel的回调函数
    eventHandling_ = false;
    // 执行任务队列中的任务，这些任务可能是线程池内的IO操作，
    // 因为不能跨线程，所以被转移到Reactor线程
    doPendingFunctors();
    poller_->handleExpired();
  }
  looping_ = false;
}

void EventLoop::doPendingFunctors() {
  std::vector<Functor> functors;
  callingPendingFunctors_ = true;

  {
    MutexLockGuard lock(mutex_);
    functors.swap(pendingFunctors_);
  }

  for (size_t i = 0; i < functors.size(); ++i) functors[i]();
  callingPendingFunctors_ = false;
}

void EventLoop::quit() {
  quit_ = true;
  if (!isInLoopThread()) {
    wakeup();
  }
}
