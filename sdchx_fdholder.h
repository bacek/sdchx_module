// Copyright (c) 2015 Yandex LLC. All rights reserved.
// Author: Vasily Chekalkin <bacek@yandex-team.ru>

#ifndef SDCHX_FDHOLDER_H_
#define SDCHX_FDHOLDER_H_

#include <unistd.h>

namespace sdchx {

// Simple RAII holder for opened files.
// It should be c++11 move only. But I'm just a poor boy, nobody loves me.
class FDHolder {
 public:
  explicit FDHolder(int fd) : fd_(fd) {}
  ~FDHolder() {
    if (fd_ != -1)
      close(fd_);
  }

  void reset(int fd) {
    FDHolder tmp(fd_);
    fd_ = fd;
  }

  operator int() { return fd_; }

 private:
  int fd_;

  FDHolder(const FDHolder&);
  FDHolder& operator=(const FDHolder&);
};


}  // namespace sdchx

#endif  // SDCHX_FDHOLDER_H_

