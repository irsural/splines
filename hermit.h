#ifndef HERMIT_H
#define HERMIT_H

#include <vector>
#include <cassert>
using namespace std;



//Piecewise Cubic Hermite Interpolating Polynomial
template <class T>
class pchip_t
{
public:
    ~pchip_t();
    pchip_t();

    void set_points(T* a_x, T* a_y, int a_length);
    T operator()(T a_x);
private:
    int m_nodes_count;
    vector<T> m_x;
    vector<T> m_y;
    vector<T> m_derivatives;

    vector<T> m_c2;
    vector<T> m_c3;

    void spline_pchip_set();
    T get_multi_sign (T a_a, T b_b);
};

template <class T>
pchip_t<T>::pchip_t() :
  m_nodes_count(0),
  m_x(),
  m_y(),
  m_derivatives(),
  m_c2(),
  m_c3()
{

}

template <class T>
pchip_t<T>::~pchip_t()
{
}

template <class T>
void pchip_t<T>::set_points(T* a_x, T* a_y, int a_length)
{
  assert(a_length >= 2);
  for (int i = 1; i < a_length; i++) {
    assert(a_x[i] >= a_x[i - 1]);
  }

  if (m_nodes_count != a_length) {
    m_nodes_count = a_length;
    m_x.resize(m_nodes_count);
    m_y.resize(m_nodes_count);
    m_derivatives.resize(m_nodes_count);
    m_c2.resize(m_nodes_count);
    m_c3.resize(m_nodes_count);
  }

  std::copy(a_x, a_x + a_length, m_x.begin());
  std::copy(a_y, a_y + a_length, m_y.begin());

  spline_pchip_set();
}

template <class T>
T pchip_t<T>::get_multi_sign (T a_a, T a_b)
{
  //Вычисляет знак произведения 2х чисел без
  //умножения, чтобы избежать переполнения
  if (a_a == 0. || a_b == 0.) {
    return 0;
  } else if ((a_a < 0. && a_b < 0.) || (a_a > 0. && a_b > 0.)) {
    return 1;
  } else {
    return -1;
  }
}

template <class T>
void pchip_t<T>::spline_pchip_set()
{
  int nless1 = m_nodes_count - 1;
  T h1 = m_x[1] - m_x[0];
  T del1 = ( m_y[1] - m_y[0] ) / h1;
  T dsave = del1;

//  Special case N=2, use linear interpolation.
  if ( m_nodes_count == 2 ) {
    m_derivatives[0] = del1;
    m_derivatives[m_nodes_count - 1] = del1;
  } else {
//  Normal case, N >= 3
    T h2 = m_x[2] - m_x[1];
    T del2 = ( m_y[2] - m_y[1] ) / h2;

//  Set D(1) via non-centered three point formula, adjusted to be
//  shape preserving.
    T hsum = h1 + h2;
    T w1 = ( h1 + hsum ) / hsum;
    T w2 = -h1 / hsum;
    m_derivatives[0] = w1 * del1 + w2 * del2;

    if (get_multi_sign ( m_derivatives[0], del1 ) <= 0.0) {
      m_derivatives[0] = 0.0;
    } else if ( get_multi_sign ( del1, del2 ) < 0.0 ) {
//    Need do this check only if monotonicity switches
      T dmax = 3.0 * del1;

      if ( fabs ( dmax ) < fabs ( m_derivatives[0] ) ) {
        m_derivatives[0] = dmax;
      }
    }

    T dmax;
    T dmin;
//  Loop through interior points.
    for (int i = 2; i <= nless1; i++ ) {
      if ( 2 < i ) {
        h1 = h2;
        h2 = m_x[i] - m_x[i-1];
        hsum = h1 + h2;
        del1 = del2;
        del2 = ( m_y[i] - m_y[i-1] ) / h2;
      }
//    Set D(I)=0 unless data are strictly monotonic
      m_derivatives[i-1] = 0.0;
      T temp = get_multi_sign ( del1, del2 );

      if ( temp < 0.0 ) {
        dsave = del2;
      } else if ( temp == 0.0 ) {
//      Count number of changes in direction of monotonicity
        if ( del2 != 0.0 ) {
          if ( get_multi_sign ( dsave, del2 ) < 0.0 ) {
          }
          dsave = del2;
        }
      } else {
//      Use Brodlie modification of Butland formula
        T hsumt3 = 3.0 * hsum;
        w1 = ( hsum + h1 ) / hsumt3;
        w2 = ( hsum + h2 ) / hsumt3;
        dmax = std::max ( fabs ( del1 ), fabs ( del2 ) );
        dmin = std::min ( fabs ( del1 ), fabs ( del2 ) );
        T drat1 = del1 / dmax;
        T drat2 = del2 / dmax;
        m_derivatives[i-1] = dmin / ( w1 * drat1 + w2 * drat2 );
      }
    }

//  Set D(N) via non-centered three point formula, adjusted to be
//  shape preserving
    w1 = -h2 / hsum;
    w2 = ( h2 + hsum ) / hsum;
    m_derivatives[m_nodes_count - 1] = w1 * del1 + w2 * del2;

    if ( get_multi_sign ( m_derivatives[m_nodes_count - 1], del2 ) <= 0.0 ) {
      m_derivatives[m_nodes_count - 1] = 0.0;
    } else if ( get_multi_sign ( del1, del2 ) < 0.0 ) {
//    Need do this check only if monotonicity switches
      dmax = 3.0 * del2;
      if ( fabs ( dmax ) < fabs (m_derivatives[m_nodes_count - 1])) {
        m_derivatives[m_nodes_count - 1] = dmax;
      }
    }
  }
  for (int i = 0; i < m_nodes_count - 1; i++) {
//  Коэффициенты для рассчета полинома
    T h = m_x[i+1] - m_x[i];
    T delta = (m_y[i+1] - m_y[i]) / h;
    T delta1 = (m_derivatives[i] - delta) / h;
    T delta2 = (m_derivatives[i+1]  - delta) / h;
    m_c2[i] = -(delta1 + delta1 + delta2);
    m_c3[i] = (delta1 + delta2) / h;
  }
}

template <class T>
T pchip_t<T>::operator()(T a_x)
{
  assert(m_nodes_count);

  int interval_num = -1;
  if (a_x <= m_x[0]) {
    interval_num = 0;
  } else if (a_x >= m_x[m_nodes_count - 1]) {
    interval_num = m_nodes_count - 2;
  } else {
    for (int i = 1; i < m_nodes_count; i++) {
      if (m_x[i] > a_x) {
        interval_num = i - 1;
        break;
      }
    }
  }
  assert(interval_num >= 0);

  T x = a_x - m_x[interval_num];
  T y = m_y[interval_num] + x * (m_derivatives[interval_num] +
    x * (m_c2[interval_num] + x * m_c3[interval_num]));
  return y;
}


#endif // HERMIT_H
