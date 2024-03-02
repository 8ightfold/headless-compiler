//===- Meta/ASM.hpp -------------------------------------------------===//
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
//  Defines some macros for creating inline assembly functions.
//
//===----------------------------------------------------------------===//

#pragma once
#pragma clang system_header

#include "Preproc.hpp"

#define $ASM_stmt(expr) expr ";\n"
#define $ASM_block(exprs...) __asm__ volatile ( exprs );
/// Creates an `__asm__` block with the input expressions, appends `";\n"`.
#define $ASM(exprs...) $ASM_block( $PP_mapC($ASM_stmt, ##exprs) )
/// Creates an `__asm__` block with an asm label, `label:`.
#define $label(label) $ASM_block( __hc_stringify(label:) )
/// Creates an inline asm label, `label:`.
#define $lbl(label) __hc_stringify(label:)

/// Creates an assembly-only function.
#define $ASM_func(ret, name, arglist, exprs...) \
[[gnu::noinline, gnu::naked]] \
ret __stdcall name arglist { \
  $ASM( exprs ) \
}

#define __$ASM_alias_alias(alias, _) alias
#define __$ASM_alias_name(_, name) name
#define __$ASM_alias_name_attr(_, name) \
  __attribute__((alias(__hc_stringify(name))))
#define $ASM_alias(ret, alias_name, arglist) \
 ret $PP_expand( __$ASM_alias_name  alias_name ) arglist; \
 $PP_expand( __$ASM_alias_name_attr alias_name ) \
 ret $PP_expand( __$ASM_alias_alias alias_name ) arglist
