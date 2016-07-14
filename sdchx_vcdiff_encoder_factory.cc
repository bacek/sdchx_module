// Copyright (c) 2016 Yandex LLC. All rights reserved.
// Author: Vasily Chekalkin <bacek@yandex-team.ru>

#include "sdchx_vcdiff_encoder_factory.h"

#include "sdchx_pool_alloc.h"
#include "sdchx_vcdiff_handler.h"
#include "sdchx_request_context.h"

namespace sdchx {

VCDiffEncoderFactory::VCDiffEncoderFactory(const char *begin, const char *end)
    : hashed_dictionary_(begin, end - begin) {
}

VCDiffEncoderFactory::~VCDiffEncoderFactory() {
}

bool VCDiffEncoderFactory::init() {
  return hashed_dictionary_.Init();
}

Handler *VCDiffEncoderFactory::create_handler(const Dictionary *dict,
                                              RequestContext *ctx,
                                              Handler *next) const {
  return POOL_ALLOC(ctx->request->pool, VCDiffHandler, ctx, dict,
                    &hashed_dictionary_, next);
}

}  // namespace sdchx
