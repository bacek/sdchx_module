// Copyright (c) 2016 Yandex LLC. All rights reserved.
// Author: Vasily Chekalkin <bacek@yandex-team.ru>

#include "sdchx_dictionary_metadata_handler.h"

#include <string>

#include "sdchx_dictionary.h"
#include "sdchx_module.h"
#include "sdchx_request_context.h"

namespace sdchx {

template <size_t K>
ngx_int_t create_output_header(ngx_http_request_t* r,
                               const char (&key)[K],
                               const std::string& value,
                               ngx_table_elt_t* prev = NULL) {
  ngx_table_elt_t* h = prev
                       ? prev
                       : static_cast<ngx_table_elt_t*>(
                            ngx_list_push(&r->headers_out.headers));
  if (h == NULL) {
    return NGX_ERROR;
  }

  h->hash = 1;
  ngx_str_set(&h->key, key);
  h->value.len = value.length();
  h->value.data = reinterpret_cast<u_char*>(const_cast<char*>(value.data()));
  h->value.data = ngx_pstrdup(r->pool, &h->value);

  return NGX_OK;
}

DictionaryMetadataHandler::DictionaryMetadataHandler(Dictionary *dict,
                                                     Handler *next)
    : Handler(next), dict_(dict) {}

DictionaryMetadataHandler::~DictionaryMetadataHandler() {
}

bool DictionaryMetadataHandler::init(RequestContext *ctx) {
  create_output_header(ctx->request, "SDCHx-Server-Id", dict_->id());
  create_output_header(ctx->request, "SDCHx-Algo", dict_->algo());
  if (!dict_->tag().empty()) {
    create_output_header(ctx->request, "SDCHx-Tag", dict_->tag());
  }
  if (dict_->max_age()) {
    // TODO(bacek)
    char buf[64];
    snprintf(buf, 64, "max-age=%zd", dict_->max_age());
    create_output_header(ctx->request, "Cache-Control", buf);
  }
  return true;
}

// Handle chunk of data. For example encode it with VCDIFF.
// Almost every Handler should call next_->on_data() to keep chain.
Status DictionaryMetadataHandler::on_data(const uint8_t *buf, size_t len) {
  return next()->on_data(buf, len);
}

} // namespace sdchx
