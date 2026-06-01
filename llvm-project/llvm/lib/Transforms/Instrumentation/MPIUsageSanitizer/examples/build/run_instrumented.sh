#!/bin/bash
# Script to run instrumented MPI examples

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
INSTRUMENTED_DIR="$SCRIPT_DIR/instrumented"

if [ $# -eq 0 ]; then
    echo "Usage: $0 <category/program> [mpi_args...]"
    echo "Example: $0 basic/hello_world -np 4"
    echo ""
    echo "Available programs:"
    find "$INSTRUMENTED_DIR" -type f -executable | sed "s|$INSTRUMENTED_DIR/||" | sort
    exit 1
fi

PROGRAM="$1"
shift

if [ ! -f "$INSTRUMENTED_DIR/$PROGRAM" ]; then
    echo "Error: Program $PROGRAM not found"
    exit 1
fi

echo "Running instrumented: $PROGRAM"
echo "MPI Sanitizer output will be displayed below:"
echo "================================================"
mpirun "$@" "$INSTRUMENTED_DIR/$PROGRAM"
