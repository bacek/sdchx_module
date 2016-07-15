// Copyright (c) 2015 Yandex LLC. All rights reserved.
// Author: Vasily Chekalkin <bacek@yandex-team.ru>

#ifndef SDCHX_ENCODING_HANDLER_H_
#define SDCHX_ENCODING_HANDLER_H_

#include "sdchx_handler.h"

#include <vector>
#include <google/vcencoder.h>

namespace sdchx {

class Dictionary;
class RequestContext;

// Actual VCDiff encoding handler
class VCDiffHandler : public Handler,
                      public open_vcdiff::OutputStringInterface {
 public:
  VCDiffHandler(RequestContext* ctx,
                  const Dictionary* dict,
                  const open_vcdiff::HashedDictionary* hashed_dict,
                  Handler* next);
  ~VCDiffHandler();

  // sdch::Handler implementation
  virtual bool init(RequestContext* ctx);
  virtual Status on_data(const uint8_t* buf, size_t len);
  virtual Status on_finish();

  // open_vcdiff::OutputStringInterface implementation
  virtual open_vcdiff::OutputStringInterface& append(const char* s, size_t n);
  virtual void clear();
  virtual void push_back(char c);
  virtual void ReserveAdditionalBytes(size_t res_arg);
  virtual size_t size() const;

 private:
  RequestContext *ctx_;
  const Dictionary *dict_;

  // Actual encoder.
  open_vcdiff::VCDiffStreamingEncoder enc_;

  // For OutputStringInterface implementation
  std::vector<uint8_t> buf_;
};


}  // namespace sdchx

#endif  // SDCHX_ENCODING_HANDLER_H_

