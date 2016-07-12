// Copyright (c) 2016 Yandex LLC. All rights reserved.
// Author: Vasily Chekalkin <bacek@yandex-team.ru>

#include "sdchx_dictionary_factory.h"

#include "sdchx_dictionary.h"

namespace sdchx {

DictionaryFactory::DictionaryFactory() {}

DictionaryFactory::~DictionaryFactory() {}

Dictionary* DictionaryFactory::get_dictionary_by_url(
    const std::string& url) const {

  for (std::set<Dictionary*>::iterator i = dictionaries_.begin();
      i != dictionaries_.end();
      ++i) {
    if ((*i)->url() == url) {
      return *i;
    }
  }

  return NULL;
}

}  // namespace sdchx
