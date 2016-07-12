// Copyright (c) 2016 Yandex LLC. All rights reserved.
// Author: Vasily Chekalkin <bacek@yandex-team.ru>

#ifndef SDCHX_DICTIONARY_FACTORY_H_
#define SDCHX_DICTIONARY_FACTORY_H_

#include <set>

namespace sdchx {

class Dictionary;

class DictionaryFactory {
 public:
  DictionaryFactory();
  ~DictionaryFactory();


 private:
  // Storage for dictionaries
  std::set<Dictionary*> dictionaries_;
};


}  // namespace sdchx

#endif  // SDCHX_DICTIONARY_FACTORY_H_

