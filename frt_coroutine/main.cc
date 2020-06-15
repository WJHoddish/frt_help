// Copyright (C) spring 2020 Jiaheng Wang
// Author: Jiaheng Wang <wjhgeneral@outlook.com>
//

#include <iostream>
#include <memory>

#include "coroutine.h"

class Test {
 public:
  Test() { std::cout << "Test()" << std::endl; }

  ~Test() { std::cout << "~Test()" << std::endl; }

  char stack[128];
};

class A : public hd::Coroutine<int> {
 public:
  void run() {
    std::cout << "A::run()" << std::endl;

    for (int i = 0; i < 10; ++i) {
      yield(i);
    }

    std::cout << "A::run() finished" << std::endl;
  }
};

int main(int argc, char* argv[]) {
  int a = 0;
  long i = 0;

  a = i;

  {
    using namespace hd;

    auto ct = coroutine<A>();  // after init, instantly get the first result
    std::cout << *ct << std::endl;

    while (resume(ct.id())) {
      std::cout << *ct << std::endl;
    }

    std::cout << OrdinatorSingleton::get()->size() << std::endl;

    auto ct2 = coroutine<A>();
    while (resume(ct2.id())) {
      std::cout << *ct2 << std::endl;
    }
  }

  std::cout << "out" << std::endl;

  std::cout << hd::OrdinatorSingleton::get()->size() << std::endl;

  return 0;
}