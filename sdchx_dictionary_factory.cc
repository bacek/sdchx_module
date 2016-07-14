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

Dictionary *DictionaryFactory::select_dictionary(
    const std::vector<std::string> &available_dictionaries) const {

  if (available_dictionaries.empty())
    return NULL;

  std::string client_id = available_dictionaries[0];
  for (std::set<Dictionary*>::const_iterator i = dictionaries_.begin();
      i != dictionaries_.end();
      ++i) {
    if ((*i)->client_id() == client_id)
      return *i;
  }

  return NULL;
}

}  // namespace sdchx
