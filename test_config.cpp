#include "llvm-project/llvm/lib/Transforms/Instrumentation/MPIUsageSanitizer/ConfigurationManager.h"
#include "llvm-project/llvm/include/llvm/Transforms/Instrumentation/MPIUsageSanitizer.h"
#include <iostream>

using namespace llvm;

int main() {
    // Test basic configuration manager functionality
    MPIUsageSanitizerOptions options;
    options.Level = MPIUsageSanitizerOptions::InstrumentationLevel::Full;
    options.EnableOptimizations = true;
    options.EnablePerformanceMonitoring = false;
    
    ConfigurationManager configMgr(options);
    
    if (configMgr.initialize()) {
        std::cout << "ConfigurationManager initialized successfully!" << std::endl;
        
        // Test function instrumentation decisions
        bool shouldInstrument = configMgr.shouldInstrument("MPI_Send");
        std::cout << "Should instrument MPI_Send: " << (shouldInstrument ? "Yes" : "No") << std::endl;
        
        shouldInstrument = configMgr.shouldInstrument(MPIFunctionType::PointToPoint);
        std::cout << "Should instrument PointToPoint: " << (shouldInstrument ? "Yes" : "No") << std::endl;
        
        // Test instrumentation policy
        InstrumentationPolicy policy = configMgr.getInstrumentationPolicy("MPI_Bcast");
        std::cout << "MPI_Bcast policy - PreHooks: " << (policy.EnablePreHooks ? "Yes" : "No") 
                  << ", PostHooks: " << (policy.EnablePostHooks ? "Yes" : "No") << std::endl;
        
        std::cout << "ConfigurationManager test completed successfully!" << std::endl;
        return 0;
    } else {
        std::cout << "Failed to initialize ConfigurationManager" << std::endl;
        return 1;
    }
}