#ifndef FILE_820D4736_731E_4394_83D4_787BEFACE734_H
#define FILE_820D4736_731E_4394_83D4_787BEFACE734_H

#include "multi_method/bases.h"
#include "multi_method/table.h"

#include <vector>
#include <string>
#include <algorithm>

namespace multi_method {

struct TypePartial {
  const std::type_info *type_;

  TypePartial() : type_(nullptr) {}

  TypePartial(const std::type_info* type) : type_(type) {}
  TypePartial(const TypePartial &) = default;

  bool operator==(const TypePartial & o) const {
    return type_ == o.type_;
  }

  bool operator!=(const TypePartial & o) const {
    return type_ != o.type_;
  }

  bool operator>(const TypePartial & o) const {
    if (type_ == o.type_) return false;
    Bases tbs(type_);
    for (int i = 0; i < tbs.size(); ++i) {
      auto t = tbs.base_at(i);
      TypePartial tp(t);
      if (tp >= o) return true;
    }
    return false;
  }

  bool operator<(const TypePartial & o) const {
    return o > (*this);
  }

  bool operator>=(const TypePartial & o) const {
    if (type_ == o.type_) return true;
    Bases tbs(type_);
    for (int i = 0; i < tbs.size(); ++i) {
      auto t = tbs.base_at(i);
      TypePartial tp(t);
      if (tp >= o) return true;
    }
    return false;
  }

  bool operator<=(const TypePartial & o) const {
    return o >= (*this);
  }
};

template <class P, int N>
struct PartialArray {
  std::array<P, N> values_;

  PartialArray(const PartialArray &) = default;
  PartialArray(PartialArray &) = default;

  template <class ...U>
  PartialArray(const U& ...u) : values_{{P{u}...}} {}

  const P& operator[](int x) const {
    return values_[x];
  }

  P& operator[](int x) {
    return values_[x];
  }

  bool operator==(const PartialArray & o) const {
    for (int i = 0; i < N; ++i) {
      if (values_[i] != o.values_[i]) return false;
    }
    return true;
  }

  bool operator!=(const PartialArray & o) const {
    return !(*this == o);
  }

  bool operator>(const PartialArray & o) const {
    for (int i = 0; i < N; ++i) {
      if (!(values_[i] >= o.values_[i])) return false;
    }
    return *this != o;
  }

  bool operator<(const PartialArray & o) const {
    return o > (*this);
  }

  bool operator<=(const PartialArray & o) const {
    for (int i = 0; i < N; ++i) {
      if (!(values_[i] <= o.values_[i])) return false;
    }
    return true;
  }

  bool operator>=(const PartialArray & o) const {
    return o <= (*this);
  }
};

template <int N>
using TypePartialArray = PartialArray<TypePartial, N>;

template <int N>
struct TableHash<TypePartialArray<N>> {
  inline size_t operator()(const TypePartialArray<N> &v) {
    size_t h = 0;
    for (int i = 0; i < N; ++i) {
      h += reinterpret_cast<intptr_t>(v.values_[i].type_);
      h ^= (reinterpret_cast<intptr_t>(v.values_[i].type_) >> 4);
    }
    return h;
  }
};

template <int N>
std::string to_str(const TypePartialArray<N> &a) {
  std::string ret = "(";
  for (int i = 0; i < N; ++i) {
    if (i > 0) ret += ", ";
    if (a[i].type_) {
      ret += a[i].type_->name();
    } else {
      ret += "null";
    }
  }
  return ret + ")";
}

template <int N, int P>
struct upcast_recursive_check_impl {
  template <class Func>
  bool operator()(
    const Func &func,
    const TypePartialArray<N> &real,
    const std::array<void*, N> &objs) {
    upcast_recursive_check_impl<N, P-1> pre;
    // std::cerr << "P=" << P << " upcast real=" << to_str(real) << std::endl;
    return pre([&](
        TypePartialArray<N> base, std::array<void*, N> ptrs) {
                 return Bases(real[P - 1].type_).upcast_recursive_check(
                     [&](const std::type_info *b, const void* p) {
                       assert(b);
                       // std::cerr << "upcast recursive for " << b->name() << "\n";
                       base[P - 1] = b;
                       ptrs[P - 1] = (void*)p;
                       return func(base, ptrs);
                     },
                     objs[P - 1]);
               },
               real, objs);
  }
};

template <int N>
struct upcast_recursive_check_impl<N, 0> {
  template <class Func>
  bool operator()(
    const Func &func,
    const TypePartialArray<N> &real,
    const std::array<void*, N> &objs) {
    TypePartialArray<N> base;
    std::array<void*, N> ptrs;
    return func(base, ptrs);
  }
};

template <int N, class Func>
bool upcast_recursive_check(
    const Func &func,
    const TypePartialArray<N> &real,
    const std::array<void*, N> &objs) {
  upcast_recursive_check_impl<N, N> impl;
  return impl(func, real, objs);
}

}  // namespace multi_method
#endif // FILE_820D4736_731E_4394_83D4_787BEFACE734_H
