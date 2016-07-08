// Copyright (c) 2016 Yandex LLC. All rights reserved.
// Author: Vasily Chekalkin <bacek@yandex-team.ru>

#include "sdchx_vcdiff_dictionary.h"

namespace sdchx {

VCDiffDictionary::VCDiffDictionary(const std::string& filename) {
}

VCDiffDictionary::~VCDiffDictionary() {}

std::string VCDiffDictionary::algo() const {
  return "vcdiff";
}

std::auto_ptr<Dictionary::Encoder> VCDiffDictionary::create_encoder() const {
  return std::auto_ptr<Encoder>();
}

}  // namespace sdchx
