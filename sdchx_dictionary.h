// Copyright (c) 2015 Yandex LLC. All rights reserved.
// Author: Vasily Chekalkin <bacek@yandex-team.ru>

#ifndef SDCHX_DICTIONARY_H_
#define SDCHX_DICTIONARY_H_

extern "C" {
#include <ngx_config.h>
#include <nginx.h>
#include <ngx_core.h>
#include <ngx_http.h>
}

#include <stdint.h>  // uint8_t
#include <memory>
#include <string>

namespace sdchx {

class Handler;
class RequestContext;

// In-memory Dictionary representation
// Should NOT be allocated from short-lived nginx pool. For example
// request->pool is totally unsuitable for FastDict.
//
// This is base class for particular encoding implementation
class Dictionary {
 public:
  // EncoderFactory creates Handler for each request.
   class HandlerFactory {
   public:
     virtual ~HandlerFactory(){};
     virtual bool init() = 0;
     virtual Handler *create_handler(const Dictionary *dict,
                                     RequestContext *ctx,
                                     Handler *next) const = 0;
   };

  Dictionary();
  virtual ~Dictionary();

  // Initialize Dictionary. Create EncoderFactory for given algo, etc.
  // TODO(bacek): Add error message
  bool init(ngx_pool_t* pool);

  // Size of dictionary
  size_t size() const {
    return size_;
  }

  const std::string& id() const {
    return id_;
  }

  const std::string& client_id() const {
    return client_id_;
  }

  const std::string& url() const {
    return url_;
  }
  void set_url(const std::string& url) {
    url_ = url;
  }


  // SDCHx-Tag header value. If any
  const std::string& tag() const {
    return tag_;
  }
  void set_tag(const std::string& tag) {
    tag_ = tag;
  }

  // Part of Cache-Control: max-age=<age>. Default is 30 days.
  size_t max_age() const {
    return max_age_;
  }
  void set_max_age(size_t max_age) {
    max_age_ = max_age;
  }

  const std::string& algo() const {
    return algo_;
  }
  void set_algo(const std::string algo) {
    algo_ = algo;
  }

  const std::string& filename() const {
    return filename_;
  }
  void set_filename(const std::string filename) {
    filename_ = filename;
  }

  // Create specific Encoder. Must be allocated from pool
  Handler* create_handler(RequestContext* ctx, Handler* next) const {
    return encoder_factory_->create_handler(this, ctx, next);
  }

 protected:

  // Calculate id and remember size. Just in case.
  void init(const char* begin, const char* end);

 private:
  std::string id_;
  std::string client_id_;
  std::string filename_;
  std::string algo_;  // = "vcdiff";
  size_t size_;

  std::string url_;
  std::string tag_;
  size_t max_age_;

  HandlerFactory* encoder_factory_;
};


}  // namespace sdchx

#endif  // SDCHX_DICTIONARY_H_

