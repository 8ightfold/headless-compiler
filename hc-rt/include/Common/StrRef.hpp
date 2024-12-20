//===- Common/StrRef.hpp --------------------------------------------===//
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
//  A non-owning wrapper around string ranges.
//
//===----------------------------------------------------------------===//

#pragma once
#pragma clang system_header

#include "Checked.hpp"
#include "PtrRange.hpp"
#include "Strings.hpp"

namespace hc::common {
  template <typename T>
  concept __strref_compatible = 
    __char_type<__remove_const(T)>;

  struct [[gsl::Pointer]] StrRef : PtrRange<const char> {
    using Type     = const char;
    using BaseType = PtrRange<Type>;
    using PtrType  = BaseType::PtrType;
    static constexpr usize npos = Max<usize>;
  public:
    using BaseType::size;
    using BaseType::sizeInBytes;
    using BaseType::isEmpty;
    using BaseType::data;
    using BaseType::begin;
    using BaseType::end;
    using BaseType::into;
    using BaseType::intoRange;
    using BaseType::operator[];
  public:
    constexpr StrRef() = default;
    constexpr StrRef(const StrRef&) = default;
    constexpr StrRef(StrRef&&) = default;
    constexpr StrRef(BaseType B) : BaseType(B) { }
    StrRef(PtrType S) : BaseType(StrRef::NewRaw(S)) { }
    StrRef(nullptr_t) = delete;
    
    template <__strref_compatible T>
    StrRef(PtrRange<T> P) : StrRef(SelfType::New(P)) { }

    StrRef& operator=(const StrRef& S) __noexcept {
      BaseType::__begin = S.BaseType::__begin;
      BaseType::__end = S.BaseType::__end;
      return *this;
    }

    template <__strref_compatible T>
    StrRef& operator=(const PtrRange<T>& P) __noexcept {
      return *this = StrRef(P);
    }

  public:
    template <__strref_compatible T>
    [[gnu::always_inline, gnu::const]]
    constexpr static StrRef New(PtrRange<T> P) {
      return StrRef(P);
    }

    [[gnu::always_inline, gnu::const]]
    constexpr static StrRef New(PtrType begin, PtrType end) {
      __hc_invariant(begin || !end);
      return BaseType::New(begin, end);
    }

    [[gnu::always_inline, gnu::const]]
    constexpr static StrRef New(PtrType begin, usize size) {
      __hc_invariant(begin || !size);
      return BaseType::New(begin, size);
    }

    template <usize N>
    [[gnu::always_inline, gnu::const]]
    constexpr static StrRef New(Type(&arr)[N]) {
      return New(arr, arr + N);
    }

    constexpr static StrRef NewRaw(PtrType S) {
      if __expect_false(!S) 
        return StrRef();
      return BaseType::New(S, __strlen(S));
    }

    //==================================================================//
    // "Mutators"
    //==================================================================//

    Type& front() const {
      __hc_invariant(size() != 0);
      return BaseType::__begin[0];
    }

    Type& back() const {
      __hc_invariant(size() != 0);
      return BaseType::__end[size() - 1];
    }

    Type& frontSafe() const {
      static constexpr Type C = Type(0);
      if __expect_false(size() == 0)
        return C;
      return front();
    }

    Type& backSafe() const {
      static constexpr Type C = Type(0);
      if __expect_false(size() == 0)
        return C;
      return front();
    }

    //==================================================================//
    // Chaining
    //==================================================================//

    [[nodiscard]] StrRef slice(usize pos, usize n) const {
      __hc_invariant(begin() && (pos + n) <= size());
      return SelfType::New(begin() + pos, n);
    }
    
    [[nodiscard]] StrRef slice(usize n) const {
      __hc_invariant(begin() && n <= size());
      return SelfType::New(begin() + n, end());
    }

    [[nodiscard]] StrRef dropFront(usize n = 1) const {
      $tail_return slice(n);
    }

    [[nodiscard]] StrRef dropBack(usize n = 1) const {
      __hc_invariant(n <= size());
      return slice(0, size() - n);
    }

    [[nodiscard]] StrRef takeFront(usize n = 1) const {
      if (n >= size()) return *this;
      $tail_return dropBack(size() - n);
    }

    [[nodiscard]] StrRef takeBack(usize n = 1) const {
      if (n >= size()) return *this;
      $tail_return dropFront(size() - n);
    }

