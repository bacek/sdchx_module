// Copyright (c) 2015 Yandex LLC. All rights reserved.
// Author: Vasily Chekalkin <bacek@yandex-team.ru>

#include "sdchx_config.h"

#include "sdchx_module.h"

namespace sdchx {

Config::Config(ngx_pool_t* pool)
    : enable(NGX_CONF_UNSET), buf_size(ngx_pagesize) {
}

Config::~Config() {}

Config* Config::get(ngx_http_request_t* r) {
  return static_cast<Config*>(ngx_http_get_module_loc_conf(r, sdchx_module));
}

}  // namespace sdch
