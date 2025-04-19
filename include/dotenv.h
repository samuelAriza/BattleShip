#pragma once
#include <cstdlib>
#include <fstream>
#include <sstream>
#include <string>

namespace dotenv {
    inline void init(const std::string& path = ".env") {
        std::ifstream file(path);
        if (!file) return;
        std::string line;
        while (std::getline(file, line)) {
            size_t eq = line.find('=');
            if (eq != std::string::npos) {
                std::string key = line.substr(0, eq);
                std::string value = line.substr(eq + 1);
                setenv(key.c_str(), value.c_str(), 1);
            }
        }
    }
}
