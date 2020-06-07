#pragma once

#ifndef HUNDUN_CO_SINGLETON_H_
#define HUNDUN_CO_SINGLETON_H_

namespace hd {
/// \brief Base class, making any other class behaves like a singleton.
template <class T>
class CoSingleton {
 public:
  /// \brief Init T() only if the `get` method is called.
  static T* get() {
    if (!instance_) {
      instance_ = new CoSingleton<T>();
    }

    return &instance_->data_;
  }

 protected:
  /// \brief Constructor, which is not allowed to be called freely in context.
  CoSingleton() : data_() {}

 private:
  static CoSingleton<T>* instance_;

  T data_;
};

// init static class member in header file, interesting.
template <class T>
CoSingleton<T>* CoSingleton<T>::instance_ = nullptr;

}  // namespace hd

#endif
