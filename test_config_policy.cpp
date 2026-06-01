#include "llvm/Transforms/Instrumentation/MPIUsageSanitizer/ConfigurationManager.h"
#include "llvm/Transforms/Instrumentation/MPIUsageSanitizer/MPICallDetector.h"
#include <iostream>

using namespace llvm;

int main() {
    // Create test options
    MPIUsageSanitizerOptions Options;
    Options.Level = MPIUsageSanitizerOptions::InstrumentationLevel::Lightweight;
    Options.EnableOptimizations = true;
    
    // Create configuration manager
    ConfigurationManager ConfigMgr(Options);
    ConfigMgr.initialize();
    
    // Test lightweight mode filtering
    std::cout << "Testing Lightweight Mode Policy Controls:\n";
    
    // Create test call sites
    CallSite SendSite;
    SendSite.FunctionName = "MPI_Send";
    SendSite.Type = MPIFunctionType::PointToPoint;
    
    CallSite InfoSite;
    InfoSite.FunctionName = "MPI_Info_create";
    InfoSite.Type = MPIFunctionType::Info;
    
    CallSite BarrierSite;
    BarrierSite.FunctionName = "MPI_Barrier";
    BarrierSite.Type = MPIFunctionType::Collective;
    
    CallSite GatherSite;
    GatherSite.FunctionName = "MPI_Gather";
    GatherSite.Type = MPIFunctionType::Collective;
    
    // Test instrumentation decisions
    std::cout << "MPI_Send (P2P): " << (ConfigMgr.shouldInstrument(SendSite) ? "INSTRUMENT" : "SKIP") << "\n";
    std::cout << "MPI_Info_create (Info): " << (ConfigMgr.shouldInstrument(InfoSite) ? "INSTRUMENT" : "SKIP") << "\n";
    std::cout << "MPI_Barrier (Collective): " << (ConfigMgr.shouldInstrument(BarrierSite) ? "INSTRUMENT" : "SKIP") << "\n";
    std::cout << "MPI_Gather (Collective): " << (ConfigMgr.shouldInstrument(GatherSite) ? "INSTRUMENT" : "SKIP") << "\n";
    
    // Test category controls
    std::cout << "\nTesting Category Controls:\n";
    ConfigMgr.disableMPIOperationCategory(MPIFunctionType::Info);
    std::cout << "After disabling Info category:\n";
    std::cout << "MPI_Info_create: " << (ConfigMgr.shouldInstrument(InfoSite) ? "INSTRUMENT" : "SKIP") << "\n";
    
    // Test mode queries
    std::cout << "\nMode Status:\n";
    std::cout << "Is Lightweight Mode: " << (ConfigMgr.isLightweightMode() ? "YES" : "NO") << "\n";
    std::cout << "Is Full Mode: " << (ConfigMgr.isFullMode() ? "YES" : "NO") << "\n";
    std::cout << "Is Performance Mode: " << (ConfigMgr.isPerformanceMode() ? "YES" : "NO") << "\n";
    
    return 0;
}