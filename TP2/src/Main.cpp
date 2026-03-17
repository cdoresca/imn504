
#include <cstdlib>
#include <iostream>

#include "Application.h"

#include <iostream>

#define ROOT_IDENTIFIER ".root"
#include <filesystem>
void setWorkingDirectoryToProjectRoot() {
    std::filesystem::path exePath = std::filesystem::current_path();
    unsigned int maxDepth = 10;
    while (exePath.has_parent_path() && !std::filesystem::exists(exePath / ROOT_IDENTIFIER) && maxDepth > 0) {
        exePath = exePath.parent_path();
        maxDepth--;
    }

    if (!std::filesystem::exists(exePath / ROOT_IDENTIFIER)) {
        std::cerr << "Project root not found."
                  << "Are you running the executable from the project folder?"
                  << std::endl;
        std::exit(EXIT_FAILURE);
    }

    std::filesystem::current_path(exePath);
}

int main(int argc, char **argv) {
    setWorkingDirectoryToProjectRoot();

    try {
        Application app;
        app.mainLoop();
    } catch (const std::runtime_error &e) {
        std::string error_msg = std::string("Caught a fatal error: ") + std::string(e.what());
        std::cout << error_msg << std::endl;
        return -1;
    }
    return 0;
}
