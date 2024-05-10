//===- UnionDriver.cpp ----------------------------------------------===//
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

#include <Common/TaggedEnum.hpp>
#include <Common/TaggedUnion.hpp>
#include <Meta/Refl.hpp>
#include <Meta/Traits.hpp>
#include <Meta/Unwrap.hpp>

#pragma push_macro("NDEBUG")
#undef NDEBUG
#include <cassert>
#include <cstdio>
#pragma pop_macro("NDEBUG")

using namespace hc;
namespace C = hc::common;
namespace M = hc::meta;

struct MoveOnly {
  MoveOnly() = default;
  MoveOnly(const MoveOnly&) = delete;
  MoveOnly(MoveOnly&&) = default;
  MoveOnly& operator=(const MoveOnly&) = delete;
  MoveOnly& operator=(MoveOnly&&) = default;
  constexpr ~MoveOnly() {
    assert(this->b);
    this->b = false;
  }
public:
  bool b = true;
};

$StrongEnum((WindowsSubsystemType, u16),
  (eSubsystemUnknown,    0),
  (eSubsystemNative,     1),
  (eSubsystemWindowsGUI, 2),
  (eSubsystemWindowsCUI, 3),
  (eSubsystemPosixCUI,   7),
  // ...
  (eSubsystemEFIApp,     10),
  (eSubsystemEFIBoot,    11),
  (eSubsystemEFIRuntime, 12),
  (eSubsystemEFIROM,     13)
  // ...
);

$Union(Foo,
  (X),
  (Y, i32),
  (Z, f32, f32)
);

$Union(Bar,
  (A),
  (B, Foo),
  (C, MoveOnly, MoveOnly)
);

void foo_disp(Foo& foo) {
  $match(foo) {
    $arm(X) {
      std::printf("X\n");
    }
    $arm(Y, e) {
      std::printf("Y: %i\n", e);
    }
    $arm(Z, (f1, f2)) {
      std::printf("Z: [%f, %f]\n", f1, f2);
    }
  }
}

int main() {
  Bar bar = Bar::A();
  {
    Foo foo = Foo::X();
    foo_disp(foo);
    foo = Foo::Y(7);
    foo_disp(foo);
    foo = Foo::Z(3.0f, 9.0f);
    bar = Bar::B($mv(foo));
    foo_disp(foo);
  }
  foo_disp(bar.as_B());
}
