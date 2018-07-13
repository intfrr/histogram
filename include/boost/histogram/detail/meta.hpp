// Copyright 2015-2017 Hans Dembinski
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt
// or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef _BOOST_HISTOGRAM_DETAIL_META_HPP_
#define _BOOST_HISTOGRAM_DETAIL_META_HPP_

#include <boost/mp11.hpp>

#include <iterator>
#include <limits>
#include <type_traits>
#include <vector>
#include <utility>
#include <tuple>

namespace boost {
namespace histogram {
namespace detail {

#define BOOST_HISTOGRAM_MAKE_SFINAE(name, cond)                  \
template <typename U> struct name {                              \
  template <typename T, typename = decltype(cond)>               \
  struct SFINAE {};                                              \
  template <typename T> static std::true_type Test(SFINAE<T> *); \
  template <typename T> static std::false_type Test(...);        \
  using type = decltype(Test<U>(nullptr));                       \
};                                                               \
template <typename T>                                            \
using name##_t = typename name<T>::type

BOOST_HISTOGRAM_MAKE_SFINAE(has_variance_support,
                            (std::declval<T&>().value(), std::declval<T&>().variance()));

BOOST_HISTOGRAM_MAKE_SFINAE(has_method_lower,
                            (std::declval<T &>().lower(0)));

BOOST_HISTOGRAM_MAKE_SFINAE(is_dynamic_container,
                            (std::begin(std::declval<T&>())));

BOOST_HISTOGRAM_MAKE_SFINAE(is_static_container,
                            (std::get<0>(std::declval<T&>())));

BOOST_HISTOGRAM_MAKE_SFINAE(is_castable_to_int,
                            (static_cast<int>(std::declval<T&>())));

struct static_container_tag {};
struct dynamic_container_tag {};
struct no_container_tag {};

template <typename T>
using classify_container_t =
  typename std::conditional<
    is_static_container_t<T>::value,
    static_container_tag,
    typename std::conditional<
      is_dynamic_container_t<T>::value,
      dynamic_container_tag,
      no_container_tag
    >::type
  >::type;

template <typename T, typename = decltype(std::declval<T &>().size(),
                                          std::declval<T &>().increase(0),
                                          std::declval<T &>()[0])>
struct requires_storage {};

template <typename T,
          typename = decltype(*std::declval<T &>(), ++std::declval<T &>())>
struct requires_iterator {};

template <typename T>
using requires_axis = decltype(std::declval<T &>().size(),
                        std::declval<T &>().shape(),
                        std::declval<T &>().uoflow(),
                        std::declval<T &>().label(),
                        std::declval<T &>()[0]);

namespace {
  struct bool_mask_impl {
    std::vector<bool> &b;
    bool v;
    template <typename Int> void operator()(Int) const { b[Int::value] = v; }
  };
}

template <typename... Ns>
std::vector<bool> bool_mask(unsigned n, bool v) {
  std::vector<bool> b(n, !v);
  mp11::mp_for_each<mp11::mp_list<Ns...>>(bool_mask_impl{b, v});
  return b;
}

template <typename T>
using size_of = std::tuple_size<typename std::decay<T>::type>;

template <unsigned D, typename T>
using type_of = typename std::tuple_element<D, typename std::decay<T>::type>::type;

namespace {
  template <typename L, typename... Ns>
  struct selection_impl {
    template <typename Int> using at = mp11::mp_at<L, Int>;
    using N = mp11::mp_list<Ns...>;
    using LNs = mp11::mp_assign<L, N>;
    using type = mp11::mp_transform<at, LNs>;
  };
}

template <typename L, typename... Ns>
using selection = typename selection_impl<L, Ns...>::type;

template <typename Ns>
using unique_sorted = mp11::mp_unique<mp11::mp_sort<Ns, mp11::mp_less>>;

namespace {
  template <typename Src, typename Dst>
  struct sub_tuple_assign_impl {
    const Src& src;
    Dst& dst;
    template <typename I1, typename I2>
    void operator()(std::pair<I1, I2>) const {
      std::get<I1::value>(dst) = std::get<I2::value>(src);
    }
  };
}

template <typename T, typename... Ns>
selection<T, Ns...> make_sub_tuple(const T& t) {
  using U = selection<T, Ns...>;
  U u;
  using N1 = mp11::mp_list<Ns...>;
  using Len = mp11::mp_size<N1>;
  using N2 = mp11::mp_iota<Len>;
  using N3 = mp11::mp_transform<std::pair, N2, N1>;
  mp11::mp_for_each<N3>(sub_tuple_assign_impl<T, U>{t, u});
  return u;
}

} // namespace detail
} // namespace histogram
} // namespace boost

#endif
