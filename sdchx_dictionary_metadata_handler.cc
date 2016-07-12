// Copyright (c) 2016 Yandex LLC. All rights reserved.
// Author: Vasily Chekalkin <bacek@yandex-team.ru>

#include "sdchx_dictionary_metadata_handler.h"

#include <string>

#include "sdchx_dictionary.h"
#include "sdchx_module.h"
#include "sdchx_request_context.h"

namespace sdchx {

DictionaryMetadataHandler::DictionaryMetadataHandler(Dictionary *dict,
                                                     Handler *next)
    : Handler(next), dict_(dict) {}

DictionaryMetadataHandler::~DictionaryMetadataHandler() {
}

bool DictionaryMetadataHandler::init(RequestContext *ctx) {
  ctx->create_output_header("SDCHx-Server-Id", dict_->id());
  ctx->create_output_header("SDCHx-Algo", dict_->algo());
  if (!dict_->tag().empty()) {
    ctx->create_output_header("SDCHx-Tag", dict_->tag());
  }
  if (dict_->max_age()) {
    // TODO(bacek)
    char buf[64];
    snprintf(buf, 64, "max-age=%zd", dict_->max_age());
    ctx->create_output_header("Cache-Control", std::string(buf));
  }
  return true;
}

// Handle chunk of data. For example encode it with VCDIFF.
// Almost every Handler should call next_->on_data() to keep chain.
Status DictionaryMetadataHandler::on_data(const uint8_t *buf, size_t len) {
  return next()->on_data(buf, len);
}

} // namespace sdchx
