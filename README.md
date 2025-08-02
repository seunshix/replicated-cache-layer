# FTCache: Fault-Tolerant Distributed Cache for HPC Systems

## Overview

FTCache is a fault-tolerant, distributed caching layer designed for high-performance computing (HPC) environments. It leverages consistent hashing and a hash ring architecture to provide load balancing and resilience against node failures. The system is implemented in C++ and supports integration with local and parallel file systems.

## Features

- Consistent Hash Ring for dynamic node management
- Fault tolerance with automatic failover
- Load balancing across multiple cache nodes
- Support for both local and parallel file systems (PFS)
- Performance benchmarking and result visualization scripts

## Project Structure

```
.
├── src/                # Source code (C++ files, hash ring, main entry points)
│   ├── ConsistentHashRing.hpp
│   ├── v1_main.cpp ... v6_main.cpp
│   ├── Local_Cache/    # Simulated local cache nodes
│   └── PFS_Source/     # Parallel file system source files
├── Archive/            # Old presentations, scripts, and results
├── build/              # Build artifacts and CMake files
├── Presentations/      # Project presentations and scripts
├── Paper/              # Research paper drafts and references
├── README.md           # This file
├── CMakeLists.txt      # CMake build configuration
└── ...                 # Other supporting files
```

## Getting Started

### Prerequisites

- C++17 compatible compiler (e.g., g++, clang++)
- CMake 3.10+
- Python 3.x (for plotting and analysis scripts)
- (Optional) Docker (for containerized node simulation)

### Build Instructions

1. Clone the repository:
   ```sh
   git clone https://github.com/seunshix/replicated-cache-layer.git
   cd FTCache
   ```

2. Build with CMake:
   ```sh
   mkdir -p build
   cd build
   cmake ..
   make
   ```

3. Or, build a specific version directly:
   ```sh
   g++ -std=c++17 -o v1_main ../src/v1_main.cpp
   ```

### Running Tests

- To run the main cache simulation:
  ```sh
  ./build/FTCache
  ```
- To run a specific version:
  ```sh
  ./src/v2_main
  ```

### Plotting Results

- Use the provided Python scripts to visualize results:
  ```sh
  python3 src/plotting_results.py
  ```

## Usage

- Configure cache nodes and PFS sources in the `src/Local_Cache/` and `src/PFS_Source/` directories.
- Modify or extend the hash ring logic in `src/ConsistentHashRing.hpp`.

## Documentation

- See the `Presentations/` and `Paper/` folders for methodology, results, and research context.
- Refer to code comments in `src/` for implementation details.

## Contributing

Contributions are welcome! Please open issues or submit pull requests for improvements or bug fixes.

## License

This project is for academic and research purposes.
