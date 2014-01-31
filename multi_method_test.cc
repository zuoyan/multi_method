#include "multi_method/multi_method.h"

#include <iostream>
#include <memory>

namespace mm = multi_method;

mm::MultiMethod<1> mm_show;

template <class T>
void show(const T& v) {
  std::array<void*, 1> ptr;
  auto fp = mm_show.Find(ptr, v);
  auto func = reinterpret_cast<void (*)(void*)>(fp);
  func(ptr[0]);
}

template <class T>
void show_static(const T &v) {
  std::cerr << "show type T=" << typeid(T).name()
            << " value=" << v.value
            << " real=" << typeid(v).name() << std::endl;
}

struct V {
  virtual ~V() {}
  int value = __LINE__;
};

struct B : virtual V {
  int value = __LINE__;
};

struct C : virtual V {
  int value = __LINE__;
};

struct D : virtual B, virtual C {
  int value = __LINE__;
};

mm::MultiMethod<1> mm_bench;

template <class T>
void bench(const T& v) {
  std::array<void*, 1> ptr;
  auto fp = mm_bench.Find(ptr, v);
  auto func = reinterpret_cast<void (*)(void*)>(fp);
  func(ptr[0]);
}

template <class T>
void bench_static(const T &v) { }

template <class Func>
void test_func(const std::string & name, const Func & func, int N=1.e8) {
  auto start = std::chrono::steady_clock::now();
  for (int i = 0; i < N; ++i) {
    func(i);
  }
  auto end = std::chrono::steady_clock::now();
  std::chrono::duration<double, std::nano> elapsed = end - start;
  std::cerr << name << ": " << elapsed.count() / N << " nano-seconds/call\n";
  std::cerr << name << ": " << N * 1.e9 / elapsed.count() << " #/second\n";
}

mm::MultiMethod<2> mm_add;

template <class A, class B>
int add(const A& a, const B & b) {
  std::array<void*, 2> ptr;
  auto fp = mm_add.Find(ptr, a, b);
  auto func = reinterpret_cast<int(*)(void*, void*)>(fp);
  return func(ptr[0], ptr[1]);
}

template <class A, class B>
int add_static(const A &a, const B & b) {
  return a.value + b.value;
}

mm::MultiMethod<2> mm_mat_add;

struct Matrix {
  virtual ~Matrix() {}
};

struct Diagonal : virtual Matrix {};

Matrix* mat_add_mm(const Matrix &, const Matrix &) {
  std::cerr << "Matrix + Matrix\n";
  return new Matrix();
}

Matrix* mat_add_dm(const Diagonal &, const Matrix &) {
  std::cerr << "Diagonal + Matrix\n";
  return new Matrix();
}

Matrix* mat_add_md(const Matrix &, const Diagonal &) {
  std::cerr << "Matrix + Diagonal\n";
  return new Matrix();
}

Matrix* mat_add_dd(const Diagonal &, const Diagonal &) {
  std::cerr << "Diagonal + Diagonal\n";
  return new Diagonal();
}

static int mm_mat_init__ = [&]() {
  mm_mat_add.Add<Matrix, Matrix>(&mat_add_mm);
  mm_mat_add.Add<Matrix, Diagonal>(&mat_add_md);
  mm_mat_add.Add<Diagonal, Matrix>(&mat_add_dm);
  mm_mat_add.Add<Diagonal, Diagonal>(&mat_add_dd);
  return 0;
}();

template <class A, class B>
std::unique_ptr<Matrix> mat_add(const A &a, const B &b) {
  std::array<void*, 2> ptrs;
  auto fp = mm_mat_add.Find(ptrs, a, b);
  auto func = reinterpret_cast<Matrix* (*)(void*, void*)>(fp);
  return std::unique_ptr<Matrix>{func(ptrs[0], ptrs[1])};
}

int main(int argc, char* argv[]) {
  {
    Matrix m; Diagonal d;
    mat_add((Matrix&)m, (Matrix&)m);
    mat_add((Matrix&)m, (Matrix&)d);
    mat_add((Matrix&)d, (Matrix&)m);
    mat_add((Matrix&)d, (Matrix&)d);
  }

  mm_show.Add<V>(show_static<V>);
  mm_show.Add<B>(show_static<B>);
  // mm_show.Add<C>(show_static<C>);

  show(V{});
  show(D{});

  mm_add.Add<V, V>(add_static<V, V>);
  mm_add.Add<B, B>(add_static<B, B>);
  mm_add.Add<B, C>(add_static<B, C>);
  mm_add.Add<C, B>(add_static<C, B>);
  mm_add.Add<C, C>(add_static<C, C>);

  mm_add.Add<D, V>(add_static<D, V>);
  mm_add.Add<D, B>(add_static<D, B>);
  mm_add.Add<D, C>(add_static<D, C>);
  mm_add.Add<D, D>(add_static<D, D>);

  mm_add.Add<V, D>(add_static<V, D>);
  mm_add.Add<B, D>(add_static<B, D>);
  mm_add.Add<C, D>(add_static<C, D>);

  std::cerr << "V+V = " << add(V{}, V{}) << std::endl;
  std::cerr << "B+V = " << add(B{}, V{}) << std::endl;
  std::cerr << "V+B = " << add(V{}, B{}) << std::endl;
  std::cerr << "B+B = " << add(B{}, B{}) << std::endl;

  mm_bench.Add({&typeid(V)}, bench_static<V>);
  mm_bench.Add({&typeid(B)}, bench_static<B>);

  {
    V v;
    test_func("bench V",
              [&](int i) {
                bench(v);
              });
  }

  {
    B v;
    test_func("bench B",
              [&](int i) {
                bench(v);
              });
  }

  {
    D v;
    test_func("bench D",
              [&](int i) {
                bench(v);
              });
  }

  {
    D d;
    B b;
    test_func("add D + B",
              [&](int i) {
                return add(d, b);
              });
  }
}
