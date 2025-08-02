// #include <iostream>
// #include <fstream>
// #include <filesystem>
// #include <chrono>
// #include <thread>
// #include <unordered_map>
// #include <string>
// #include <vector>
// #include "ConsistentHashRing.hpp"

// namespace fs = std::filesystem;
// using Clock = std::chrono::high_resolution_clock;
// typedef Clock::time_point TimePoint;

// // Metadata for each cached file
// struct FileMetadata {
//     TimePoint timestamp;      // when file was cached
//     std::string fileKey;      // filename identifier
//     std::string owner;        // owning node identifier
// };

// // Global in-memory metadata map: filename -> metadata
// static std::unordered_map<std::string, FileMetadata> metadataMap;

// // Cache a file into node-local directory
// bool cacheFile(const fs::path& sourceFile,
//                const fs::path& cacheDir,
//                const ConsistentHashRing& ring) {
//     if (!fs::exists(sourceFile)) return false;
//     std::string owner = ring.getNodeForKey(sourceFile.string());
//     fs::path nodeDir = cacheDir / owner;
//     fs::create_directories(nodeDir);
//     std::string fileName = sourceFile.filename().string();
//     fs::path dest = nodeDir / fileName;
//     std::ifstream in(sourceFile, std::ios::binary);
//     std::ofstream out(dest, std::ios::binary);
//     if (!in || !out) return false;
//     out << in.rdbuf();
//     metadataMap[fileName] = { Clock::now(), fileName, owner };
//     return true;
// }

// // Recache missing files from PFS
// void recacheLostFiles(const fs::path& pfsDir,
//                       const fs::path& cacheDir,
//                       ConsistentHashRing& ring) {
//     for (auto& kv : metadataMap) {
//         const auto& meta = kv.second;
//         fs::path expected = cacheDir / meta.owner / meta.fileKey;
//         if (!fs::exists(expected)) {
//             cacheFile(pfsDir / meta.fileKey, cacheDir, ring);
//         }
//     }
// }

// // Pure PFS fallback: read each file into memory
// void simulatePFSFallback(const fs::path& pfsDir) {
//     std::vector<char> buf;
//     for (auto& kv : metadataMap) {
//         fs::path src = pfsDir / kv.first;
//         std::ifstream in(src, std::ios::binary);
//         if (!in) continue;
//         in.seekg(0, std::ios::end);
//         size_t n = in.tellg(); in.seekg(0, std::ios::beg);
//         buf.resize(n);
//         in.read(buf.data(), n);
//     }
// }

// // Move files to new owners after ring change
// int rebalanceCache(const fs::path& cacheDir, ConsistentHashRing& ring) {
//     int moved = 0;
//     for (auto& kv : metadataMap) {
//         auto& meta = kv.second;
//         std::string newOwner = ring.getNodeForKey(meta.fileKey);
//         if (newOwner != meta.owner) {
//             fs::path oldp = cacheDir / meta.owner / meta.fileKey;
//             fs::path newDir = cacheDir / newOwner;
//             fs::create_directories(newDir);
//             fs::path newp = newDir / meta.fileKey;
//             if (fs::exists(oldp)) {
//                 fs::rename(oldp, newp);
//                 meta.owner = newOwner;
//                 ++moved;
//             }
//         }
//     }
//     return moved;
// }

// // Evict cache entries older than TTL
// void evictExpired(const fs::path& cacheDir, int ttl) {
//     auto now = Clock::now();
//     for (auto it = metadataMap.begin(); it != metadataMap.end();) {
//         int age = std::chrono::duration_cast<std::chrono::seconds>(now - it->second.timestamp).count();
//         if (age >= ttl) {
//             fs::remove(cacheDir / it->second.owner / it->second.fileKey);
//             it = metadataMap.erase(it);
//         } else {
//             ++it;
//         }
//     }
// }

