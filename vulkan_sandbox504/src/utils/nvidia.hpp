#pragma once
#ifndef __linux__
#ifdef _WIN32
#include <iostream>
#include <mutex>
#include <atomic>
#include <vector>
namespace NV {
inline bool hasNvidiaGPU = false;
// Define NVML types and error codes
typedef int nvmlReturn_t;
typedef void *nvmlDevice_t;
#define NVML_SUCCESS 0

// Error string function prototype for better error messages
typedef const char *(*NvmlErrorString_t)(nvmlReturn_t);

typedef nvmlReturn_t (*PFN_NvmlInit_t)();
typedef nvmlReturn_t (*PFN_NvmlShutdown_t)();
typedef nvmlReturn_t (*PFN_NvmlDeviceGetHandleByIndex_t)(unsigned int, nvmlDevice_t *);
typedef nvmlReturn_t (*PFN_NvmlDeviceGetPowerUsage_t)(nvmlDevice_t, unsigned int *);

// Double buffers to store the current GPU power draw
inline float currentPowerDrawBuffer[2] = {0.0f, 0.0f};
inline std::atomic<int> currentBufferIndex(0);  // Index to track which buffer is active for reading
inline std::mutex bufferMutex;  // Mutex to protect buffer swapping
inline std::atomic<bool> stopMonitoring(false);  // Flag to stop the monitoring thread
 

int getNvidiaGpuIndex();
bool loadNVMLFunctions();
void unloadNVML();
bool initializeNVML();
void shutdownNVML();
float getGPUPowerDraw();
void monitorPowerDraw();
void startMonitoringThread();
void stopMonitoringThread();
float getCurrentPowerDraw();
} // namespace NV
#endif
#endif
