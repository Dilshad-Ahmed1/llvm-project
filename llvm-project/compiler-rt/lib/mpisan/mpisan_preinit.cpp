//===-- mpisan_preinit.cpp - MPI Usage Sanitizer ------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// This section is linked into the main executable when -fsanitize=mpi-usage is
// specified to perform initialization at a very early stage.
//===----------------------------------------------------------------------===//

#include "mpisan/mpisan_rtl.h"
#include "sanitizer_common/sanitizer_internal_defs.h"

#if SANITIZER_CAN_USE_PREINIT_ARRAY

// This section is linked into the main executable when -fsanitize=mpi-usage is
// specified to perform initialization at a very early stage.
__attribute__((section(".preinit_array"), used)) static auto preinit =
    __mpisan_init;

#endif
