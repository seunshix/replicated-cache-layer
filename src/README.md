# FTCache: Fault-Tolerant Caching Layer Demo

This project implements a fault-tolerant, consistent-hash based caching layer in C++ for simulated HPC deep-learning workloads. It demonstrates:

1. **Data Caching**: Node-local caching using a consistent hash ring to assign file ownership.
2. **TTL Eviction**: Time-to-live based cleanup of stale cache entries.
3. **Fault-Tolerant Recache**: Simulated node failures (removeNode) and recaching lost files.
4. **Rebalancing**: Node re-addition (addNode) and moving files to maintain even distribution.

---

## Prerequisites

- **C++17** compiler (e.g., `g++`, Clang)
- **CMake** ≥ 3.10
- Unix-like environment (Linux, macOS)

---

## Directory Structure

```text
FTCache/            # Project root
├── CMakeLists.txt   # Build configuration
├── README.md        # This guide
├── src/             # Source files
│   ├── main.cpp
│   ├── ConsistentHashRing.hpp
│   └── ...
└── tests/           # (Optional) unit tests
```

---

## Build & Demo

1. **Create build directory**
   ```bash
   mkdir -p build && cd build
   ```

2. **Configure & build**
   ```bash
   cmake ..
   make FTCache
   ```

3. **Run the demo**
   ```bash
   # Default TTL=10s, fail-node=NodeB
   make demo
   ```

4. **Run tests** (uses CTest)
   ```bash
   make run_tests
   ```

---

## Command-Line Options

`FTCache` supports the following flags:

| Option               | Description                                       | Default  |
|----------------------|---------------------------------------------------|----------|
| `--ttl <seconds>`    | TTL (time-to-live) for cache eviction in seconds | `10`     |
| `--fail-node <node>` | Node ID to simulate failure                      | `NodeB`  |
| `--help`             | Show usage and exit                              | N/A      |

**Example:**
```bash
./FTCache --ttl 5 --fail-node NodeC
```

---

## Workflow Overview

1. **Initial Caching**: Reads files from `PFS_Source/`, assigns each to a node, and writes to `Local_Cache/<NodeID>/`.
2. **Simulate Failure**: Removes `NodeB` (or configured node) from the hash ring and deletes its on-disk cache folder.
3. **Recache**: Pulls missing files back from `PFS_Source/` into the updated ring’s node folders.
4. **Rebalance**: Adds the failed node back to the ring and moves any out-of-place files to restore balance.
5. **Eviction**: (Optional) Removes any files older than TTL seconds.

---

## Extensibility

- Add size-based eviction (LRU, maximum cache size).
- Integrate real node-health checks instead of simulated failures.
- Persist metadata to survive restarts.

Feel free to fork or modify for your own experiments!

