/**
 * @file mpi_cpp_example.cpp
 * @brief C++ MPI example demonstrating object-oriented MPI usage
 * 
 * This example shows C++ MPI programming patterns and demonstrates how the
 * MPI Usage Sanitizer handles C++ language features, templates, and
 * object-oriented MPI code.
 * 
 * Expected instrumentation:
 * - C++ name mangling and template instantiation handling
 * - Object-oriented MPI wrapper validation
 * - STL container integration with MPI
 * - Exception handling in MPI context
 */

#include <mpi.h>
#include <iostream>
#include <vector>
#include <string>
#include <algorithm>
#include <memory>
#include <stdexcept>

// Template class for MPI communication wrapper
template<typename T>
class MPICommunicator {
private:
    MPI_Comm comm_;
    int rank_;
    int size_;
    
public:
    MPICommunicator(MPI_Comm comm = MPI_COMM_WORLD) : comm_(comm) {
        MPI_Comm_rank(comm_, &rank_);
        MPI_Comm_size(comm_, &size_);
    }
    
    int getRank() const { return rank_; }
    int getSize() const { return size_; }
    MPI_Comm getComm() const { return comm_; }
    
    // Template method for sending STL containers
    // The sanitizer will instrument template instantiations to validate:
    // - Template parameter type compatibility with MPI datatypes
    // - STL container memory layout and contiguity
    // - C++ object serialization requirements
    void send(const std::vector<T>& data, int dest, int tag = 0) {
        if (data.empty()) {
            throw std::invalid_argument("Cannot send empty vector");
        }
        
        MPI_Datatype mpi_type = getMPIDatatype<T>();
        
        // MPI_Send will be instrumented to validate:
        // - STL vector data() pointer validity
        // - Template type T compatibility with MPI_Datatype
        // - C++ exception safety during MPI operations
        int result = MPI_Send(data.data(), 
                             static_cast<int>(data.size()), 
                             mpi_type, 
                             dest, 
                             tag, 
                             comm_);
        
        if (result != MPI_SUCCESS) {
            throw std::runtime_error("MPI_Send failed with code " + std::to_string(result));
        }
    }
    
    // Template method for receiving into STL containers
    std::vector<T> receive(int source, int tag = 0) {
        MPI_Status status;
        MPI_Datatype mpi_type = getMPIDatatype<T>();
        
        // Probe for message size first
        MPI_Probe(source, tag, comm_, &status);
        
        int count;
        MPI_Get_count(&status, mpi_type, &count);
        
        std::vector<T> data(count);
        
        // MPI_Recv will be instrumented to validate:
        // - STL vector automatic resizing and memory management
        // - Template type consistency between send and receive
        // - C++ RAII compliance during MPI operations
        int result = MPI_Recv(data.data(), 
                             count, 
                             mpi_type, 
                             source, 
                             tag, 
                             comm_, 
                             &status);
        
        if (result != MPI_SUCCESS) {
            throw std::runtime_error("MPI_Recv failed with code " + std::to_string(result));
        }
        
        return data;
    }
    
    // Collective operations with STL containers
    std::vector<T> allGather(const std::vector<T>& local_data) {
        if (local_data.empty()) {
            throw std::invalid_argument("Cannot gather empty vector");
        }
        
        MPI_Datatype mpi_type = getMPIDatatype<T>();
        int local_count = static_cast<int>(local_data.size());
        
        // Gather counts from all processes
        std::vector<int> all_counts(size_);
        MPI_Allgather(&local_count, 1, MPI_INT, 
                     all_counts.data(), 1, MPI_INT, comm_);
        
        // Calculate displacements
        std::vector<int> displs(size_);
        int total_count = 0;
        for (int i = 0; i < size_; ++i) {
            displs[i] = total_count;
            total_count += all_counts[i];
        }
        
        std::vector<T> result(total_count);
        
        // MPI_Allgatherv will be instrumented to validate:
        // - Variable-length STL container handling
        // - Dynamic memory allocation during collective operations
        // - C++ container iterator validity
        int mpi_result = MPI_Allgatherv(local_data.data(), local_count, mpi_type,
                                       result.data(), all_counts.data(), 
                                       displs.data(), mpi_type, comm_);
        
        if (mpi_result != MPI_SUCCESS) {
            throw std::runtime_error("MPI_Allgatherv failed");
        }
        
        return result;
    }
    
private:
    // Template specialization for MPI datatypes
    template<typename U>
    MPI_Datatype getMPIDatatype() {
        if constexpr (std::is_same_v<U, int>) {
            return MPI_INT;
        } else if constexpr (std::is_same_v<U, double>) {
            return MPI_DOUBLE;
        } else if constexpr (std::is_same_v<U, float>) {
            return MPI_FLOAT;
        } else {
            static_assert(std::is_same_v<U, int> || 
                         std::is_same_v<U, double> || 
                         std::is_same_v<U, float>, 
                         "Unsupported type for MPI communication");
            return MPI_DATATYPE_NULL;
        }
    }
};

