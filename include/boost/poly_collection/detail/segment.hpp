/* Copyright 2016 Joaquin M Lopez Munoz.
 * Distributed under the Boost Software License, Version 1.0.
 * (See accompanying file LICENSE_1_0.txt or copy at
 * http://www.boost.org/LICENSE_1_0.txt)
 *
 * See http://www.boost.org/libs/poly_collection for library home page.
 */

#ifndef BOOST_POLY_COLLECTION_DETAIL_SEGMENT_HPP
#define BOOST_POLY_COLLECTION_DETAIL_SEGMENT_HPP

#if defined(_MSC_VER)
#pragma once
#endif

#include <memory>
#include <type_traits>
#include <utility>

namespace boost{

namespace poly_collection{

namespace detail{

/* segment<Model,Allocator> encapsulates implementations of
 * Model::segment_backend virtual interface under a value-semantics type for
 * use by poly_collection. The techique is described by Sean Parent at slides
 * 157-205 of
 * https://github.com/sean-parent/sean-parent.github.com/wiki/
 *   presentations/2013-09-11-cpp-seasoning/cpp-seasoning.pdf
 */

template<typename Model,typename Allocator>
class segment
{
public:
  using value_type=typename Model::value_type;
  using base_iterator=typename Model::base_iterator;
  using const_base_iterator=typename Model::const_base_iterator;
  using base_sentinel=typename Model::base_sentinel;
  using const_base_sentinel=typename Model::const_base_sentinel;
  template<typename T>
  using iterator=typename Model::template iterator<T>;
  template<typename T>
  using const_iterator=typename Model::template const_iterator<T>;

  template<typename T>
  static segment make(const Allocator& al)
  {
    return std::unique_ptr<segment_backend>{Model::template make<T>(al)};
  }

  /* clones the implementation of x with no elements */

  static segment make_from_prototype(const segment& x)
  {
    return {from_prototype{},x};
  }

  segment(const segment& x):pimpl{x.pimpl->copy()}{set_sentinel();}
  segment(segment&& x)=default;
  segment& operator=(segment x) // TODO: Why do we need this?
  {
    pimpl=std::move(x.pimpl);
    snt=x.snt;
    return *this;
  }

  friend bool operator==(const segment& x,const segment& y)
  {
    if(typeid(*(x.pimpl))!=typeid(*(y.pimpl)))return false;
    else return x.pimpl->equal(y.pimpl.get());
  }

  friend bool operator!=(const segment& x,const segment& y){return !(x==y);}

  base_iterator begin()const noexcept{return pimpl->begin();}
  base_iterator end()const noexcept{return pimpl->end();}
  base_sentinel sentinel()const noexcept{return snt;}
  bool          empty()const noexcept{return pimpl->empty();}
  std::size_t   size()const noexcept{return pimpl->size();}
  std::size_t   max_size()const noexcept{return pimpl->max_size();}
  void          reserve(std::size_t n){filter(pimpl->reserve(n));}
  std::size_t   capacity()const noexcept{return pimpl->capacity();}
  void          shrink_to_fit(){filter(pimpl->shrink_to_fit());}
  template<typename Iterator>
  base_iterator emplace(Iterator it,void (*emplf)(void*,void*),void* arg)
                  {return filter(pimpl->emplace(decay(it),emplf,arg));}
  base_iterator emplace_back(void (*emplf)(void*,void*),void* arg)
                  {return filter(pimpl->emplace_back(emplf,arg));}
  template<typename T>
  base_iterator push_back(const T& x)
                  {return filter(pimpl->push_back(subtype_address(x)));}
  template<
    typename T,
    typename std::enable_if<
      !std::is_lvalue_reference<T>::value&&!std::is_const<T>::value
    >::type* =nullptr
  >
  base_iterator push_back(T&& x)
                  {return filter(pimpl->push_back_move(subtype_address(x)));}
  template<typename Iterator,typename T>
  base_iterator insert(Iterator it,const T& x)
                  {return filter(pimpl->insert(decay(it),subtype_address(x)));}
  template<
    typename Iterator,typename T,
    typename std::enable_if<
      !std::is_lvalue_reference<T>::value&&!std::is_const<T>::value
    >::type* =nullptr
  >
  base_iterator insert(Iterator it,T&& x)
                  {return filter(
                    pimpl->insert_move(decay(it),subtype_address(x)));}
  template<typename Iterator>
  base_iterator erase(Iterator it){return filter(pimpl->erase(decay(it)));}
  template<typename Iterator>
  base_iterator erase(Iterator f,Iterator l)
                  {return filter(pimpl->erase(decay(f),decay(l)));}
  template<typename Iterator>
  base_iterator erase_till_end(Iterator f)
                  {return filter(pimpl->erase_till_end(decay(f)));}
  template<typename Iterator>
  base_iterator erase_from_begin(Iterator l)
                  {return filter(pimpl->erase_from_begin(decay(l)));}
  void          clear()noexcept{filter(pimpl->clear());}

private:
  using segment_backend=typename Model::segment_backend;
  using position_pointer=typename segment_backend::position_pointer;
  using range=typename segment_backend::range;

  struct from_prototype{};

  segment(std::unique_ptr<segment_backend>&& pimpl):
    pimpl{std::move(pimpl)}{set_sentinel();}
  segment(from_prototype,const segment& x):
    pimpl{x.pimpl->empty_copy()}{set_sentinel();}

  template<typename T>
  static void*       subtype_address(T& x){return Model::subtype_address(x);}
  template<typename T>
  static const void* subtype_address(const T& x)
                       {return Model::subtype_address(x);}

  static const_base_iterator decay(base_iterator it){return it;}
  static const_base_iterator decay(const_base_iterator it){return it;}
  template<typename T>
  static position_pointer    decay(iterator<T> it)
                               {return static_cast<T*>(it);}
  template<typename T>
  static position_pointer    decay(const_iterator<T> it)
                               {return static_cast<const T*>(it);}

  void          set_sentinel(){filter(pimpl->end());}
  void          filter(base_sentinel x){snt=x;}
  base_iterator filter(const range& x){snt=x.second;return x.first;}

  std::unique_ptr<segment_backend> pimpl;
  base_sentinel                    snt;
};

} /* namespace poly_collection::detail */

} /* namespace poly_collection */

} /* namespace boost */

#endif
