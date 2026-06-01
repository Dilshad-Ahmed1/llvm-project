#!/bin/bash
# Script to run uninstrumented MPI examples

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
UNINSTRUMENTED_DIR="$SCRIPT_DIR/uninstrumented"

if [ $# -eq 0 ]; then
    echo "Usage: $0 <category/program> [mpi_args...]"
    echo "Example: $0 basic/hello_world -np 4"
    echo ""
    echo "Available programs:"
    find "$UNINSTRUMENTED_DIR" -type f -executable | sed "s|$UNINSTRUMENTED_DIR/||" | sort
    exit 1
fi

PROGRAM="$1"
shift

if [ ! -f "$UNINSTRUMENTED_DIR/$PROGRAM" ]; then
    echo "Error: Program $PROGRAM not found"
    exit 1
fi

echo "Running uninstrumented: $PROGRAM"
mpirun "$@" "$UNINSTRUMENTED_DIR/$PROGRAM"