// RAII wrapper for MPI initialization
class MPIEnvironment {
public:
    MPIEnvironment(int argc, char* argv[]) {
        int provided;
        int result = MPI_Init_thread(&argc, &argv, MPI_THREAD_SINGLE, &provided);
        if (result != MPI_SUCCESS) {
            throw std::runtime_error("MPI_Init_thread failed");
        }
        
        MPI_Comm_rank(MPI_COMM_WORLD, &rank_);
        MPI_Comm_size(MPI_COMM_WORLD, &size_);
        
        if (rank_ == 0) {
            std::cout << "MPI Environment initialized with " << size_ 
                      << " processes" << std::endl;
        }
    }
    
    ~MPIEnvironment() {
        try {
            MPI_Finalize();
        } catch (...) {
            // Don't throw from destructor
        }
    }
    
    int getRank() const { return rank_; }
    int getSize() const { return size_; }
    
private:
    int rank_;
    int size_;
};

// Custom MPI data structure
struct ParticleData {
    double x, y, z;
    double vx, vy, vz;
    int id;
    
    ParticleData() = default;
    ParticleData(int particle_id, double pos_x, double pos_y, double pos_z)
        : x(pos_x), y(pos_y), z(pos_z), vx(0), vy(0), vz(0), id(particle_id) {}
};

// Function to create MPI datatype for custom structure
MPI_Datatype createParticleDatatype() {
    MPI_Datatype particle_type;
    int block_lengths[] = {3, 3, 1};  // 3 doubles, 3 doubles, 1 int
    MPI_Aint displacements[3];
    MPI_Datatype types[] = {MPI_DOUBLE, MPI_DOUBLE, MPI_INT};
    
    // Calculate displacements
    displacements[0] = offsetof(ParticleData, x);
    displacements[1] = offsetof(ParticleData, vx);
    displacements[2] = offsetof(ParticleData, id);
    
    // Create the MPI datatype
    // MPI_Type_create_struct will be instrumented to validate:
    // - C++ struct layout and padding
    // - Memory alignment requirements
    // - Type safety for custom datatypes
    MPI_Type_create_struct(3, block_lengths, displacements, types, &particle_type);
    MPI_Type_commit(&particle_type);
    
    return particle_type;
}

