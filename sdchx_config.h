// Copyright (c) 2015 Yandex LLC. All rights reserved.
// Author: Vasily Chekalkin <bacek@yandex-team.ru>

#ifndef SDCHX_CONFIG_H_
#define SDCHX_CONFIG_H_

extern "C" {
#include <ngx_core.h>
#include <ngx_http.h>
#include <ngx_config.h>
}

#include <vector>

#include "sdchx_pool_alloc.h"

namespace sdchx {

class DictionaryFactory;

class Config {
 public:
  explicit Config(ngx_pool_t* pool);
  ~Config();

  static Config* get(ngx_http_request_t* r);

  ngx_flag_t enable;
};


}  // namespace sdch

#endif  // SDCHX_CONFIG_H_

