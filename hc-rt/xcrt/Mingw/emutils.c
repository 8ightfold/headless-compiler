//===---------- emutls.c - Implements __emutls_get_address ---------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "int_lib.h"

#define EMUTLS_SKIP_DESTRUCTOR_ROUNDS 0

typedef struct emutls_address_array {
  uintptr_t skip_destructor_rounds;
  uintptr_t size; // number of elements in the 'data' array
  void *data[];
} emutls_address_array;

static void emutls_shutdown(emutls_address_array *array);

#include <assert.h>
#include <malloc.h>
#include <stdio.h>
#include <windows.h>

static LPCRITICAL_SECTION emutls_mutex;
static DWORD emutls_tls_index = TLS_OUT_OF_INDEXES;

typedef uintptr_t gcc_word;
typedef void *gcc_pointer;

static void win_error(DWORD last_err, const char *hint) {
  char *buffer = NULL;
  if (FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER |
                         FORMAT_MESSAGE_FROM_SYSTEM |
                         FORMAT_MESSAGE_MAX_WIDTH_MASK,
                     NULL, last_err, 0, (LPSTR)&buffer, 1, NULL)) {
    fprintf(stderr, "Windows error: %s\n", buffer);
  } else {
    fprintf(stderr, "Unknown Windows error: %s\n", hint);
  }
  LocalFree(buffer);
}

static __inline void win_abort(DWORD last_err, const char *hint) {
  win_error(last_err, hint);
  abort();
}

static __inline void *emutls_memalign_alloc(size_t align, size_t size) {
  void *base = _aligned_malloc(size, align);
  if (!base)
    win_abort(GetLastError(), "_aligned_malloc");
  return base;
}

static __inline void emutls_memalign_free(void *base) { _aligned_free(base); }

static void emutls_exit(void) {
  if (emutls_mutex) {
    DeleteCriticalSection(emutls_mutex);
    _aligned_free(emutls_mutex);
    emutls_mutex = NULL;
  }
  if (emutls_tls_index != TLS_OUT_OF_INDEXES) {
    emutls_shutdown((emutls_address_array *)TlsGetValue(emutls_tls_index));
    TlsFree(emutls_tls_index);
    emutls_tls_index = TLS_OUT_OF_INDEXES;
  }
}

static BOOL CALLBACK emutls_init(PINIT_ONCE p0, PVOID p1, PVOID *p2) {
  (void)p0;
  (void)p1;
  (void)p2;
  emutls_mutex =
      (LPCRITICAL_SECTION)_aligned_malloc(sizeof(CRITICAL_SECTION), 16);
  if (!emutls_mutex) {
    win_error(GetLastError(), "_aligned_malloc");
    return FALSE;
  }
  InitializeCriticalSection(emutls_mutex);

  emutls_tls_index = TlsAlloc();
  if (emutls_tls_index == TLS_OUT_OF_INDEXES) {
    emutls_exit();
    win_error(GetLastError(), "TlsAlloc");
    return FALSE;
  }
  atexit(&emutls_exit);
  return TRUE;
}

static __inline void emutls_init_once(void) {
  static INIT_ONCE once;
  InitOnceExecuteOnce(&once, emutls_init, NULL, NULL);
}

static __inline void emutls_lock(void) { EnterCriticalSection(emutls_mutex); }

static __inline void emutls_unlock(void) { LeaveCriticalSection(emutls_mutex); }

static __inline void emutls_setspecific(emutls_address_array *value) {
  if (TlsSetValue(emutls_tls_index, (LPVOID)value) == 0)
    win_abort(GetLastError(), "TlsSetValue");
}

static __inline emutls_address_array *emutls_getspecific(void) {
  LPVOID value = TlsGetValue(emutls_tls_index);
  if (value == NULL) {
    const DWORD err = GetLastError();
    if (err != ERROR_SUCCESS)
      win_abort(err, "TlsGetValue");
  }
  return (emutls_address_array *)value;
}

static size_t emutls_num_object = 0; // number of allocated TLS objects

// Free the allocated TLS data
static void emutls_shutdown(emutls_address_array *array) {
  if (array) {
    uintptr_t i;
    for (i = 0; i < array->size; ++i) {
      if (array->data[i])
        emutls_memalign_free(array->data[i]);
    }
  }
}

// For every TLS variable xyz,
// there is one __emutls_control variable named __emutls_v.xyz.
// If xyz has non-zero initial value, __emutls_v.xyz's "value"
// will point to __emutls_t.xyz, which has the initial value.
typedef struct __emutls_control {
  // Must use gcc_word here, instead of size_t, to match GCC.  When
  // gcc_word is larger than size_t, the upper extra bits are all
  // zeros.  We can use variables of size_t to operate on size and
  // align.
  gcc_word size;  // size of the object in bytes
  gcc_word align; // alignment of the object in bytes
  union {
    uintptr_t index; // data[index-1] is the object address
    void *address;   // object address, when in single thread env
  } object;
  void *value; // null or non-zero initial value for the object
} __emutls_control;