// Demonstration function for advanced C++ MPI patterns
void demonstrateAdvancedPatterns(const MPIEnvironment& env) {
    MPICommunicator<double> comm;
    
    std::cout << "Process " << env.getRank() 
              << ": Demonstrating advanced C++ MPI patterns" << std::endl;
    
    // Example 1: STL container communication
    if (env.getSize() >= 2) {
        if (env.getRank() == 0) {
            std::cout << "--- STL Container Communication ---" << std::endl;
            
            // Create and send a vector of doubles
            std::vector<double> data = {1.1, 2.2, 3.3, 4.4, 5.5};
            std::cout << "Process 0: Sending vector of size " << data.size() << std::endl;
            
            try {
                comm.send(data, 1, 100);
                std::cout << "Process 0: Vector sent successfully" << std::endl;
            } catch (const std::exception& e) {
                std::cerr << "Process 0: Send failed: " << e.what() << std::endl;
            }
            
        } else if (env.getRank() == 1) {
            try {
                auto received_data = comm.receive(0, 100);
                std::cout << "Process 1: Received vector of size " 
                          << received_data.size() << std::endl;
                std::cout << "Process 1: Data: ";
                for (const auto& val : received_data) {
                    std::cout << val << " ";
                }
                std::cout << std::endl;
            } catch (const std::exception& e) {
                std::cerr << "Process 1: Receive failed: " << e.what() << std::endl;
            }
        }
    }
    
    MPI_Barrier(MPI_COMM_WORLD);
    
    // Example 2: Collective operations with STL containers
    if (env.getRank() == 0) {
        std::cout << "--- STL Container Collective Operations ---" << std::endl;
    }
    
    // Each process contributes different amount of data
    std::vector<int> local_data;
    for (int i = 0; i < env.getRank() + 1; ++i) {
        local_data.push_back(env.getRank() * 100 + i);
    }
    
    std::cout << "Process " << env.getRank() << ": Contributing " 
              << local_data.size() << " elements" << std::endl;
    
    try {
        MPICommunicator<int> int_comm;
        auto gathered_data = int_comm.allGather(local_data);
        
        std::cout << "Process " << env.getRank() 
                  << ": Gathered total of " << gathered_data.size() 
                  << " elements" << std::endl;
        
        if (env.getRank() == 0) {
            std::cout << "All gathered data: ";
            for (size_t i = 0; i < std::min(gathered_data.size(), size_t(20)); ++i) {
                std::cout << gathered_data[i] << " ";
            }
            if (gathered_data.size() > 20) std::cout << "...";
            std::cout << std::endl;
        }
    } catch (const std::exception& e) {
        std::cerr << "Process " << env.getRank() 
                  << ": AllGather failed: " << e.what() << std::endl;
    }
    
    MPI_Barrier(MPI_COMM_WORLD);
    
    // Example 3: Custom datatype communication
    if (env.getSize() >= 2) {
        if (env.getRank() == 0) {
            std::cout << "--- Custom Datatype Communication ---" << std::endl;
        }
        
        MPI_Datatype particle_type = createParticleDatatype();
        
        if (env.getRank() == 0) {
            // Create and send particle data
            std::vector<ParticleData> particles;
            for (int i = 0; i < 5; ++i) {
                particles.emplace_back(i, i * 1.0, i * 2.0, i * 3.0);
            }
            
            std::cout << "Process 0: Sending " << particles.size() 
                      << " particles" << std::endl;
            
            // MPI_Send with custom datatype will be instrumented to validate:
            // - Custom MPI datatype correctness
            // - C++ object serialization
            // - Memory layout compatibility
            MPI_Send(particles.data(), static_cast<int>(particles.size()), 
                    particle_type, 1, 200, MPI_COMM_WORLD);
            
        } else if (env.getRank() == 1) {
            std::vector<ParticleData> received_particles(5);
            
            MPI_Recv(received_particles.data(), 5, particle_type, 0, 200, 
                    MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            
            std::cout << "Process 1: Received particles:" << std::endl;
            for (const auto& p : received_particles) {
                std::cout << "  Particle " << p.id << ": pos(" 
                          << p.x << "," << p.y << "," << p.z << ")" << std::endl;
            }
        }
        
        MPI_Type_free(&particle_type);
    }
}

int main(int argc, char* argv[]) {
    try {
        // RAII MPI environment management
        MPIEnvironment env(argc, argv);
        
        std::cout << "Process " << env.getRank() 
                  << ": C++ MPI Example Started" << std::endl;
        
        // Demonstrate advanced C++ MPI patterns
        demonstrateAdvancedPatterns(env);
        
        MPI_Barrier(MPI_COMM_WORLD);
        
        if (env.getRank() == 0) {
            std::cout << "\n=== C++ MPI Features Demonstrated ===" << std::endl;
            std::cout << "1. Template-based MPI communication wrappers" << std::endl;
            std::cout << "2. STL container integration with MPI" << std::endl;
            std::cout << "3. RAII-based MPI resource management" << std::endl;
            std::cout << "4. Exception handling in MPI context" << std::endl;
            std::cout << "5. Custom MPI datatypes for C++ structures" << std::endl;
            std::cout << "6. Object-oriented MPI programming patterns" << std::endl;
        }
        
        // MPIEnvironment destructor will call MPI_Finalize
        
    } catch (const std::exception& e) {
        std::cerr << "Exception caught: " << e.what() << std::endl;
        MPI_Abort(MPI_COMM_WORLD, EXIT_FAILURE);
    }
    
    return EXIT_SUCCESS;
}

/*
 * Expected Sanitizer Output (C++ specific):
 * 
 * [MPI Sanitizer] Pre-call: MPI_Init_thread at mpi_cpp_example.cpp:95
 * [MPI Sanitizer]   - C++ binding detected
 * [MPI Sanitizer]   - Thread support requested: MPI_THREAD_SINGLE
 * [MPI Sanitizer]   - RAII pattern detected (MPIEnvironment constructor)
 * [MPI Sanitizer] Post-call: MPI_Init_thread returned MPI_SUCCESS
 * [MPI Sanitizer]   - Thread support provided: MPI_THREAD_SINGLE
 * 
 * [MPI Sanitizer] Pre-call: MPI_Send at mpi_cpp_example.cpp:45
 * [MPI Sanitizer]   - C++ template instantiation: MPICommunicator<double>::send
 * [MPI Sanitizer]   - STL container detected: std::vector<double>
 * [MPI Sanitizer]   - Container size: 5 elements
 * [MPI Sanitizer]   - Memory layout: Contiguous (std::vector guarantees)
 * [MPI Sanitizer]   - Template type: double → MPI_DOUBLE
 * [MPI Sanitizer]   - Buffer: std::vector::data() = 0x7f8b4c000b20
 * [MPI Sanitizer] Post-call: MPI_Send returned MPI_SUCCESS
 * [MPI Sanitizer]   - STL container communication: SUCCESSFUL
 * 
 * [MPI Sanitizer] Pre-call: MPI_Probe at mpi_cpp_example.cpp:60
 * [MPI Sanitizer]   - C++ exception safety: ENABLED
 * [MPI Sanitizer]   - Dynamic sizing for STL container
 * [MPI Sanitizer] Post-call: MPI_Probe returned MPI_SUCCESS
 * [MPI Sanitizer]   - Message size: 5 doubles (40 bytes)
 * 
 * [MPI Sanitizer] Pre-call: MPI_Recv at mpi_cpp_example.cpp:68
 * [MPI Sanitizer]   - STL container automatic resizing detected
 * [MPI Sanitizer]   - std::vector<double> resized to 5 elements
 * [MPI Sanitizer]   - RAII compliance: Memory automatically managed
 * [MPI Sanitizer] Post-call: MPI_Recv returned MPI_SUCCESS
 * 
 * [MPI Sanitizer] Pre-call: MPI_Allgatherv at mpi_cpp_example.cpp:95
 * [MPI Sanitizer]   - Template instantiation: MPICommunicator<int>::allGather
 * [MPI Sanitizer]   - Variable-length collective with STL containers
 * [MPI Sanitizer]   - Dynamic memory allocation: std::vector management
 * [MPI Sanitizer]   - Process contributions: [1, 2, 3, 4] elements
 * [MPI Sanitizer] Post-call: MPI_Allgatherv returned MPI_SUCCESS
 * [MPI Sanitizer]   - Total gathered: 10 elements
 * 
 * [MPI Sanitizer] Pre-call: MPI_Type_create_struct at mpi_cpp_example.cpp:165
 * [MPI Sanitizer]   - Custom C++ datatype creation
 * [MPI Sanitizer]   - Struct: ParticleData (56 bytes)
 * [MPI Sanitizer]   - Field analysis:
 * [MPI Sanitizer]     * x,y,z: 3 × MPI_DOUBLE at offset 0
 * [MPI Sanitizer]     * vx,vy,vz: 3 × MPI_DOUBLE at offset 24
 * [MPI Sanitizer]     * id: 1 × MPI_INT at offset 48
 * [MPI Sanitizer]   - Padding analysis: 4 bytes at end (alignment)
 * [MPI Sanitizer] Post-call: MPI_Type_create_struct returned MPI_SUCCESS
 * 
 * [MPI Sanitizer] Pre-call: MPI_Send at mpi_cpp_example.cpp:195
 * [MPI Sanitizer]   - Custom datatype communication
 * [MPI Sanitizer]   - C++ object serialization: ParticleData
 * [MPI Sanitizer]   - Object count: 5, Total size: 280 bytes
 * [MPI Sanitizer]   - Memory layout validation: PASSED
 * [MPI Sanitizer] Post-call: MPI_Send returned MPI_SUCCESS
 * 
 * [MPI Sanitizer] RAII Destructor: MPIEnvironment::~MPIEnvironment
 * [MPI Sanitizer]   - Automatic MPI_Finalize called
 * [MPI Sanitizer]   - Resource cleanup: SUCCESSFUL
 * [MPI Sanitizer]   - Exception safety: No exceptions during cleanup
 * 
 * [MPI Sanitizer] C++ Features Summary:
 * [MPI Sanitizer]   - Template instantiations monitored: 3
 * [MPI Sanitizer]   - STL container operations: 4
 * [MPI Sanitizer]   - Custom datatypes created: 1
 * [MPI Sanitizer]   - RAII patterns detected: 2
 * [MPI Sanitizer]   - Exception safety: MAINTAINED
 * [MPI Sanitizer]   - Memory management: Automatic (STL + RAII)
 */