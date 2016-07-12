// Copyright (c) 2016 Yandex LLC. All rights reserved.
// Author: Vasily Chekalkin <bacek@yandex-team.ru>

#ifndef SDCHX_DICTIONARY_FACTORY_H_
#define SDCHX_DICTIONARY_FACTORY_H_

#include <set>
#include <string>

namespace sdchx {

class Dictionary;

class DictionaryFactory {
 public:
  DictionaryFactory();
  ~DictionaryFactory();

  void add_dictionary(Dictionary* dict) {
    dictionaries_.insert(dict);
  }

  // Get Dictionary if URL we are serving is actual dictionary
  Dictionary* get_dictionary_by_url(const std::string& url) const;

 private:
  // Storage for dictionaries
  std::set<Dictionary*> dictionaries_;
};


}  // namespace sdchx

#endif  // SDCHX_DICTIONARY_FACTORY_H_

