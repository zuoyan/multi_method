# Open multi method for C++11

This's a C++11 implementation of open multi method, with help of `cxxabi.h`, see
Wikipedia http://en.wikipedia.org/wiki/Multiple_dispatch for the meaning of the
name.

There're some hacks/tricks, it's not portable.

# Reference

For open multi method in C++, you're refered to http://www.stroustrup.com/multimethods.pdf‎

But we're not following that paper.

# Usage

Assume we're implementing a matrix library, for performance, we should
specialized the mat_add routine for different matrix types.

```C++
struct Matrix {
  virtual ~Matrix() {}
};
struct SparseMatrix : Matrix {};
struct DiagonalMatrix : SparseMatrix {};
struct BandedMatrix : SparseMatrix {};

Matrix* mat_add_mm(const Matrix &a, const Matrix &b) {
  // general matrix + general matrix
}
Matrix* mat_add_md(const Matrix &a, const DiagonalMatrix &b) {
  // general matrix + diagonal matrix
}
Matrix* mat_add_dm(const DiagonalMatrix &a, const Matrix &b) {
  // diagonal matrix + general matrix
}
Matrix* mat_add_dd(const DiagonalMatrix &a, const DiagonalMatrix &b) {
  // diagonal matrix + diagonal matrix
}
```

We can implement it like this:

```C++
Matrix* mat_add(const Matrix&a, const Matrix &b) {
  const Diagonal * ad = dynamic_cast<const Diagonal*>(&a);
  const Diagonal * bd = dynamic_cast<const Diagonal*>(&b);
  if (ad) {
    if (bd) {
	  return mat_add_dd(*ad, *bd);
	}
	return mat_add_dm(*ad, b);
  }
  if (bd) {
    return mat_add_md(a, *bd);
  }
  return mat_add_mm(a, b);
}
```

But as the number of matrix types increasing, the mat_add has been changed every
times, and it's maybe very long.

With multi method, we can find the function depending on arguments types, like
virtual method of object, but not just depending on one object.

With this library, we can do it like this:

```C++
multi_method::MultiMethod<2> mm_mat_add;
mm_mat_add.Add<Matrix, Matrix>(mat_add_mm);
mm_mat_add.Add<Matrix, DiagonalMatrix>(mat_add_md);
mm_mat_add.Add<DiagonalMatrix, Matrix>(mat_add_dm);
mm_mat_add.Add<DiagonalMatrix, DiagonalMatrix>(mat_add_dd);

std::unique_ptr<Matrix> mat_add(const Matrix &a, const Matrix &b) {
   std::array<void*, 2> ptrs;
   auto fp = mm_mat_add.Find(ptrs, a, b);
   auto func = reinterpret_cast<Matrix* (*)(void*, void*)>(fp);
   return std::unique_ptr<Matrix>{func(ptrs[0], ptrs[1])};
}

const Matrix *m, *d, *b; // m is general, d is diagnonal, b is banded
mat_add(*m, *d); // find and call mat_add_md
mat_add(*d, *m); // find and call mat_add_dm
mat_add(*d, *d); // find and call mat_add_dd
mat_add(*b, *d); // call mat_add_md here
// We should not add BandedMatrix specializations after the last call.
```
