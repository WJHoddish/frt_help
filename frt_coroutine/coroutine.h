#pragma once

// Coroutine in C++, the main idea is making a ordinator for all coroutines,
// which is based on `ucontext.h` under the Linux environment.

// What we need to do?
// Make a scheduler class (Ordinator). It acts as a pool (ctxPool), which should
// be a singleton.
// A coroutine class (Coroutine), each object of this class implies a coroutine.
// It includes: a "coroutine logic" virtual function; a context of the current
// coroutine.

// Any coroutine requires such 3 tasks in its lifetime.
// 1. first-time activate: The ordinator will assign a stack area for it.
// 2. coroutine suspend: Stack info => buffer.
// 3. coroutine await: reload stack info from buffer to stack.

// Coroutine behaviours:
// activate: init coroutine; init context; add coroutine into pool.
// suspend: stack to buffer, change coroutine status (SUSPEND).
// resume: buffer to stack.

// Coroutine useage:
// Become the base class, which requires such class to override the virtual
// `run()` method.

// -2020/6/5    @jiaheng    Declare both `Coroutine` & `Ordinator` in the same
// header.

#ifndef HUNDUN_COROUTINE_H_
#define HUNDUN_COROUTINE_H_

#include <ucontext.h>

#include <cstring>
#include <iostream>
#include <map>
#include <memory>
#include <type_traits>

#include "co_singleton.h"

#define DEFAULT_STACK_SIZE (1024 * 1024)

namespace hd {

enum class CoResult { OK = 0, FAIL };

/// \brief Coroutine's status.
enum class CoStatus { FREE = 0, RUNNING, SUSPEND };

class Ordinator;

using OrdinatorSingleton = CoSingleton<Ordinator>;

/// \brief Please remember that: all coroutines use stack given by the
/// ordinator (scheduler). Each coroutine's buffer, just as its name implies, is
/// buffer for temporary use.
class CoroutineBase {
  friend class Ordinator;

 public:
  CoroutineBase()
      : size_(0), capacity_(0), state_(CoStatus::FREE), buffer(nullptr) {}

  virtual ~CoroutineBase() {
    if (buffer) {
      delete[] buffer;
      buffer = nullptr;
    }
  }

  /// \brief The entry to any coroutine, enabling...
  virtual void run() = 0;

  bool resume();

  void reload();

  void remain();

 protected:
  // The required members for a particular coroutine which not depend on the
  // environment.
  // The buffer (heap-allocated) is for temporary-use only, where `reload()` and
  // `remain()` support data exchange between this particular coroutine and the
  // singleton ordinator.
  int size_, capacity_;

  CoStatus state_;

  char* buffer;

  // Linux env
  ucontext_t ctx_temp;
};

template <typename T>
class Coroutine : public CoroutineBase {
 public:
  Coroutine() {}

  ~Coroutine() {
    if (arg_) {
      delete arg_;
      arg_ = nullptr;
    }
  }

  T* get() { return arg_; }

 protected:
  void yield(const T& arg);

  T* arg_;
};

using CrtMap = std::map<int, CoroutineBase*>;  // id <-> coroutine

/// \brief Singleton, val members: coroutine pool; main_context; a stack for
/// all coroutines (all coroutines will use this stack area). This stack
/// consumes a large amount of memory on stack.
///
/// functions: init a coroutine; entry a coroutine; check & clean the
/// coroutine pool;
class Ordinator {
 public:
  Ordinator() : id_max_(0) {}

  virtual ~Ordinator() {}

  /// \brief See, only the ordinator can set status as RUNNING, which is a
  /// signal warning .
  static void entry(void* crt) {
    std::cout << "entry()" << std::endl;

    auto p = static_cast<CoroutineBase*>(crt);

    p->state_ = CoStatus::RUNNING;
    p->run();
    p->state_ = CoStatus::FREE;

    std::cout << "entry() finsihed" << std::endl;
  }

  /// \brief The nature of the coroutine pool is an std::map of coroutine's
  /// address.
  int create(CoroutineBase* crt) {
    // assign an id
    id_max_++;
    crt_pool_[id_max_] = crt;  // emplace into the map with a unique id number

    // construct coroutine's context
    getcontext(&(crt->ctx_temp));
    {
      crt->ctx_temp.uc_stack.ss_sp = stack;
      crt->ctx_temp.uc_stack.ss_size = DEFAULT_STACK_SIZE;
      crt->ctx_temp.uc_stack.ss_flags = 0;
      crt->ctx_temp.uc_link = &ctx_main;
    }

    makecontext(&crt->ctx_temp, (void (*)(void))entry, 1, (void*)crt);
    swapcontext(&ctx_main, &crt->ctx_temp);

    std::cout << "create over" << std::endl;

    return id_max_;
  }

