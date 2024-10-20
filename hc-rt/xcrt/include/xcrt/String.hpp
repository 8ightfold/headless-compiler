//===- xcrt/String.hpp ----------------------------------------------===//
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

#pragma once

#include <Common/Fundamental.hpp>

extern "C" {
#define __rst __restrict

int memcmp(const void* lhs, const void* rhs, usize len);
void* memcpy(void* __rst dst, const void* __rst src, usize len);
wchar_t* wmemcpy(wchar_t* __rst dst, const wchar_t* __rst src, usize len);
void* memset(void* dst, int ch, usize len);

#undef __rst
} // extern "C"

extern "C" {

int strcmp(const char* lhs, const char* rhs);
int wcscmp(const wchar_t* lhs, const wchar_t* rhs);
char* strcpy(char* lhs, const char* rhs);
wchar_t* wcscpy(wchar_t* lhs, const wchar_t* rhs);
usize strlen(const char* lhs);
usize wcslen(const char_t* lhs);
int strncmp(const char* lhs, const char* rhs, usize len);
int wcsncmp(const wchar_t* lhs, const wchar_t* rhs, usize len);

} // extern "C"

namespace xcrt {

using ::memcmp;
using ::memcpy;
using ::memset;

using ::strcmp;
using ::strcpy;
using ::strlen;
using ::strncmp;

using ::wcscmp;
using ::wcscpy;
using ::wcslen;
using ::wcsncmp;
using ::wmemcpy;

} // namespace xcrt
