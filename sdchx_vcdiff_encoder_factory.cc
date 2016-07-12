// Copyright (c) 2016 Yandex LLC. All rights reserved.
// Author: Vasily Chekalkin <bacek@yandex-team.ru>

#include "sdchx_vcdiff_encoder_factory.h"

#include "sdchx_pool_alloc.h"

namespace sdchx {

namespace {
class VCDiffEncoder : public Dictionary::Encoder {
public:
  explicit VCDiffEncoder(const open_vcdiff::HashedDictionary &dict)
      : enc_(&dict, open_vcdiff::VCD_FORMAT_INTERLEAVED |
                        open_vcdiff::VCD_FORMAT_CHECKSUM,
             false) {}

private:
  open_vcdiff::VCDiffStreamingEncoder enc_;
};
}

VCDiffEncoderFactory::VCDiffEncoderFactory(const char *begin, const char *end)
    : hashed_dictionary_(begin, end - begin) {
}

VCDiffEncoderFactory::~VCDiffEncoderFactory() {
}

bool VCDiffEncoderFactory::init() {
  return hashed_dictionary_.Init();
}

Dictionary::Encoder* VCDiffEncoderFactory::create_encoder(ngx_pool_t* pool) const {
  return POOL_ALLOC(pool, VCDiffEncoder, hashed_dictionary_);
}


}  // namespace sdchx
