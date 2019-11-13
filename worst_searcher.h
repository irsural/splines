#ifndef WORST_SEARCHER_H
#define WORST_SEARCHER_H

#include <limits>
#include <functional>

template<typename T, typename Compare = std::greater<T>>
class peak_searcher_t
{
private:
  T m_current_worst;
  T m_current_worst_index;
  std::size_t m_index;
public:
  peak_searcher_t() :
    m_current_worst(0),
    m_current_worst_index(0),
    m_index(0)
  {}
  void add(T a_value);
  T get();
  std::size_t get_index();
  void clear();
};

template<typename T, typename Compare>
void peak_searcher_t<T, Compare>::add(T a_value)
{
  if (m_index == 0) {
    m_current_worst = a_value;
  } else if (Compare()(a_value, m_current_worst)) {
    m_current_worst =  a_value;
    m_current_worst_index = m_index;
  }
  m_index++;
}

template<typename T, typename Compare>
T peak_searcher_t<T, Compare>::get()
{
  return m_current_worst;
}

template<typename T, typename Compare>
std::size_t peak_searcher_t<T, Compare>::get_index()
{
  return m_current_worst_index;
}

template<typename T, typename Compare>
void peak_searcher_t<T, Compare>::clear()
{
  m_current_worst = 0;
  m_current_worst_index = 0;
  m_index = 0;
}


#endif // WORST_SEARCHER_H
