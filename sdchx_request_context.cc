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

ngx_int_t RequestContext::create_output_header(
  const char *key, size_t key_len, bool dup_key,
  const char *value, size_t value_len, bool dup_value,
  ngx_table_elt_t *prev) {

  ngx_table_elt_t* h = prev
                       ? prev
                       : static_cast<ngx_table_elt_t*>(
                            ngx_list_push(&request->headers_out.headers));
  if (h == NULL) {
    return NGX_ERROR;
  }

  h->hash = 1;
  h->key.len = key_len;
  h->key.data = reinterpret_cast<u_char*>(const_cast<char*>(key));
  if (dup_key)
    h->key.data = ngx_pstrdup(request->pool, &h->key);

  h->value.len = value_len;
  h->value.data = reinterpret_cast<u_char*>(const_cast<char*>(value));
  if (dup_value)
    h->value.data = ngx_pstrdup(request->pool, &h->value);

  return NGX_OK;
}

}  // namespace sdch
