#ifndef FILE_0D5D7553_9F5D_484A_B873_F599D81EFA9D_H
#define FILE_0D5D7553_9F5D_484A_B873_F599D81EFA9D_H
// Rule: Any time we find a resolution, it's fixed.
//
// Consideration: We can disambuiguous according declartions, not just real
// types, but it costs.
//
// Usage:
//   struct Matrix {
//     virtual ~Matrix() {}
//   };
//   struct SparseMatrix : Matrix {};
//   struct DiagonalMatrix : SparseMatrix {};
//   struct BandedMatrix : SparseMatrix {};
//
//   Matrix* mat_add_mm(const Matrix &a, const Matrix &b) {
//     // general matrix + general matrix
//   }
//   Matrix* mat_add_md(const Matrix &a, const DiagonalMatrix &b) {
//     // general matrix + diagonal matrix
//   }
//   Matrix* mat_add_dm(const DiagonalMatrix &a, const Matrix &b) {
//     // diagonal matrix + general matrix
//   }
//   Matrix* mat_add_dd(const DiagonalMatrix &a, const DiagonalMatrix &b) {
//     // diagonal matrix + diagonal matrix
//   }
//  multi_method::MultiMethod<2> mm_mat_add;
//  mm_mat_add.Add<Matrix, Matrix>(mat_add_mm);
//  mm_mat_add.Add<Matrix, DiagonalMatrix>(mat_add_md);
//  mm_mat_add.Add<DiagonalMatrix, Matrix>(mat_add_dm);
//  mm_mat_add.Add<DiagonalMatrix, DiagonalMatrix>(mat_add_dd);
//
//  std::unique_ptr<Matrix> mat_add(const Matrix &a, const Matrix &b) {
//     std::array<void*, 2> ptrs;
//     auto fp = mm_mat_add.Find(ptrs, a, b);
//     auto func = reinterpret_cast<Matrix* (*)(void*, void*)>(fp);
//     return std::unique_ptr<Matrix>{func(ptrs[0], ptrs[1])};
//  }
//
//  Matrix m; DiagonalMatrix d; BandedMatrix b;
//  mat_add(m, (Matrix&)d); // find can find and call mat_add_md
//  mat_add((Matrix&)d, m); // find can find and call mat_add_dm
//  mat_add((Matrix&)d, (Matrix&)d); // find can find and call mat_add_dd
//  // but banded is not specialize before, so
//  mat_add((Matrix&)b, (Matrix&)d); // we'll call mat_add_mm here

#include "multi_method/partial.h"

namespace multi_method {

typedef void (*void_func)(void);

template <int N>
struct MultiMethod {
  typedef TypePartialArray<N> partial;
  typedef std::array<ptrdiff_t, N> offsets_type;

  struct ResolvedMethod {
    partial pos;
    void_func func;
    offsets_type offsets;
  };

  Table<partial, void_func> table_;
  Table<partial, ResolvedMethod> resolved_;

  template <class Func>
  int Add(const partial &p, Func func) {
    void_func vf = reinterpret_cast<void_func>(func);
    table_.Add(p, vf);
    return 1;
  }

  template <class ...U, class Func>
  int Add(Func func) {
    void_func vf = reinterpret_cast<void_func>(func);
    table_.Add({TypePartial{&typeid(U)}...}, vf);
    return 1;
  }

  void_func Find(const partial &real,
                 const std::array<void*, N> &objs,
                 std::array<void*, N> &func_ptrs) {
    auto m = resolved_.Find(real);
    if (m.func) {
      for (int i = 0; i < N; ++i) {
        func_ptrs[i] = (char*)objs[i] + m.offsets[i];
      }
      return m.func;
    }
    std::vector<ResolvedMethod> funcs;
    upcast_recursive_check<N>(
        [&](const partial &b, const std::array<void*, N> &ptrs) {
          auto func = table_.Find(b);
          if (!func) return false;
          auto it = std::find_if(funcs.begin(), funcs.end(),
                                 [&](const ResolvedMethod &m) {
                                   return m.pos >= b;
                                 });
          if (it != funcs.end()) return false;
          it = std::remove_if(funcs.begin(), funcs.end(),
                              [&](const ResolvedMethod &m) {
                                return m.pos <= b;
                              });
          funcs.resize(it - funcs.begin());
          std::array<ptrdiff_t, N> offsets;
          for (int i = 0; i < N; ++i) {
            offsets[i] = (char*)ptrs[i] - (char*)objs[i];
          }
          funcs.push_back({b, func, offsets});
          return false;
        },
        real, objs);
    if (funcs.size() == 1) {
      auto &m = funcs.front();
      resolved_.Add(real, m);
      return Find(real, objs, func_ptrs);
    }
    abort();
    return nullptr;
  }

  template <int X>
  typename std::enable_if<X==N>::type
  inline init_find(
      partial &real, std::array<void*, N> &objs) {}

  template <int X, class H, class ...U>
  typename std::enable_if<(X < N)>::type
  inline init_find(partial &real, std::array<void*, N> &objs,
            const H & h, const U &  ...rest) {
    objs[X] = get_whole(&h, real[X].type_);
    this->init_find<X + 1>(real, objs, rest...);
  }

  template <class ...U>
  inline void_func Find(
      std::array<void*, N>&func_ptrs, const U& ...v) {
    partial real{&typeid(v)...};
    std::array<void*, N> objs;
    this->init_find<0>(real, objs, v...);
    return Find(real, objs, func_ptrs);
  }
};

}  // namespace multi_method
#endif // FILE_0D5D7553_9F5D_484A_B873_F599D81EFA9D_H
