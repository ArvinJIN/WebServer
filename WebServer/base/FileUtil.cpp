// @Author Lin Ya
// @Email xxbbb@vip.qq.com
#include "FileUtil.h"
#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <sys/stat.h>
#include <unistd.h>

using namespace std;
//"ae":a 表示以添加的方式打开文件；e 表示以O_CLOEXEC打开文件，当exce()以后，文件流自动关闭，是原子操作。
AppendFile::AppendFile(string filename) : fp_(fopen(filename.c_str(), "ae")) {
  // 用户提供缓冲区
  setbuffer(fp_, buffer_, sizeof buffer_);
  //在打开文件流后,读取或写入内容之前,调用 setbuffer() 可用来设置文件流的缓冲区.
}

AppendFile::~AppendFile() { fclose(fp_); }

void AppendFile::append(const char* logline, const size_t len) {
  size_t n = this->write(logline, len);
  size_t remain = len - n;
  while (remain > 0) {
    size_t x = this->write(logline + n, remain);
    if (x == 0) {
      int err = ferror(fp_); //ferror()如果设置了与流关联的错误标识符，该函数返回一个非零值，否则返回一个零值。
      if (err) fprintf(stderr, "AppendFile::append() failed !\n");
      break;
    }
    n += x;
    remain = len - n;
  }
}

//清除读写缓冲区，在需要立即把输出缓冲区的数据进行物理写入
void AppendFile::flush() { fflush(fp_); }

size_t AppendFile::write(const char* logline, size_t len) {
  return fwrite_unlocked(logline, 1, len, fp_);
}