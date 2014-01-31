#ifndef FILE_721B30FE_9DDC_42C9_8123_B481207097FA_H
#define FILE_721B30FE_9DDC_42C9_8123_B481207097FA_H
#include <typeinfo>
#include <cstdint>
#include <cassert>
#include <cxxabi.h>

namespace multi_method {

struct Bases {
  const std::type_info * type_;
  const void * si_type_;
  const void * vmi_type_;

  inline Bases(const std::type_info *type)
      : type_(type) {
    si_type_ = static_cast<const void*>(dynamic_cast<const abi::__si_class_type_info*>(type_));
    vmi_type_ = static_cast<const void*>(dynamic_cast<const abi::__vmi_class_type_info*>(type_));
  }

  int size() const {
    if (si_type_) {
      return 1;
    }
    if (vmi_type_) {
      auto vmi = static_cast<const abi::__vmi_class_type_info*>(vmi_type_);
      return vmi->__base_count;
    }
    return 0;
  }

  bool is_virtual_at(int at) const {
    if (si_type_) {
      return false;
    }
    auto vmi = static_cast<const abi::__vmi_class_type_info*>(vmi_type_);
    return vmi->__base_info[at].__is_virtual_p();
  }

  bool is_public_at(int at) const {
    if (si_type_) {
      return true;
    }
    auto vmi = static_cast<const abi::__vmi_class_type_info*>(vmi_type_);
    return vmi->__base_info[at].__is_public_p();
  }

  const std::type_info* base_at(int at) const {
    if (si_type_) {
      auto si = static_cast<const abi::__si_class_type_info*>(si_type_);
      return si->__base_type;
    }
    auto vmi = static_cast<const abi::__vmi_class_type_info*>(vmi_type_);
    return vmi->__base_info[at].__base_type;
  }

  ptrdiff_t offset_at(int at) const {
    if (si_type_) {
      return 0;
    }
    auto vmi = static_cast<const abi::__vmi_class_type_info*>(vmi_type_);
    return vmi->__base_info[at].__offset();
  }

  ptrdiff_t offset_at(int at, const void *obj) const {
    auto o = offset_at(at);
    if (is_virtual_at(at)) {
      const void *vtable = *static_cast<const void *const*>(obj);
      o = *(ptrdiff_t*)((char*)vtable + o);
    }
    return o;
  }

  template <class F>
  bool find(const F &func) {
    for (int i = 0; i < size(); ++i) {
      if (!is_public_at(i)) continue;
      bool r = func(base_at(i));
      if (r) return true;
    }
    return false;
  }

  template <class F>
  bool find_recursive(const F &func) const {
    if (func(type_)) return true;
    for (int i = 0; i < size(); ++i) {
      if (!is_public_at(i)) continue;
      auto b = base_at(i);
      if (func(b)) return true;
      bool r = Bases(b).find_recursive(func);
      if (r) return true;
    }
    return false;
  }

  bool contains(const void *obj,
                const std::type_info *srctype,
                const void *srcptr) const {
    if (type_ == srctype && obj == srcptr) return true;
    if ((char*)srcptr - (char*)obj < 0) return false;
    for (int i = 0, c = size(); i < c; ++i) {
      if (!is_public_at(i)) continue;
      auto b = base_at(i);
      auto o = offset_at(i, obj);
      bool r = Bases(b).contains((char*)obj + o, srctype, srcptr);
      if (r) return true;
    }
    return false;
  }

  template <class Func>
  bool upcast_recursive_check(const Func &func,
                             const void *obj,
                             const std::type_info *srctype = nullptr,
                              const void *srcptr = nullptr) const {
    assert(type_);
    if (srctype == type_ && obj == srcptr) {
      srctype = nullptr;
      srcptr = nullptr;
    }
    if (srcptr && (char*)srcptr - (char*)obj < 0) return false;
    if (!srctype || contains(obj, srctype, srcptr)) {
      if (func(type_, obj)) return true;
    } else {
      return false;
    }
    for (int i = 0, c = size(); i < c; ++i) {
      if (!is_public_at(i)) continue;
      auto b = base_at(i);
      auto o = offset_at(i, obj);
      bool r = Bases(b).upcast_recursive_check(
          func, (char*)obj + o, srctype, srcptr);
      if (r) return true;
    }
    return false;
  }

  void* upcast(const void *obj,
               const std::type_info *dest,
               const std::type_info *srctype = nullptr,
               const void *srcptr = nullptr) const {
    const void *ret = nullptr;
    upcast_recursive_check(
        [&](const std::type_info *b, const void *o) {
          if (b == dest) {
            ret = o;
            return true;
          }
          return false;
        }, obj, srctype, srcptr);
    return (void*)ret;
  }
};

template <class T>
inline void *get_whole(const T *ptr, const std::type_info *whole_type) {
  if (whole_type != &typeid(T)) {
    intptr_t offset = *(*(intptr_t**)ptr - 2);
    void *whole = (char*)ptr + offset;
    assert(*(*(void***)whole - 1) == whole_type);
    return whole;
  }
  return static_cast<void*>(const_cast<T*>(ptr));
}

template <class T>
inline void *get_whole(const T *ptr) {
  return get_whole(ptr, &typeid(*ptr));
}

}  // namespace multi_method
#endif // FILE_721B30FE_9DDC_42C9_8123_B481207097FA_H