  bool resume(int id) {
    auto iter = crt_pool_.find(id);

    return iter != crt_pool_.end() ? iter->second->resume() : false;
  }

  void remove(int id) {
    auto iter = crt_pool_.find(id);

    if (iter != crt_pool_.end()) {
      crt_pool_.erase(iter);
    }
  }

  /// \brief How many coroutines are currently kept.
  int size() {
    int res = 0;

    // traverse all coroutine instance in the pool (std::map)
    for (CrtMap::iterator it = crt_pool_.begin(); it != crt_pool_.end();) {
      if (it->second->state_ == CoStatus::FREE) {
        crt_pool_.erase(it++);
      } else {
        res++;  // non-free coroutine contributes
        it++;
      }
    }

    return res;
  }

  /// \brief Require the bottom of the ordinator stack.
  char* getstack_bottom() { return stack + DEFAULT_STACK_SIZE; }

  // Single ordinator (scheduler), single main context. We shall use no
  // multi-thread but let coroutine simulates one.
  ucontext_t ctx_main;

  char stack[DEFAULT_STACK_SIZE];

 private:
  CrtMap crt_pool_;

  int id_max_;
};

inline bool CoroutineBase::resume() {
  switch (state_) {
    case CoStatus::FREE: {
      return false;
    }
    case CoStatus::RUNNING: {
      throw CoResult::FAIL;
    }
    case CoStatus::SUSPEND: {
      // TODO(jiaheng): To let suspending coroutine reboot, we need to transfer
      // buffer to the ordinator's stack, then switch the main context to the
      // coroutine context.
      reload();
      state_ = CoStatus::RUNNING;

      swapcontext(&(OrdinatorSingleton::get()->ctx_main), &ctx_temp);
      // From now on, resume the coroutine until the next yield.
      // If the next yield is hit, then:
      // 1. value assignment.
      // 2. switch from coroutine context to here (main).
      // Otherwise,
      //
      // This `true` means that "we have already made a new result and you can
      // use it in the next while loop".

      return state_ == CoStatus::SUSPEND ? true : false;
    }
    default: {
      throw CoResult::FAIL;
    }
  }
}

inline void CoroutineBase::reload() {
  memcpy(OrdinatorSingleton::get()->getstack_bottom() - size_, buffer, size_);
}

inline void CoroutineBase::remain() {
  char* stack_bottom = OrdinatorSingleton::get()->getstack_bottom();
  char dummy = 0;

  // if the difference is larger than the capacity_acity, reallocate the buffer.
  if (capacity_ < stack_bottom - &dummy) {
    if (buffer) {
      delete[] buffer;
    }
    capacity_ = stack_bottom - &dummy;
    buffer = new char[capacity_];
  }
  size_ = stack_bottom - &dummy;

  memcpy(buffer, &dummy, size_);
}

/// \brief The nature of "yield" is suspending the coroutine and switching
/// back to the main context.
template <typename T>
inline void Coroutine<T>::yield(const T& arg) {
  if (state_ == CoStatus::RUNNING) {
    // TODO(jiaheng): Hang on the coroutine and switch back to the main context.
    if (!arg_) {
      arg_ = new T();
    }
    *arg_ = arg;

    remain();
    state_ = CoStatus::SUSPEND;

    swapcontext(&ctx_temp, &(OrdinatorSingleton::get()->ctx_main));
  }
}

inline bool resume(int id) { OrdinatorSingleton::get()->resume(id); }

template <typename T>
class CoroutineHandler {
 public:
  CoroutineHandler(int id, std::shared_ptr<T> p) : id_(id), p_(p) {}

  ~CoroutineHandler() { OrdinatorSingleton::get()->remove(id_); }

  int id() const { return id_; }

  auto* operator*() { return p_->get(); }

 private:
  int id_;

  std::shared_ptr<T> p_;
};

template <typename T>
CoroutineHandler<T> coroutine() {
  std::shared_ptr<T> crt(new T());

  // add into the ordinator, being assigned with a unique number
  int id = OrdinatorSingleton::get()->create(crt.get());

  return CoroutineHandler<T>(id, crt);
}

}  // namespace hd

#endif
