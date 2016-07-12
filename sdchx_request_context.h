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

#include <string>

namespace sdchx {

class Handler;

// Context used inside nginx to keep relevant data.
struct RequestContext {
 public:
  // Create RequestContext.
  explicit RequestContext(ngx_http_request_t* r);

  // Fetch RequestContext associated with nginx request
  static RequestContext* get(ngx_http_request_t* r);

  ngx_int_t create_output_header(
    const char* key, size_t key_len, bool dup_key,
    const char* value, size_t value_len, bool dup_value,
    ngx_table_elt_t* prev);

  template <size_t K>
  ngx_int_t create_output_header(const char (&key)[K], const std::string &value,
                                 ngx_table_elt_t *prev = NULL) {
    return create_output_header(key, K - 1, false,
                                value.data(), value.length(), true,
                                prev);
  }

  template <size_t K, size_t V>
  ngx_int_t
  create_output_header(const char (&key)[K],
                       const char (&value)[V], ngx_table_elt_t *prev = NULL) {
    return create_output_header(key, K - 1, false, value, V - 1, false, prev);
  }

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

