#include "interpolation_base.h"

#include <iostream>
#include <vector>
#include <map>
using namespace std;

namespace irs {

#define OUTPAR(PAR)\
  cout << #PAR << " = " << PAR << endl;

template <class T>
void out_map(T& a_map)
{
  typedef typename T::iterator it_t;
  for (it_t it = a_map.begin(); it != a_map.end(); ++it) {
    cout << "{ " << it->first << ", " << it->second.y << ", " << 
    	it->second.k << ", " << it->second.b << " }, ";
  }
  cout << endl;
}

template <class T>
class line_interp_t : public interpolation_base_t
{
public:
	line_interp_t();
  virtual ~line_interp_t() override;
  virtual void set_points(const T* ap_x_carray, const T* ap_y_carray, size_t a_size) override;
  virtual T operator()(T x) override;

	void add(T a_x, T a_y);
	void clear();
	void prepare();
	void prepare_inv();
	T calc_inv(T x);

private:
	struct point_info_t
	{
	  T y;
	  T k;
	  T b;
	  
	  point_info_t():
	  	y(0),
	  	k(0),
	  	b(0)
	  {}
	};
	/*enum state_t { 
		st_not_enough_points,
		st_enough_points,
		st_ready
	};*/
	enum {
		point_list_size_limit = 2
	};
	
	typedef map<T, point_info_t> point_list_type;
	typedef typename point_list_type::value_type point_type;
	typedef typename point_list_type::iterator plist_it_type;
	
	bool m_ready;
	point_list_type m_point_list;
	bool m_ready_inv;
	point_list_type m_point_list_inv;
	
	point_type make_point(T a_x, T a_y);
	T calc_helper(T x, point_list_type& a_point_list);
	void prepare_helper(point_list_type& a_point_list);
};

template <class T>
line_interp_t<T>::line_interp_t():
	m_ready(false),
	m_point_list(),
	m_ready_inv(false),
	m_point_list_inv()
{
}

template <class T>
line_interp_t<T>::~line_interp_t()
{
}

template <class T>
typename line_interp_t<T>::point_type line_interp_t<T>::
make_point(T a_x, T a_y)
{
	point_info_t pi;
	pi.y = a_y;
	return make_pair(a_x, pi);
}

template <class T>
void line_interp_t<T>::add(T a_x, T a_y)
{
	m_point_list.insert(m_point_list.end(), make_point(a_x, a_y));
	m_ready = false;
	m_ready_inv = false;
}

template <class T>
void line_interp_t<T>::clear()
{
	m_point_list.clear();
	m_ready = false;
	m_point_list_inv.clear();
	m_ready_inv = false;
}

template <class T>
void line_interp_t<T>::set_points(const T* ap_x_carray, const T* ap_y_carray, size_t a_size)
{
  clear();
  const T* p_y_el = ap_y_carray;
  const T* p_x_el_last = ap_x_carray + a_size;
  for (const T* p_x_el = ap_x_carray; p_x_el != p_x_el_last; ++p_x_el) {
    m_point_list.insert(m_point_list.end(), make_point(*p_x_el, *p_y_el));
    ++p_y_el;
  }
  prepare();
}
	
template <class T>
void line_interp_t<T>::prepare_helper(point_list_type& a_point_list)
{
	plist_it_type it = a_point_list.begin();
	T x1 = it->first;
	T y1 = it->second.y;
	++it;
	for (; it != a_point_list.end(); ++it) {
		T x2 = it->first;
		T y2 = it->second.y;
	  it->second.k = (y2 - y1)/(x2 - x1);
	  it->second.b = y1 - it->second.k*x1;
	  x1 = x2;
	  y1 = y2;
	}
}

template <class T>
void line_interp_t<T>::prepare()
{
	if (m_point_list.size() < point_list_size_limit) {
		return;
	}
	
	prepare_helper(m_point_list);
	
	m_ready = true;
}

template <class T>
void line_interp_t<T>::prepare_inv()
{
	if (m_point_list.size() < point_list_size_limit) {
		return;
	}
	
	m_point_list_inv.clear();
	for (plist_it_type it = m_point_list.begin(); it != m_point_list.end(); ++it)
	{
		m_point_list_inv.insert(m_point_list_inv.end(), 
			make_point(it->second.y, it->first));
	}
	
	prepare_helper(m_point_list_inv);

	m_ready_inv = true;
}

template <class T>
T line_interp_t<T>::calc_helper(T x, point_list_type& a_point_list)
{
  pair<plist_it_type, bool> ret = a_point_list.insert(make_point(x, 1.));

  plist_it_type target_it = a_point_list.end();
  plist_it_type x_it = ret.first;
  if (ret.second == true) {
    plist_it_type it_next = x_it;
    ++it_next;
    if (x_it == a_point_list.begin()) {
      target_it = it_next;
      ++target_it;
    } else if (it_next == a_point_list.end()) {
      target_it = x_it;
      --target_it;
    } else {
      target_it = it_next;
    }
    a_point_list.erase(x_it);
  } else {
    if (x_it == a_point_list.begin()) {
      target_it = ++x_it;
    } else {
      target_it = x_it;
    }
  }

  T k = target_it->second.k;
  T b = target_it->second.b;
  return k*x + b;
}

template <class T>
T line_interp_t<T>::operator()(T x)
{
	if (m_point_list.size() < point_list_size_limit) {
		return 0;
	}
	if (!m_ready) {
		prepare();
	}
	
	return calc_helper(x, m_point_list);
}

template <class T>
T line_interp_t<T>::calc_inv(T x)
{
	if (m_point_list.size() < point_list_size_limit) {
		return 0;
	}
	if (!m_ready_inv) {
		prepare_inv();
	}
	if (m_point_list_inv.size() < point_list_size_limit) {
		return 0;
	}
	
	return calc_helper(x, m_point_list_inv);
}

#ifndef ARRAYSIZE
#define ARRAYSIZE(ARRAY) (sizeof(ARRAY)/sizeof(*ARRAY))
#endif //ARRAYSIZE

} //namespace irs
