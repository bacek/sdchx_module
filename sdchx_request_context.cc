// Copyright (c) 2015 Yandex LLC. All rights reserved.
// Author: Vasily Chekalkin <bacek@yandex-team.ru>

#include "sdchx_request_context.h"

#include "sdchx_module.h"

namespace sdchx {

RequestContext::RequestContext(ngx_http_request_t* r) : request(r) {
  ngx_http_set_ctx(r, this, sdchx_module);
}

RequestContext* RequestContext::get(ngx_http_request_t* r) {
  return static_cast<RequestContext*>(ngx_http_get_module_ctx(r, sdchx_module));
}

}  // namespace sdch
