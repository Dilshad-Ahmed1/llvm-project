//===-- mpisan_flags.inc - MPI Usage Sanitizer flag definitions -----------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// MPISan runtime flags. Consumed via the MPISAN_OPTIONS environment variable.
//
// Usage:
//   MPISAN_OPTIONS="halt_on_error=0:print_stats=1" ./my_mpi_program
//
//===----------------------------------------------------------------------===//
#ifndef MPISAN_FLAG
#error "Define MPISAN_FLAG prior to including this file!"
#endif

// MPISAN_FLAG(Type, Name, DefaultValue, Description)

/// Abort the program after the first error is reported.
MPISAN_FLAG(bool, halt_on_error, true,
            "Abort after the first MPI usage error is reported.")

/// Enable detection of MPI type mismatches between send and receive.
MPISAN_FLAG(bool, check_type_mismatch, true,
            "Detect MPI datatype mismatches between send and receive calls.")

/// Enable detection of element count mismatches between send and receive.
MPISAN_FLAG(bool, check_count_mismatch, true,
            "Detect element count mismatches between send and receive calls.")

/// Enable detection of overlapping (aliased) send/receive buffers.
MPISAN_FLAG(bool, check_buffer_aliasing, true,
            "Detect overlapping send/receive buffer regions.")

/// Enable validation of collective operations across ranks.
MPISAN_FLAG(bool, check_collective_mismatch, true,
            "Detect mismatched collective operations across communicator ranks.")

/// Enable deadlock detection via wait-for graph cycle detection.
MPISAN_FLAG(bool, check_deadlock, true,
            "Detect potential MPI deadlocks using a wait-for graph.")

/// Timeout in microseconds before a pending operation is flagged as a
/// potential deadlock.
MPISAN_FLAG(int, deadlock_timeout_us, 5000000,
            "Microseconds to wait before flagging a pending op as a deadlock.")

/// Print a summary of all detected errors on program exit.
MPISAN_FLAG(bool, print_stats, false,
            "Print a summary of detected errors on program exit.")

/// Suppress duplicate error reports with identical stack traces.
MPISAN_FLAG(bool, suppress_equal_stacks, true,
            "Suppress duplicate error reports with the same stack trace.")

/// Enable lightweight mode: only type/count checking, no buffer tracking.
MPISAN_FLAG(bool, lightweight_mode, false,
            "Enable lightweight mode (type/count checks only, no buffer "
            "aliasing or deadlock detection).")

/// Path to a suppressions file.
MPISAN_FLAG(const char *, suppressions, "",
            "Path to a suppressions file listing errors to ignore.")

/// Verbosity level (0 = errors only, 1 = warnings, 2 = debug).
MPISAN_FLAG(int, verbosity, 0,
            "Verbosity level: 0=errors only, 1=warnings, 2=debug.")
