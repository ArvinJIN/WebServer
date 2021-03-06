// @Author Lin Ya
// @Email xxbbb@vip.qq.com
// 通道类
// 每个Channel对象自始至终只属于一个EventLoop，因此每个Channel对象都只属于某一个IO线程。
// 每个Channel对象自始至终只负责一个文件描述符（fd）的IO事件分发，但它并不拥有这个fd，
// 也不会在析构的时候关闭这个fd，Channel会把不同的IO事件分发为不同的回调，例如readHandler_
// writeHandler_

// * 负责注册读写事件的类，并保存了fd读写事件发生时调用的回调函数，如果epoll有读写事件发生则
// 将这些事件添加到对应的通道中
// * 一个通道对应唯一一个EventLoop，一个EventLoop可以有多个通道
// * Channel类不负责fd的生命期，fd的生命期由socket决定
// * 当有fd返回读写事件时，调用提前注册的回调函数处理读写事件

#pragma once
#include <sys/epoll.h>
#include <sys/epoll.h>
#include <functional>
#include <memory>
#include <string>
#include <unordered_map>
#include "Timer.h"

class EventLoop;
class HttpData;

class Channel {
 private:
  typedef std::function<void()> CallBack;
  EventLoop *loop_; //持有该Channel的EventLoop的指针
  int fd_; //Channel对应的fd,
  __uint32_t events_; //该fd_正在监听的事件
  __uint32_t revents_; //epoll调用后，该fd需要处理的事件，依据它，epoller调用它相应的回调函数
  __uint32_t lastEvents_;

  // 方便找到上层持有该Channel的对象
  std::weak_ptr<HttpData> holder_;

 private:
  int parse_URI();
  int parse_Headers();
  int analysisRequest();

//四种回调函数
  CallBack readHandler_;
  CallBack writeHandler_;
  CallBack errorHandler_;
  CallBack connHandler_;

 public:
  Channel(EventLoop *loop);
  Channel(EventLoop *loop, int fd);
  ~Channel();
  int getFd();
  void setFd(int fd);

  void setHolder(std::shared_ptr<HttpData> holder) { holder_ = holder; }
  std::shared_ptr<HttpData> getHolder() {
      //使用lock()从被观测的shared_ptr获得一个可用的shared_ptr对象，从而操作资源.
    std::shared_ptr<HttpData> ret(holder_.lock());
    return ret;
  }

//设置回调函数
  void setReadHandler(CallBack &&readHandler) { 
    readHandler_ = readHandler; 
  }
  void setWriteHandler(CallBack &&writeHandler) {
    writeHandler_ = writeHandler;
  }
  void setErrorHandler(CallBack &&errorHandler) {
    errorHandler_ = errorHandler;
  }
  void setConnHandler(CallBack &&connHandler) { 
    connHandler_ = connHandler; 
  }

  void handleEvents() {
    events_ = 0;
    if ((revents_ & EPOLLHUP) && !(revents_ & EPOLLIN)) { //文件被挂断并且文件不可读
      events_ = 0;
      return;
    }
    if (revents_ & EPOLLERR) { //处理错误
      if (errorHandler_) errorHandler_();
      events_ = 0;
      return;
    }
    if (revents_ & (EPOLLIN | EPOLLPRI | EPOLLRDHUP)) { //EPOLLIN : 文件可读
      handleRead();                 //EPOLLPRI : 文件有紧急数据可读
    }                               //EPOLLRDHUP: 对端关闭连接或者shutdown写入半连接
    if (revents_ & EPOLLOUT) { //EPOLLOUT : 文件可写
      handleWrite();
    }
    handleConn();   // socket的fd 则绑定的是Server::handThisConn
  }                 // eventfd(wakeupFd_) 则绑定的是EventLoop::handleConn   
  void handleRead();
  void handleWrite();
  void handleError(int fd, int err_num, std::string short_msg);
  void handleConn();

  void setRevents(__uint32_t ev) { revents_ = ev; }

  void setEvents(__uint32_t ev) { events_ = ev; }
  __uint32_t &getEvents() { return events_; }

  bool EqualAndUpdateLastEvents() {
    bool ret = (lastEvents_ == events_);
    lastEvents_ = events_;
    return ret;
  }

  __uint32_t getLastEvents() { return lastEvents_; }
};

typedef std::shared_ptr<Channel> SP_Channel;