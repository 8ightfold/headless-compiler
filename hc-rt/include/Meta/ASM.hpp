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

//////////////////////////////////////////////////////////////////////////
// $ASM_alias(
// int, (myfunc_a, _Z6myfuncif), (int x, float y));
// 
// $ASM_func(
// int, myfunc, (int x, float y),
//   "cvtsi2ss  %ecx,  %xmm0",
//   "addss     %xmm0, %xmm1",
//   "cvttss2si %xmm1, %eax",
//   "retq"
// )
//////////////////////////////////////////////////////////////////////////

#define $ASM_stmt(expr) expr ";\n"
#define $ASM_block(exprs...) __asm__ volatile ( exprs );
/// Creates an `__asm__` block with the input expressions, appends `";\n"`.
#define $ASM(exprs...) $ASM_block( $PP_mapC($ASM_stmt, ##exprs) )
/// Creates an `__asm__` block with an asm label, `label:`.
#define $label(label) $ASM_block( __hc_stringify(label:) )
/// Creates an inline asm label, `label:`.
#define $lbl(label) __hc_stringify(label:)

/// Creates an assembly-only function.
/// @param ret      The return type.
/// @param callconv The calling convention.
/// @param name     The name of the function.
/// @param arglist  The list of parameters in parentheses.
/// @param exprs    A comma-separated list of at&t assembly strings.
#define $ASM_func_cc(ret, cconv, name, arglist, exprs...) \
[[gnu::noinline, gnu::naked]] \
$PP_rm_parens(ret) cconv name arglist { \
  $ASM( exprs ) \
}

/// Creates an assembly-only function with the default calling convention.
/// @param ret      The return type.
/// @param name     The name of the function.
/// @param arglist  The list of parameters in parentheses.
/// @param exprs    A comma-separated list of at&t assembly strings.
#define $ASM_func(ret, name, arglist, exprs...) \
  $ASM_func_cc(ret, __defaultcall, name, arglist, ##exprs)

//////////////////////////////////////////////////////////////////////////

#define __$ASM_alias_alias(alias, _) alias
#define __$ASM_alias_name(_, name) name
#define __$ASM_alias_name_attr(_, name) \
  __attribute__((alias(__hc_stringify(name))))

#define __$ASM_alias_def(alias_name, arglist) \
  $PP_expand( __$ASM_alias_name  alias_name ) arglist
#define __$ASM_alias_imp(alias_name, arglist) \
  $PP_expand( __$ASM_alias_alias  alias_name ) arglist

/// Creates a function alias.
/// @param ret      The return type.
/// @param callconv The calling convention of the functions.
/// @param names    The names, in `(alias, real)` form.
/// @param arglist  The list of parameters in parentheses.
#define $ASM_alias_cc(ret, cconv, alias_name, arglist) \
 extern "C" { [[gnu::noinline, gnu::naked]] $PP_rm_parens(ret) \
  cconv __$ASM_alias_def(alias_name, arglist); } \
 [[gnu::noinline, gnu::naked]] \
 $PP_expand( __$ASM_alias_name_attr alias_name ) $PP_rm_parens(ret) \
  cconv __$ASM_alias_imp(alias_name, arglist)

/// Creates a function alias with the default calling convention.
/// @param ret      The return type.
/// @param names    The names, in `(alias, real)` form.
/// @param arglist  The list of parameters in parentheses.
#define $ASM_alias(ret, alias_name, arglist) \
  $ASM_alias_cc(ret, __defaultcall, alias_name, arglist)
