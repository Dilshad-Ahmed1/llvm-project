// Simplified test without MPI headers - just function pointer patterns
typedef int MPI_Comm;
typedef int MPI_Datatype;

// Mock MPI function declarations
int MPI_Send(void* buf, int count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm);
int MPI_Isend(void* buf, int count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm);
int MPI_Init(int* argc, char*** argv);
int MPI_Finalize(void);

// Constants
#define MPI_INT 1
#define MPI_COMM_WORLD 0

// Test case 1: Direct function pointer assignment
void test_direct_function_pointer() {
    int (*mpi_func_ptr)(void*, int, MPI_Datatype, int, int, MPI_Comm) = MPI_Send;
    
    // This should be detected as an indirect MPI call
    mpi_func_ptr(0, 0, MPI_INT, 0, 0, MPI_COMM_WORLD);
}

// Test case 2: Function pointer through variable
void test_function_pointer_variable() {
    int (*send_func)(void*, int, MPI_Datatype, int, int, MPI_Comm);
    send_func = MPI_Send;
    
    // This should be detected as an indirect MPI call
    send_func(0, 0, MPI_INT, 0, 0, MPI_COMM_WORLD);
}

// Test case 3: Function pointer array
void test_function_pointer_array() {
    int (*mpi_funcs[])(void*, int, MPI_Datatype, int, int, MPI_Comm) = {
        MPI_Send,
        MPI_Isend
    };
    
    // This should be detected as an indirect MPI call
    mpi_funcs[0](0, 0, MPI_INT, 0, 0, MPI_COMM_WORLD);
}

// Test case 4: Conditional function pointer selection
void test_conditional_function_pointer(int use_blocking) {
    int (*send_func)(void*, int, MPI_Datatype, int, int, MPI_Comm);
    
    if (use_blocking) {
        send_func = MPI_Send;
    } else {
        send_func = MPI_Isend;
    }
    
    // This should be detected as an indirect MPI call
    send_func(0, 0, MPI_INT, 0, 0, MPI_COMM_WORLD);
}

int main() {
    MPI_Init(0, 0);
    
    test_direct_function_pointer();
    test_function_pointer_variable();
    test_function_pointer_array();
    test_conditional_function_pointer(1);
    
    MPI_Finalize();
    return 0;
}