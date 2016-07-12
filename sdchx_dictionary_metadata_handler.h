// Copyright (c) 2016 Yandex LLC. All rights reserved.
// Author: Vasily Chekalkin <bacek@yandex-team.ru>

#ifndef SDCHX_DICTIONARY_METADATA_HANDLER_H_
#define SDCHX_DICTIONARY_METADATA_HANDLER_H_

#include "sdchx_handler.h"

namespace sdchx {

class Dictionary;

// Serve HTTP headers for given Dictionary
class DictionaryMetadataHandler : public Handler {
 public:
  DictionaryMetadataHandler(Dictionary* dict, Handler* next);
  ~DictionaryMetadataHandler();

  // Called after constructor to avoid exceptions
  // Should return true if inited successfully
  virtual bool init(RequestContext* ctx);

  // Handle chunk of data. For example encode it with VCDIFF.
  // Almost every Handler should call next_->on_data() to keep chain.
  virtual Status on_data(const uint8_t* buf, size_t len);

 private:
  Dictionary* dict_;
};


}  // namespace sdchx

#endif  // SDCHX_DICTIONARY_METADATA_HANDLER_H_

