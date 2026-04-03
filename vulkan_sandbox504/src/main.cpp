#include <filesystem>
#include <iostream>

// #include "thirdparty_implementation.cpp"

#include "application/Application.hpp"
#include "application/SubappsList.hpp" // subapp registration
#include "config.hpp"

// #include "utils/nvidia.hpp"

int main() {
    setWorkingDirectoryToProjectRoot();
    std::cout << "Working directory set to: " << std::filesystem::current_path() << '\n';
    // if (NV::initializeNVML()) {std::cout << "NVML loaded successfully." << std::endl;}

    ApplicationConfig config = loadApplicationConfigFromFile("appconfig.json");
    Application app(config);

    try {
        app.run();
    } catch (const std::exception &e) {
        std::cerr << "[Error] " << e.what() << '\n';
        // if (NV::hasNvidiaGPU) { NV::shutdownNVML(); }
        return 1;
    }
    // if (NV::hasNvidiaGPU) { NV::shutdownNVML(); }
    return 0;
}