    StrRef& dropFrontMut(usize n = 1) {
      return (*this = dropFront(n));
    }

    StrRef& dropBackMut(usize n = 1) {
      return (*this = dropBack(n));
    }

    //==================================================================//
    // Comparison
    //==================================================================//

    bool isEqual(StrRef S) const {
      if (size() != S.size())
        return false;
      if (data() == S.data())
        return true;
      return __memcmp(data(), S.data(), size()) == 0;
    }

    bool beginsWith(char C) {
      return !isEmpty() && (*data() == C);
    }

    bool beginsWith(auto&& R) {
      const auto S = StrRef(R);
      if (S.size() > this->size())
        return false;
      return takeFront(S.size()).isEqual(S);
    }

    bool beginsWith(auto&& R, auto&&...RR)
     requires(sizeof...(RR) > 0) {
      return (beginsWith(__hc_fwd(R)) || 
        ... || beginsWith(__hc_fwd(RR)));
    }

    template <usize N>
    bool beginsWithAny(const char(&A)[N]) {
      constexpr usize sz = Checked<true>(N) - 1ULL;
      if constexpr (sz > 0)
       for (usize I = 0; I < sz; ++I) {
        if (beginsWith(A[I]))
          return true;
      }
      return false;
    }

    bool endsWith(char C) {
      return !isEmpty() && (back() == C);
    }

    bool endsWith(auto&& R) {
      const auto S = StrRef(R);
      if (S.size() > this->size())
        return false;
      return takeBack(S.size()).isEqual(S);
    }

    bool endsWith(auto&& R, auto&&...RR)
     requires(sizeof...(RR) > 0) {
      return (endsWith(__hc_fwd(R)) || 
        ... || endsWith(__hc_fwd(RR)));
    }

    __always_inline friend bool 
     operator==(StrRef lhs, StrRef rhs) {
      return lhs.isEqual(rhs);
    }

    friend bool operator==(StrRef S, const char* lit) {
      if __expect_false(!lit) return false;
      return S.isEqual(StrRef::NewRaw(lit));
    }

    //==================================================================//
    // Searching
    //==================================================================//

    

    //==================================================================//
    // Parsing
    //==================================================================//

    bool consumeFront(char C) {
      if (!this->beginsWith(C))
        return false;
      ++BaseType::__begin;
      return true;
    }
    bool consumeFront(StrRef S) {
      if (!this->beginsWith(S))
        return false;
      BaseType::__begin += S.size();
      return true;
    }
    bool consumeFront(auto&& R, auto&&...RR)
     requires(sizeof...(RR) > 0) {
      return (consumeFront(__hc_fwd(R)) || 
        ... || consumeFront(__hc_fwd(RR)));
    }

    bool consumeBack(char C) {
      if (!this->endsWith(C))
        return false;
      --BaseType::__end;
      return true;
    }
    bool consumeBack(StrRef S) {
      if (!this->endsWith(S))
        return false;
      BaseType::__end -= S.size();
      return true;
    }
    bool consumeBack(auto&& R, auto&&...RR)
     requires(sizeof...(RR) > 0) {
      return (consumeBack(__hc_fwd(R)) || 
        ... || consumeBack(__hc_fwd(RR)));
    }

    [[nodiscard]] StrRef dropNull() const {
      if __expect_false(size() == 0) 
        return *this;
      if (back() != '\0')
        return *this;
      const auto len = __strlen(data());
      return takeFront(len);
    }

    StrRef& dropNullMut() {
      return (*this = dropNull());
    }

    // TODO: Replace with something more advanced...
    // Returns `true` on error.
    template <meta::is_unsigned Int>
    [[nodiscard]] bool consumeUnsigned(Int& I) {
      StrRef S = *this;
      if (S.isEmpty())
        return true;
      
      Int result = 0;
      while (!S.isEmpty()) {
        const char C = S[0];
        if __expect_false(C < '0' || C > '9')
          break;
        result += Int(C - '0');
        result *= 10;
        S = S.dropFront();
      }

      I = result;
      *this = S;
      return false;
    }
  };
} // namespace hc::common

namespace hc::literals {
  constexpr com::StrRef operator""sv(const char* S, usize N) {
    return com::StrRef::New(S, N);
  }
} // namespace hc::literals
