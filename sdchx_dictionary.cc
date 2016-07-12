// Copyright (c) 2015 Yandex LLC. All rights reserved.
// Author: Vasily Chekalkin <bacek@yandex-team.ru>

#include "sdchx_dictionary.h"

#include <cassert>
#include <cstring>
#include <vector>
#include <openssl/sha.h>

#include "sdchx_fdholder.h"
#include "sdchx_pool_alloc.h"
#include "sdchx_vcdiff_encoder_factory.h"

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

bool read_file(const std::string& fn, std::vector<char>& blob) {
  blob.clear();
  FDHolder fd(open(fn.c_str(), O_RDONLY));
  if (fd == -1)
    return false;
  struct stat st;
  if (fstat(fd, &st) == -1)
    return false;
  blob.resize(st.st_size);
  if (read(fd, &blob[0], blob.size()) != (ssize_t)blob.size())
    return false;
  return true;
}


}  // namespace

Dictionary::Dictionary() : algo_("vcdiff") {}

Dictionary::~Dictionary() {}

bool Dictionary::init(ngx_pool_t* pool) {
  // TODO(bacek): implement "Factory"
  if (algo() != "vcdiff") {
    return false;
  }

  // Get Dictionary content and create EncoderFactory with it
  std::vector<char> blob;
  if (!read_file(filename(), blob))
    return false;

  encoder_factory_ = POOL_ALLOC(pool, VCDiffEncoderFactory, blob.data(),
                                blob.data() + blob.size());
  if (!encoder_factory_)
    return false;

  init(blob.data(), blob.data() + blob.size());

  return true;
}

void Dictionary::init(const char* begin,
                      const char* end) {
  size_ = end - begin;
  id_ = get_dict_id(begin, size_);
}

}  // namespace sdchx
