#ifndef WORST_SEARCHER_H
#define WORST_SEARCHER_H

#include <limits>

template<typename T>
class worst_searcher_t
{
private:
  T m_current_worst;
  T m_current_worst_index;
  std::size_t m_index;
public:
  worst_searcher_t() :
    m_current_worst(std::numeric_limits<T>::min()),
    m_current_worst_index(0),
    m_index(0)
  {}
  void add(T a_value);
  T get();
  std::size_t get_index();
  void clear();
};

template<typename T>
void worst_searcher_t<T>::add(T a_value)
{
  if (a_value > m_current_worst) {
    m_current_worst =  a_value;
    m_current_worst_index = m_index;
  }
  m_index++;
}

template<typename T>
T worst_searcher_t<T>::get()
{
  return m_current_worst;
}

template<typename T>
std::size_t worst_searcher_t<T>::get_index()
{
  return m_current_worst_index;
}

template<typename T>
void worst_searcher_t<T>::clear()
{
  m_current_worst = std::numeric_limits<T>::min();
  m_current_worst_index = 0;
  m_index = 0;
}


#endif // WORST_SEARCHER_H
