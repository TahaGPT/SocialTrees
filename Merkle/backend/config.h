#ifndef CONFIG_H
#define CONFIG_H

#include <string>

namespace Config {
    // Server Configuration
    const std::string SERVER_HOST = "0.0.0.0";
    const int SERVER_PORT = 8080;
    
    // File Paths
    const std::string DATASET_DIR = "../Dataset/";
    const std::string ROOT_HISTORY_FILE = "merkle_roots.txt";
    
    // Tree Configuration
    const int TREE_ARITY = 4; // Quad-tree (4-ary)
    const int DEFAULT_MAX_DEPTH = 10;
    
    // Performance Configuration
    const int DEFAULT_THREAD_COUNT = 12; // Can be overridden by system
}

#endif // CONFIG_H
