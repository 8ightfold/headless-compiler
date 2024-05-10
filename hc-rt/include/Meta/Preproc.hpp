//===- Common/Preproc.hpp ---------------------------------------===//
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
//  This file defines some common preprocessor utilities.
//
//===----------------------------------------------------------------===//

#pragma once

#include <Common/Predefs.hpp>

// For more info:
// https://github.com/pfultz2/Cloak/wiki
// https://github.com/swansontec/map-macro

#define __$COMMA__ ,
#define __$EMPTY__
#define __$LBRACK__ [
#define __$RBRACK__ ]

#define $PP_expand(args...) args
#define $PP_eat(...)
#define $PP_eat2(...) $PP_eat

#define __$test_emptyP(...) __VA_OPT__(0,) 1
#define __$test_emptyI1(n, ...) n
#define __$test_emptyI0(...) __$test_emptyI1(__VA_ARGS__)
#define $PP_is_empty(...) __$test_emptyI0(__$test_emptyP(__VA_ARGS__))

#define __$eat_empty_(f) $PP_eat
#define __$eat_empty_X(f) f
#define $PP_invoke_valued(f, ...) __hc_2cat(__$eat_empty_, __VA_OPT__(X))(f)(__VA_ARGS__)

#define __$count(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, _16, count, ...) count
#define $PP_count(args...) __$count(0, ##__args, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0)

#define __$compl_0 1
#define __$compl_1 0
#define $PP_compl(ex) __hc_2cat(__$compl_, ex)

#define __$iif_0(t, f) f
#define __$iif_1(t, f) t
#define $PP_iif(cond) __hc_2cat(__$iif_, cond)

#define __$chk_n(x, n, ...) n
#define __$chk(ex...) __$chk_n(ex, 0,)
#define __$probe(ex...) (), 1,
#define __$PROBE__ __$probe(~)

#define __$pchk(ex...) __$chk_n(ex, 1,)
#define __$pprobe(ex...) (), 0,
#define __$parens(ex...) __$pchk(__$pprobe ex)

#define __$not_0     __$PROBE__
#define __$not_false __$PROBE__
#define $PP_not(ex) __$chk(__hc_2cat(__$not_, ex))

#define $PP_bool(ex) $PP_compl($PP_not(ex))
#define $PP_parens(ex...) $PP_not(__$parens(ex))
#define $PP_if(cond) $PP_iif($PP_bool(cond))
#define $PP_if_parens(ex...) $PP_iif($PP_not(__$parens(ex)))
#define $PP_if_valued(ex...) $PP_iif($PP_not($PP_is_empty(ex)))
#define $PP_rm_parens(ex...) $PP_if_parens(ex)($PP_expand,) ex

#define __$eval0(...) __VA_ARGS__
#define __$eval1(...) __$eval0(__$eval0(__$eval0(__VA_ARGS__)))
#define __$eval2(...) __$eval1(__$eval1(__$eval1(__VA_ARGS__)))
#define __$eval3(...) __$eval2(__$eval2(__$eval2(__VA_ARGS__)))
#define __$eval4(...) __$eval3(__$eval3(__$eval3(__VA_ARGS__)))
#define $PP_eval(...) __$eval4(__$eval4(__$eval4(__VA_ARGS__)))

#define __$map_end2() 0, $PP_eat
#define __$map_end1(...) __$map_end2
#define __$map_end(...) __$map_end1
#define __$map_nxt0(test, next, ...) next __$EMPTY__
#define __$map_nxt1(test, next) __$map_nxt0(test, next, 0)
#define __$map_nxt(test, next)  __$map_nxt1(__$map_end test, next)

#define __$map0(f, x, peek, args...) f(x) __$map_nxt(peek, __$map1)(f, peek, args)
#define __$map1(f, x, peek, args...) f(x) __$map_nxt(peek, __$map0)(f, peek, args)

#define __$mapL_nxt1(test, next) __$map_nxt0(test, __$COMMA__ next, 0)
#define __$mapL_nxt(test, next)  __$mapL_nxt1(__$map_end test, next)

#define __$mapL0(f, x, peek, args...) f(x) __$mapL_nxt(peek, __$mapL1)(f, peek, args)
#define __$mapL1(f, x, peek, args...) f(x) __$mapL_nxt(peek, __$mapL0)(f, peek, args)

#define $PP_map(f, args...)  $PP_eval(__$map1(f, args, ()()(), ()()(), ()()(), 0))
#define $PP_mapL(f, args...) $PP_eval(__$mapL1(f, args, ()()(), ()()(), ()()(), 0))

#define $PP_mapC(f, args...)  $PP_if_valued(args)($PP_map, $PP_eat)(f, args)
#define $PP_mapCL(f, args...) $PP_if_valued(args)($PP_mapL, $PP_eat)(f, args)