// Emulated TLS objects are always allocated at run-time.
static __inline void *emutls_allocate_object(__emutls_control *control) {
  // Use standard C types, check with gcc's emutls.o.
  COMPILE_TIME_ASSERT(sizeof(uintptr_t) == sizeof(gcc_pointer));
  COMPILE_TIME_ASSERT(sizeof(uintptr_t) == sizeof(void *));

  size_t size = control->size;
  size_t align = control->align;
  void *base;
  if (align < sizeof(void *))
    align = sizeof(void *);
  // Make sure that align is power of 2.
  if ((align & (align - 1)) != 0)
    abort();

  base = emutls_memalign_alloc(align, size);
  if (control->value)
    memcpy(base, control->value, size);
  else
    memset(base, 0, size);
  return base;
}

// Returns control->object.index; set index if not allocated yet.
static __inline uintptr_t emutls_get_index(__emutls_control *control) {
  uintptr_t index = __atomic_load_n(&control->object.index, __ATOMIC_ACQUIRE);
  if (!index) {
    emutls_init_once();
    emutls_lock();
    index = control->object.index;
    if (!index) {
      index = ++emutls_num_object;
      __atomic_store_n(&control->object.index, index, __ATOMIC_RELEASE);
    }
    emutls_unlock();
  }
  return index;
}

// Updates newly allocated thread local emutls_address_array.
static __inline void emutls_check_array_set_size(emutls_address_array *array,
                                                 uintptr_t size) {
  if (array == NULL)
    abort();
  array->size = size;
  emutls_setspecific(array);
}

// Returns the new 'data' array size, number of elements,
// which must be no smaller than the given index.
static __inline uintptr_t emutls_new_data_array_size(uintptr_t index) {
  // Need to allocate emutls_address_array with extra slots
  // to store the header.
  // Round up the emutls_address_array size to multiple of 16.
  uintptr_t header_words = sizeof(emutls_address_array) / sizeof(void *);
  return ((index + header_words + 15) & ~((uintptr_t)15)) - header_words;
}

// Returns the size in bytes required for an emutls_address_array with
// N number of elements for data field.
static __inline uintptr_t emutls_asize(uintptr_t N) {
  return N * sizeof(void *) + sizeof(emutls_address_array);
}

// Returns the thread local emutls_address_array.
// Extends its size if necessary to hold address at index.
static __inline emutls_address_array *
emutls_get_address_array(uintptr_t index) {
  emutls_address_array *array = emutls_getspecific();
  if (array == NULL) {
    uintptr_t new_size = emutls_new_data_array_size(index);
    array = (emutls_address_array *)malloc(emutls_asize(new_size));
    if (array) {
      memset(array->data, 0, new_size * sizeof(void *));
      array->skip_destructor_rounds = EMUTLS_SKIP_DESTRUCTOR_ROUNDS;
    }
    emutls_check_array_set_size(array, new_size);
  } else if (index > array->size) {
    uintptr_t orig_size = array->size;
    uintptr_t new_size = emutls_new_data_array_size(index);
    array = (emutls_address_array *)realloc(array, emutls_asize(new_size));
    if (array)
      memset(array->data + orig_size, 0,
             (new_size - orig_size) * sizeof(void *));
    emutls_check_array_set_size(array, new_size);
  }
  return array;
}

// Our emulated TLS implementation relies on local state (e.g. for the pthread
// key), and if we duplicate this state across different shared libraries,
// accesses to the same TLS variable from different shared libraries will yield
// different results (see https://github.com/android/ndk/issues/1551 for an
// example). __emutls_get_address is the only external entry point for emulated
// TLS, and by making it default visibility and weak, we can rely on the dynamic
// linker to coalesce multiple copies at runtime and ensure a single unique copy
// of TLS state. This is a best effort; it won't work if the user is linking
// with -Bsymbolic or -Bsymbolic-functions, and it also won't work on Windows,
// where the dynamic linker has no notion of coalescing weak symbols at runtime.
// A more robust solution would be to create a separate shared library for
// emulated TLS, to ensure a single copy of its state.
void *__emutls_get_address(__emutls_control *control) {
  uintptr_t index = emutls_get_index(control);
  emutls_address_array *array = emutls_get_address_array(index--);
  if (array->data[index] == NULL)
    array->data[index] = emutls_allocate_object(control);
  return array->data[index];
}
