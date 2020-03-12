// @Author Lin Ya
// @Email xxbbb@vip.qq.com
#pragma once
#include <stdint.h>

namespace CurrentThread {
// internal
extern __thread int t_cachedTid;
extern __thread char t_tidString[32];
extern __thread int t_tidStringLength;
extern __thread const char* t_threadName;
void cacheTid();

// 这个指令是gcc引入的，作用是允许程序员将最有可能执行的分支告诉编译器。
// 这个指令的写法为：__builtin_expect(EXP, N)。
// 意思是：EXP==N的概率很大
// 这样编译器可以对代码进行优化，以减少指令跳转带来的性能下降。
inline int tid() {
  if (__builtin_expect(t_cachedTid == 0, 0)) {
    cacheTid();
  }
  return t_cachedTid;
}

inline const char* tidString()  // for logging
{
  return t_tidString;
}

inline int tidStringLength()  // for logging
{
  return t_tidStringLength;
}

inline const char* name() { return t_threadName; }
}
