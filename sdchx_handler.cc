// Copyright (c) 2016 Yandex LLC. All rights reserved.
// Author: Vasily Chekalkin <bacek@yandex-team.ru>

#include "sdchx_handler.h"

namespace sdchx {

Handler::Handler(Handler* next) : next_(next) {}

Handler::~Handler() {}

Status Handler::on_finish() {
  if (next_)
    return next_->on_finish();
  return STATUS_OK;
}

}  // namespace sdchx
