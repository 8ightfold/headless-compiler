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

#define $PUnion(tys...) ::hc::common::PtrUnion<tys>
#define $extract_member(name, ty...) visitR<ty>([](auto* p) { return p->name; })

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
  using __ptrunion_base = 
    _PtrUnion<make_idxseq<sizeof...(TT)>, TT...>;
  
  __global usize __ptrunion_tag_size  = __bitsizeof(u8);
  __global usize __ptrunion_addr_size = __bitsizeof(uptr) - __ptrunion_tag_size;
  __global usize __ptrunion_max       = 255ULL;

  template <typename...TT>
  struct [[gsl::Pointer]] PtrUnion {
    static_assert(sizeof...(TT) < __ptrunion_max);
    static_assert(__all_unique<TT...>, "Types cannot repeat.");
    using SelfType = PtrUnion<TT...>;
    using BaseType = __ptrunion_base<TT...>;
    struct _SelfTag { };

    template <typename U>
    static constexpr usize _ID = BaseType::GetID((U*)nullptr);

    template <typename R, typename F>
    using _RetType = __conditional_t<__is_same(R, _SelfTag),
      __common_return_t<F, __add_pointer(TT)...>, R>;

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
      return __dyn_cast<U>();
    }

    template <typename U>
    U* getUnchecked() const __noexcept {
      __hc_invariant(__isa<U>());
      return reinterpret_cast<U*>(__addr);
    }

    void visit(auto&& f) const __noexcept {
      if (this->isEmpty()) return;
      $tail_return __visitVoid<TT...>(__hc_fwd(f));
    }

    template <typename R = _SelfTag, typename F>
    decltype(auto) visitR(F&& f) const __noexcept 
     requires(sizeof...(TT) > 0) {
      using Ret = _RetType<R, F>;
      if constexpr (!__is_void(__remove_const(Ret))) {
        if (this->isEmpty())
          $tail_return __visitR<Ret>(__hc_fwd(f));
        // Real visitor
        $tail_return __visitR<Ret, TT...>(__hc_fwd(f));
      } else {
        if (this->isEmpty()) return;
        $tail_return __visitVoid<TT...>(__hc_fwd(f));
      }
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

    template <typename U>
    [[gnu::always_inline, gnu::nodebug]]
    U* __dyn_cast(U* = nullptr) const __noexcept {
      if __expect_true(__isa<U>())
        return reinterpret_cast<U*>(__addr);
      return nullptr;
    }

    template <typename...UU>
    [[gnu::nodebug]]
    void __visitVoid(auto&& f) const __noexcept
     requires(sizeof...(UU) == 0) { }

    template <typename U, typename...UU>
    [[gnu::nodebug]]
    void __visitVoid(auto&& f) const __noexcept {
      if (__tag != _ID<U>)
        $tail_return __visitVoid<UU...>(__hc_fwd(f));
      (void)__hc_fwd(f)(getUnchecked<U>());
    }

    template <typename R, typename...UU>
    R __visitR(auto&& f) const __noexcept
     requires(sizeof...(UU) == 0) {
      if constexpr (!__is_reference(R)) {
        return R{};
      }
      __hc_unreachable("Unable to instantiate a default object.");
    }

    template <typename R, typename U, typename...UU>
    [[gnu::nodebug]]
    R __visitR(auto&& f) const __noexcept {
      if (__tag != _ID<U>)
        $tail_return __visitR<R, UU...>(__hc_fwd(f));
      return static_cast<R>(__hc_fwd(f)(getUnchecked<U>()));
    }

  private:
    uptr __addr : __ptrunion_addr_size = 0;
    uptr __tag  : __ptrunion_tag_size  = 0;
  };

  using DualString = PtrUnion<const char, const wchar_t>;
} // namespace hc::common