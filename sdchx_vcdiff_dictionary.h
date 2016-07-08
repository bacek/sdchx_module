// Copyright (c) 2016 Yandex LLC. All rights reserved.
// Author: Vasily Chekalkin <bacek@yandex-team.ru>

#ifndef SDCHX_VCDIFF_DICTIONARY_H_
#define SDCHX_VCDIFF_DICTIONARY_H_

#include "sdchx_dictionary.h"

namespace sdchx {

class VCDiffDictionary : public Dictionary {
 public:
  explicit VCDiffDictionary(const std::string& filename);
  ~VCDiffDictionary();

  std::string algo() const;

  Encoder* create_encoder(ngx_pool_t* pool) const;

 private:
};


}  // namespace sdchx

#endif  // SDCHX_VCDIFF_DICTIONARY_H_

