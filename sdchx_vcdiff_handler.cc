// Copyright (c) 2015 Yandex LLC. All rights reserved.
// Author: Vasily Chekalkin <bacek@yandex-team.ru>

#include "sdchx_vcdiff_handler.h"

#include <cassert>

#include "sdchx_config.h"
#include "sdchx_dictionary.h"
#include "sdchx_request_context.h"

namespace sdchx {

VCDiffHandler::VCDiffHandler(RequestContext* ctx,
                  const Dictionary* const dict,
                  const open_vcdiff::HashedDictionary* hashed_dict,
                  Handler* next)
    : Handler(next),
      ctx_(ctx),
      dict_(dict),
      enc_(hashed_dict,
        open_vcdiff::VCD_FORMAT_INTERLEAVED | open_vcdiff::VCD_FORMAT_CHECKSUM,
        false) {
  assert(next_);
}

VCDiffHandler::~VCDiffHandler() {}

bool VCDiffHandler::init(RequestContext* ctx) {
  if (!enc_.StartEncodingToInterface(this))
    return false;

  assert(ctx == ctx_);
  // We need to remove Content-Length header
  ctx->request->headers_out.content_length_n = -1;

  ctx->request->headers_out.status = 242;
  ctx->create_output_header("SDCHx-Algo", dict_->algo());
  ctx->create_output_header("SDCHx-Used-Dictionary-Id", dict_->id());

  if (Config::get(ctx->request)->webworker_mode) {
    // In WebWorker mode we bypass Browser's decoding facilities by
    // setting C-E in application/binary and set actual encoding in x-c-e
    ctx->create_output_header("X-Content-Encoding", "sdchx");
    ctx->create_output_header("Content-Encoding", "application/binary");
  }
  else {
    ctx->create_output_header("Content-Encoding", "sdchx");
  }


  return true;
}

Status VCDiffHandler::on_data(const uint8_t* buf, size_t len) {
  // It will call ".append" which will pass it to the next_
  if (!len) {
    // No data was supplied. Just pass it through.
    return next_->on_data(buf, len);
  }

  if (enc_.EncodeChunkToInterface(reinterpret_cast<const char *>(buf), len,
                                  this)) {
    Status res = next()->on_data(buf_.data(), buf_.size());
    buf_.clear();
    return res;
  } else {
    return STATUS_ERROR;
  }
}

Status VCDiffHandler::on_finish() {
  if (!enc_.FinishEncodingToInterface(this))
    return STATUS_ERROR;

  // It's not really good idea.
  if (!buf_.empty()) {
    next()->on_data(buf_.data(), buf_.size());
  }

  return next_->on_finish();
}


open_vcdiff::OutputStringInterface& VCDiffHandler::append(const char* s,
                                                          size_t n) {
  buf_.insert(buf_.end(), s, s + n);
  return *this;
}

void VCDiffHandler::clear() { buf_.clear(); }

void VCDiffHandler::push_back(char c) { append(&c, 1); }

void VCDiffHandler::ReserveAdditionalBytes(size_t res_arg) {
  buf_.reserve(buf_.size() + res_arg);
}

size_t VCDiffHandler::size() const { return buf_.size(); }

}  // namespace sdchx
