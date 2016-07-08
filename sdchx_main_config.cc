// Copyright (c) 2015 Yandex LLC. All rights reserved.
// Author: Vasily Chekalkin <bacek@yandex-team.ru>

#include "sdchx_main_config.h"

#include "sdchx_module.h"

namespace sdchx {

MainConfig::MainConfig() {}

MainConfig::~MainConfig() {}

MainConfig* MainConfig::get(ngx_http_request_t* r) {
  return static_cast<MainConfig*>(ngx_http_get_module_main_conf(r, sdchx_module));
}

}  // namespace sdchx
