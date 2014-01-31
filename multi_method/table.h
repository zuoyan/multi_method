#ifndef FILE_5BC057F4_76D1_4834_81DF_7DED6F0EDB9E_H
#define FILE_5BC057F4_76D1_4834_81DF_7DED6F0EDB9E_H
#include <functional>
#include <atomic>
#include <mutex>

namespace multi_method {

template <class T>
struct ValueFree {
  inline void operator()(const T & v) const {}
};

template <>
struct ValueFree<void(*)(void)> {
  inline void operator()(void (*v)(void)) const {  }
};

template <class T>
struct ValueFree<T*> {
  inline void operator()(T * v) const {
    delete v;
  }
};

template <class T>
struct TableHash : std::hash<T> {};

template <class T>
struct TableHash<T *> {
  inline size_t operator()(const T*ptr) const {
    return (intptr_t)(ptr) >> 4;
  }
};

template <class Key, class Value, class Hash=TableHash<Key>,
          class DeleteValue=ValueFree<Value>>
struct Table {
  struct State {
    State * old;
    size_t size;
    size_t buckets;
    std::pair<Key, Value> table[0];
  };
  std::atomic<State *> state_;
  std::mutex add_mutex_;

  Table() {
    const size_t buckets = 8;
    auto state = (State*)calloc(sizeof(State) + sizeof(state_.load()->table[0]) * buckets, 1);
    state->buckets = buckets;
    state_.store(state);
  }

  ~Table() {
    DeleteValue delete_value;
    auto state = state_.load();
    for (size_t i = 0; i < state->buckets; ++i) {
      auto &kv = state->table[i];
      if (kv.first != Key()) {
        delete_value(kv.second);
      }
    }
    while (state) {
      auto old = state->old;
      free(state);
      state = old;
    }
  }

  template <class F>
  void foreach_check(const F & func) {
    auto s = state_.load();
    for (size_t b = 0; b < s->buckets; ++b) {
      auto &o = s->table[b];
      if (o.first == Key()) continue;
      if (!func(o.first, o.second)) break;
    }
  }

  template <class F>
  void foreach_check(const F & func) const {
    const_cast<Table*>(this)->foreach_check(std::forward<F>(func));
  }

  template <class F>
  void foreach(const F & func) {
    auto s = state_.load();
    for (size_t b = 0; b < s->buckets; ++b) {
      auto &o = s->table[b];
      if (o.first == Key()) continue;
      func(o.first, o.second);
    }
  }

  template <class F>
  void foreach(const F & func) const {
    const_cast<Table*>(this)->foreach(std::forward<F>(func));
  }

  Value Find(const Key &k) {
    auto state = state_.load(std::memory_order_relaxed);
    size_t b = Hash()(k) & (state->buckets - 1);
    size_t idx = 0;
    while (1) {
      auto &o = state->table[b];
      if (o.first == k) {
        return o.second;
      }
      if (o.first == Key()) break;
      b = (b + ++idx) & (state->buckets - 1);
    }
    return Value();
  }

  void Resize(int dir) {
    assert(dir == 1);
    auto old = state_.load(std::memory_order_acquire);
    size_t buckets = old->buckets * 2;
    State *state = (State*)malloc(sizeof(State) + sizeof(old->table[0]) * buckets);
    state->old = old;
    state->size = old->size;
    state->buckets = buckets;
    for (size_t i = 0; i < buckets; ++i) {
      state->table[i].first = Key();
    }
    for (size_t i = 0; i < old->buckets; ++i) {
      auto &o = old->table[i];
      if (o.first == Key()) continue;
      size_t b = Hash()(o.first) & (state->buckets - 1);
      size_t idx = 0;
      while (state->table[b].first != Key()) {
        b = (b + ++idx) & (state->buckets - 1);
      }
      state->table[b] = o;
    }
    state_.store(state, std::memory_order_release);
  }

  std::pair<Value&, bool> Add(const Key &k, const Value &value) {
    std::lock_guard<std::mutex> lk(add_mutex_);
    auto state = state_.load(std::memory_order_acquire);
    if (state->size * 5 >= 4 * state->buckets) {
      Resize(1);
      state = state_.load(std::memory_order_relaxed);
    }
    size_t b = Hash()(k) & (state->buckets - 1);
    size_t idx = 0;
    while (1) {
      auto &o = state->table[b];
      if (o.first == k) {
        return std::pair<Value&, bool>{o.second, false};
      }
      if (o.first == Key()) {
        o.second = value;
        o.first = k;
        ++state->size;
        return std::pair<Value&, bool>{o.second, true};
      }
      b = (b + ++idx) & (state->buckets - 1);
    }
  }
};

}  // namespace multi_method
#endif // FILE_5BC057F4_76D1_4834_81DF_7DED6F0EDB9E_H
