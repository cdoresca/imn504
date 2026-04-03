#ifndef __linux__
#include "nvidia.hpp"
#include <windows.h>
#include <thread>
#include <atomic>
#include <mutex>

namespace NV {

inline NvmlErrorString_t nvmlErrorStringPtr = nullptr;

// Function pointers for dynamically loaded NVML functions
PFN_NvmlInit_t nvmlInitPtr = nullptr;
PFN_NvmlShutdown_t nvmlShutdownPtr = nullptr;
PFN_NvmlDeviceGetHandleByIndex_t nvmlDeviceGetHandleByIndexPtr = nullptr;
PFN_NvmlDeviceGetPowerUsage_t nvmlDeviceGetPowerUsagePtr = nullptr;

// Handle for the loaded DLL
HMODULE nvmlLib = nullptr;
nvmlDevice_t device;


int getNvidiaGpuIndex() {
    DISPLAY_DEVICEA dd;
    dd.cb = sizeof(dd);
    int deviceIndex = 0;
    while (EnumDisplayDevicesA(nullptr, deviceIndex, &dd, 0)) {
        if (strstr(dd.DeviceString, "NVIDIA")) {
            hasNvidiaGPU = true;
            return deviceIndex; // Return the index of the first NVIDIA GPU found
        }
        deviceIndex++;
    }
    return -1; // No NVIDIA GPU found
}

bool loadNVMLFunctions() {
    nvmlLib = LoadLibraryA("nvml.dll");
    if (!nvmlLib) {
        std::cerr << "Failed to load nvml.dll." << std::endl;
        return false;
    }

    nvmlInitPtr = (PFN_NvmlInit_t)GetProcAddress(nvmlLib, "nvmlInit_v2");
    nvmlShutdownPtr = (PFN_NvmlShutdown_t)GetProcAddress(nvmlLib, "nvmlShutdown");
    nvmlDeviceGetHandleByIndexPtr = (PFN_NvmlDeviceGetHandleByIndex_t)GetProcAddress(nvmlLib, "nvmlDeviceGetHandleByIndex_v2");
    nvmlDeviceGetPowerUsagePtr = (PFN_NvmlDeviceGetPowerUsage_t)GetProcAddress(nvmlLib, "nvmlDeviceGetPowerUsage");
    nvmlErrorStringPtr = (NvmlErrorString_t)GetProcAddress(nvmlLib, "nvmlErrorString");

    if (!nvmlInitPtr || !nvmlShutdownPtr || !nvmlDeviceGetHandleByIndexPtr || !nvmlDeviceGetPowerUsagePtr || !nvmlErrorStringPtr) {
        std::cerr << "Failed to get NVML function addresses." << std::endl;
        unloadNVML();
        return false;
    }

    return true;
}

void unloadNVML() {
    if (nvmlLib) {
        FreeLibrary(nvmlLib);
        nvmlLib = nullptr;
    }
}

bool initializeNVML() {
    if (!loadNVMLFunctions()) {
        return false;
    }

    nvmlReturn_t result = nvmlInitPtr();
    if (result != NVML_SUCCESS) {
        std::cerr << "Failed to initialize NVML: " << nvmlErrorStringPtr(result) << std::endl;
        unloadNVML();
        return false;
    }

    // Initialize device handle after NVML initialization
    int nvidiaGpuIndex = getNvidiaGpuIndex();
    if (nvidiaGpuIndex == -1) {
        std::cerr << "No NVIDIA GPU detected." << std::endl;
        shutdownNVML();
        return false;
    }

    result = nvmlDeviceGetHandleByIndexPtr(nvidiaGpuIndex, &device);
    if (result != NVML_SUCCESS) {
        std::cerr << "Failed to get handle for device: " << nvmlErrorStringPtr(result) << std::endl;
        shutdownNVML();
        return false;
    }

    return true;
}

void shutdownNVML() {
    if (nvmlShutdownPtr) {
        nvmlReturn_t result = nvmlShutdownPtr();
        if (result != NVML_SUCCESS) {
            std::cerr << "Failed to shut down NVML: " << nvmlErrorStringPtr(result) << std::endl;
        }
    }
    unloadNVML();
}

float getGPUPowerDraw() {
    if (!nvmlDeviceGetPowerUsagePtr) {
        std::cerr << "NVML function pointer for nvmlDeviceGetPowerUsage is null." << std::endl;
        return 0.0f;
    }

    unsigned int powerDraw;
    nvmlReturn_t result = nvmlDeviceGetPowerUsagePtr(device, &powerDraw);
    if (result != NVML_SUCCESS) {
        std::cerr << "Failed to get power usage: " << nvmlErrorStringPtr(result) << std::endl;
        return 0.0f;
    } else {
        return powerDraw / 1000.0f; // Convert to Watts
    }
}

// Monitoring thread function to update current power draw
void monitorPowerDraw() {
    while (!stopMonitoring) {
        float powerDraw = getGPUPowerDraw();

        // Update the inactive buffer with the latest power draw value
        int nextBufferIndex = (currentBufferIndex.load() + 1) % 2;  // Determine the next buffer to update
        {
            std::lock_guard<std::mutex> lock(bufferMutex);
            currentPowerDrawBuffer[nextBufferIndex] = powerDraw;  // Update the inactive buffer
        }

        // Swap the buffers by updating the index
        currentBufferIndex.store(nextBufferIndex);

        // Sleep for a while to reduce the frequency of updates (adjust as needed)
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}

// Start the monitoring thread
void startMonitoringThread() {
    stopMonitoring = false;
    std::thread(monitorPowerDraw).detach();  // Start the thread and detach it
}

// Stop the monitoring thread
void stopMonitoringThread() {
    stopMonitoring = true;
}

float getCurrentPowerDraw() {
    std::lock_guard<std::mutex> lock(bufferMutex);
    return currentPowerDrawBuffer[currentBufferIndex.load()];
}
} // namespace NV
#endif