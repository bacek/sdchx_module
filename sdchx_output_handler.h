// Copyright (c) 2015 Yandex LLC. All rights reserved.
// Author: Vasily Chekalkin <bacek@yandex-team.ru>

#ifndef SDCHX_OUTPUT_HANDLER_H_
#define SDCHX_OUTPUT_HANDLER_H_

extern "C" {
#include <ngx_config.h>
#include <nginx.h>
#include <ngx_core.h>
#include <ngx_http.h>
}

#include "sdchx_handler.h"

namespace sdchx {

class RequestContext;

// nginx output handler. Will pass data to nginx.
class OutputHandler : public Handler {
 public:
  OutputHandler(RequestContext* ctx, ngx_http_output_body_filter_pt next_body);
  ~OutputHandler();

  virtual bool init(RequestContext* ctx);

  virtual Status on_data(const uint8_t* buf, size_t len);

  virtual Status on_finish();

 private:
  Status get_buf();
  Status flush_out_buf(bool flush);
  Status write(const uint8_t* buf, size_t len);
  Status next_body();

  RequestContext* ctx_;
  ngx_http_output_body_filter_pt next_body_;

  ngx_buf_t* out_buf_;
  ngx_chain_t* free_;
  ngx_chain_t* busy_;
  ngx_chain_t* out_;
  ngx_chain_t** last_out_;
};


}  // namespace sdchx

#endif  // SDCHX_OUTPUT_HANDLER_H_

