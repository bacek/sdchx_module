// Copyright (c) 2015 Yandex LLC. All rights reserved.
// Author: Vasily Chekalkin <bacek@yandex-team.ru>

#ifndef SDCHX_REQUEST_CONTEXT_H_
#define SDCHX_REQUEST_CONTEXT_H_

extern "C" {
#include <ngx_config.h>
#include <nginx.h>
#include <ngx_core.h>
#include <ngx_http.h>
}

#include <memory>

namespace sdchx {

class Handler;

// Context used inside nginx to keep relevant data.
struct RequestContext {
 public:
  // Create RequestContext.
  explicit RequestContext(ngx_http_request_t* r);

  // Fetch RequestContext associated with nginx request
  static RequestContext* get(ngx_http_request_t* r);

  ngx_http_request_t* request;
  Handler*            handler;

  bool started : 1;
  bool done : 1;
  bool need_flush : 1;

  size_t total_in;
  size_t total_out;
};


}  // namespace sdchx

#endif  // SDCHX_REQUEST_CONTEXT_H_

