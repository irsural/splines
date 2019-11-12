#ifndef INTERPOLATION_BASE_H
#define INTERPOLATION_BASE_H

#include <cstdlib>

class interpolation_base_t
{
public:
  virtual ~interpolation_base_t() {}
  virtual void set_points(const double* a_x, const double* a_y, size_t a_length) = 0;
  virtual double operator()(double a_x) = 0;
};


#endif // INTERPOLATION_BASE_H
