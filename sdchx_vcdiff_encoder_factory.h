// Copyright (c) 2016 Yandex LLC. All rights reserved.
// Author: Vasily Chekalkin <bacek@yandex-team.ru>

#ifndef SDCHX_VCDIFF_ENCODER_H_
#define SDCHX_VCDIFF_ENCODER_H_

#include "sdchx_dictionary.h"

#include <google/vcencoder.h>

namespace sdchx {

class Dictionary;

class VCDiffEncoderFactory : public Dictionary::HandlerFactory {
public:
  VCDiffEncoderFactory(const char *begin, const char *end);
  ~VCDiffEncoderFactory();

  bool init();

  virtual Handler *create_handler(const Dictionary *dict,
                                  RequestContext *ctx, Handler *next) const;

private:
  open_vcdiff::HashedDictionary hashed_dictionary_;
};

}  // namespace sdchx

#endif  // SDCHX_VCDIFF_ENCODER_H_

