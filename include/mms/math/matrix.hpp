#pragma once

#include <vector>

namespace math
{
template <typename Type = double> class SimpleMatrix
{
  private:
    using Matrix = std::vector<std::vector<Type>>;
    Matrix _mat;

  public:
    SimpleMatrix();
    SimpleMatrix(const int &rows, const int &cols, const Type &val);
    SimpleMatrix(const SimpleMatrix<Type> &other);
    SimpleMatrix operator=(const SimpleMatrix<Type> &rhs);
    virtual ~SimpleMatrix();

    Matrix get_mat() const;
    Type &value(const int &row, const int &col);
};

template <typename Type> inline SimpleMatrix<Type>::SimpleMatrix()
{
}

template <typename Type> inline SimpleMatrix<Type>::SimpleMatrix(const int &rows, const int &cols, const Type &val)
{
    std::vector<Type> col(cols, val);
    for (int i = 0; i < rows; ++i)
    {
        _mat.push_back(col);
    }
}

template <typename Type> inline SimpleMatrix<Type>::SimpleMatrix(const SimpleMatrix<Type> &other)
{
    mat = other.get_mat();
}

template <typename Type> inline SimpleMatrix<Type> SimpleMatrix<Type>::operator=(const SimpleMatrix<Type> &rhs)
{
    _mat = rhs._mat;
    return *this;
}

template <typename Type> inline SimpleMatrix<Type>::Matrix SimpleMatrix<Type>::get_mat() const
{
    return _mat;
}

template <typename Type> inline Type &SimpleMatrix<Type>::value(const int &row, const int &col)
{
    return _mat[row][col];
}

} // namespace math
