//===- Common/PtrUnion.hpp --------------------------------------===//
//
// Copyright (C) 2024 Eightfold
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
//     limitations under the License.
//
//===----------------------------------------------------------------===//
//
//  A tagged pointer holding a single pointer type from a set.
//
//===----------------------------------------------------------------===//

#pragma once

#include "Features.hpp"
#include "Fundamental.hpp"
#include "Traits.hpp"

namespace hc::common {
  template <typename T, usize I>
  struct _PtrUNode {
    using Type = T*;
    static constexpr usize value = I;
  public:
    __always_inline static constexpr 
     IdxNode<I + 1> __GetI(TyNode<T>) __noexcept 
    { return { }; }
  };

  template <typename IS, typename...>
  struct _PtrUnion;

  template <usize...II, typename...TT>
  struct _PtrUnion<IdxSeq<II...>, TT...> : _PtrUNode<TT, II>... {
  private: 
    using _PtrUNode<TT, II>::__GetI...;
  public:
    template <typename U>
    [[gnu::always_inline, gnu::const]] 
    static constexpr usize GetID(U*) __noexcept {
      return __GetI(TyNode<U>{}).value;
    }
  
    template <usize I>
    [[gnu::always_inline, gnu::const]] 
    static constexpr auto GetTy() __noexcept {
      return TyNode<__selector_t<I, TT...>>{};
    }
  };

  template <typename...TT>
  using __ptr_union_base = 
    _PtrUnion<make_idxseq<sizeof...(TT)>, TT...>;
  
  __global usize __ptr_union_max = 255;

  template <typename...TT>
  struct [[gsl::Pointer]] PtrUnion {
    static_assert(sizeof...(TT) < __ptr_union_max);
    static_assert(__all_unique<TT...>, "Types cannot repeat.");
    using SelfType = PtrUnion<TT...>;
    using BaseType = __ptr_union_base<TT...>;

    template <typename, usize>
    friend struct _PtrUNode;

    template <typename U>
    static constexpr usize _ID = BaseType::GetID((U*)nullptr);

  public:
    PtrUnion() = default;
    PtrUnion(const PtrUnion&) = default;
    PtrUnion(nullptr_t) : PtrUnion() { }
    PtrUnion& operator=(const PtrUnion&) = default;

    template <typename U>
    requires __any_same<U, TT...> PtrUnion(U* data) 
     : __addr(reinterpret_cast<uptr>(data)), 
     __tag(data ? _ID<U> : 0U) { }
    
    template <typename U>
    requires __any_same<U, TT...> 
    PtrUnion& operator=(U* data) __noexcept {
      return this->set(data);
    }

  public:
    template <typename U>
    requires __any_same<U, TT...>
    SelfType& set(U* data) __noexcept {
      if __expect_false(!data)
        return *this;
      this->__addr = reinterpret_cast<uptr>(data);
      this->__tag  = _ID<U>;
      return *this;
    }

    template <typename U>
    requires __any_same<U, TT...>
    U* get() const __noexcept {
      if __expect_true(__isa<U>())
        return reinterpret_cast<U*>(__addr);
      return nullptr;
    }


    template <typename U>
    U* getUnchecked() const __noexcept {
      __hc_invariant(__isa<U>());
      return reinterpret_cast<U*>(__addr);
    }

    template <typename F>
    void visit(F&& f) const __noexcept {
      if(this->isEmpty()) return;
      $tail_return __visit<TT...>(__hc_fwd(f));
    }

    //=== Observers ===//

    bool isEmpty() const __noexcept {
      return this->__tag == 0;
    }

    explicit operator bool() const __noexcept {
      return !this->isEmpty();
    }

    //=== Internals ===//

    template <typename U>
    [[gnu::always_inline, gnu::nodebug]]
    bool __isa(U* = nullptr) const __noexcept {
      return this->__tag == _ID<U>;
    }

    template <typename...UU>
    [[gnu::nodebug]]
    void __visit(auto&& f) const __noexcept
     requires(sizeof...(UU) == 0) { }

    template <typename U, typename...UU>
    [[gnu::nodebug]]
    void __visit(auto&& f) const __noexcept {
      if(__tag != _ID<U>)
        $tail_return __visit<UU...>(__hc_fwd(f));
      (void)__hc_fwd(f)(getUnchecked<U>());
    }

  private:
    uptr __addr : __bitsizeof(uptr) - 8 = 0;
    uptr __tag  : 8 = 0;
  };
} // namespace hc::common