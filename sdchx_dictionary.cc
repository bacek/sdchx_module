// Copyright (c) 2015 Yandex LLC. All rights reserved.
// Author: Vasily Chekalkin <bacek@yandex-team.ru>

#include "sdchx_dictionary.h"

#include <cassert>
#include <cstring>
#include <vector>
#include <openssl/sha.h>

namespace sdchx {

namespace {

#if nginx_version < 1006000
static void
ngx_encode_base64url(ngx_str_t *dst, ngx_str_t *src)
{
	ngx_encode_base64(dst, src);
	unsigned i;

	for (i = 0; i < dst->len; i++) {
		if (dst->data[i] == '+')
			dst->data[i] = '-';
		if (dst->data[i] == '/')
			dst->data[i] = '_';
	}
}
#endif

std::string get_dict_id(const char* buf, size_t buflen) {
  SHA256_CTX ctx;
  SHA256_Init(&ctx);
  SHA256_Update(&ctx, buf, buflen);
  unsigned char sha[SHA256_DIGEST_LENGTH];
  SHA256_Final(sha, &ctx);

  std::vector<unsigned char> res;
  res.resize(44);  // Magic constant
  ngx_str_t src = {SHA256_DIGEST_LENGTH, sha};
	ngx_str_t dst = {res.size(), res.data()};
	ngx_encode_base64url(&dst, &src);

  return std::string(res.begin(), res.end());
}


}  // namespace

Dictionary::Dictionary() {}

Dictionary::~Dictionary() {}

void Dictionary::init(const char* begin,
                      const char* end) {
  size_ = end - begin;
  id_ = get_dict_id(begin, size_);
}

}  // namespace sdchx
