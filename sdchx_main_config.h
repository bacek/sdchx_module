// Copyright (c) 2015 Yandex LLC. All rights reserved.
// Author: Vasily Chekalkin <bacek@yandex-team.ru>

#ifndef SDCHX_MAIN_CONFIG_H_
#define SDCHX_MAIN_CONFIG_H_

extern "C" {
#include <ngx_core.h>
#include <ngx_http.h>
#include <ngx_config.h>
}

#include "sdchx_dictionary_factory.h"

namespace sdchx {

class MainConfig {
 public:
  MainConfig();
  ~MainConfig();

  static MainConfig* get(ngx_http_request_t* r);

  // Put factories here
  DictionaryFactory dictionary_factory;
};


}  // namespace sdchx

#endif  // SDCHX_MAIN_CONFIG_H_