// // Simulate one epoch of reads
// long runEpoch(const std::string& mode,
//               const fs::path& pfsDir,
//               const fs::path& cacheDir) {
//     auto start = Clock::now();
//     for (auto& kv : metadataMap) {
//         const std::string& key = kv.first;
//         if (mode == "pfs") {
//             std::ifstream in(pfsDir / key, std::ios::binary);
//             if (!in) continue;
//             in.seekg(0, std::ios::end);
//             size_t n = in.tellg(); in.seekg(0, std::ios::beg);
//             std::vector<char> buf(n);
//             in.read(buf.data(), n);
//         } else {
//             std::ifstream in(cacheDir / kv.second.owner / key, std::ios::binary);
//         }
//     }
//     auto end = Clock::now();
//     return std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
// }

// int main(int argc, char* argv[]) {
//     // CLI defaults
//     int ttlSeconds = 10;
//     std::string failNode = "NodeC";
//     std::string mode = "nvme"; // or "pfs"
//     // parse args
//     for (int i = 1; i < argc; ++i) {
//         std::string a = argv[i];
//         if (a == "--ttl" && i+1<argc) ttlSeconds = std::stoi(argv[++i]);
//         else if (a == "--fail-node" && i+1<argc) failNode = argv[++i];
//         else if (a == "--mode" && i+1<argc) mode = argv[++i];
//         else if (a == "--help") {
//             std::cout<<"Usage: "<<argv[0]<<" [--ttl N] [--fail-node ID] [--mode nvme|pfs]\n";
//             return 0;
//         }
//     }
//     // std::cout<<"Mode="<<mode;
//     std::cout<<"TTL="<<ttlSeconds<<" FailNode="<<failNode<<"\n";

//     fs::path proj = fs::current_path();
//     fs::path pfsSrc = proj/"PFS_Source";
//     fs::path localCache = proj/"Local_Cache";
//     fs::create_directories(pfsSrc);
//     fs::create_directories(localCache);

//     // ensure test files
//     for (auto name : {"example.txt","data.bin","image.png"}) {
//         fs::path f = pfsSrc/name;
//         if (!fs::exists(f)) std::ofstream(f)<<"data";
//     }

//     // build ring
//     std::vector<std::string> nodes={"NodeA","NodeB","NodeC"};
//     ConsistentHashRing ring(nodes,100);

//     // 1) Epoch 1: cache warmup & recache/fallback
//     auto t0 = Clock::now();
//     for (auto& e: fs::directory_iterator(pfsSrc)) {
//         if (!e.is_regular_file()) continue;
//         cacheFile(e.path(), localCache, ring);
//     }
//     ring.removeNode(failNode);
//     fs::remove_all(localCache/ failNode);
//     if (mode=="nvme") {
//         recacheLostFiles(pfsSrc, localCache, ring);
//         ring.addNode(failNode);
//         rebalanceCache(localCache, ring);
//     } else {
//         simulatePFSFallback(pfsSrc);
//     }
//     auto t1 = Clock::now();
//     std::cout<<"Epoch 1 time: "<<std::chrono::duration_cast<std::chrono::milliseconds>(t1-t0).count()<<" ms\n";

//     // 2) Epochs 2-5
//     long nvmeSum=0, pfsSum=0;
//     for(int ep=2; ep<=5; ++ep) {
//         nvmeSum += runEpoch("nvme", pfsSrc, localCache);
//         pfsSum  += runEpoch("pfs",  pfsSrc, localCache);
//     }
//     std::cout<<"Epochs 2-5 NVMe total: "<<nvmeSum<<" ms\n";
//     std::cout<<"Epochs 2-5 PFS  total: "<<pfsSum<<" ms\n";
//     double speedup = 100.0*(pfsSum - nvmeSum)/pfsSum;
//     std::cout<<"Amortized speedup: "<<speedup<<"%\n";

//     // 3) TTL eviction
//     evictExpired(localCache, ttlSeconds);
//     return 0;
// }
