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

// // Metadata for each cached file
// struct FileMetadata {
//     std::chrono::system_clock::time_point timestamp;  // time when file was cached
//     std::string fileKey;                              // filename identifier
//     std::string owner;                                // owning node identifier
// };

// // Global in-memory metadata map: filename -> metadata
// static std::unordered_map<std::string, FileMetadata> metadataMap;

// // Caches a file into a node-specific subdirectory based on consistent hashing
// bool cacheFile(const fs::path& sourceFile,
//                const fs::path& cacheDir,
//                const ConsistentHashRing& ring) {
//     // Verify source exists
//     if (!fs::exists(sourceFile)) {
//         std::cerr << "Source file does not exist: " << sourceFile << std::endl;
//         return false;
//     }

//     // Determine owning node via hash ring
//     std::string owner = ring.getNodeForKey(sourceFile.string());

//     // Create node-specific subdirectory
//     fs::path nodeDir = cacheDir / owner;
//     if (!fs::exists(nodeDir)) {
//         fs::create_directories(nodeDir);
//     }

//     // Destination path under the node subdirectory
//     std::string fileName = sourceFile.filename().string();
//     fs::path destFile = nodeDir / fileName;

//     // Copy file in binary mode
//     std::ifstream inFile(sourceFile, std::ios::binary);
//     std::ofstream outFile(destFile, std::ios::binary);
//     if (!inFile || !outFile) {
//         std::cerr << "Error opening files: " << sourceFile << " or " << destFile << std::endl;
//         return false;
//     }
//     outFile << inFile.rdbuf();

//         // Record metadata (timestamp, fileKey, owner)
//     FileMetadata meta;
//     meta.timestamp = std::chrono::system_clock::now();
//     meta.fileKey   = fileName;
//     meta.owner     = owner;                     // set owner before insertion
//     metadataMap[fileName] = meta;

//     std::cout << "Cached file " << sourceFile << " -> " << destFile << std::endl;
//     return true;
// }


// void recacheLostFiles(const fs::path& pfsSourceDir, const fs::path& cacheDir, ConsistentHashRing& ring){
//     for (auto& kv : metadataMap){
//         const std::string& fileName = kv.first;
//         const FileMetadata& meta = kv.second;
//         fs::path expectedPath = cacheDir / meta.owner / fileName;

//         if (!fs::exists(expectedPath)){
//             fs::path src = pfsSourceDir / fileName;
//             std::cout << "Recaching lost file " << fileName << "\n";
//             cacheFile(src, cacheDir, ring);
//         }
//     }
// }

// // In “pfs” mode, simulate fallback by reading each missing file from PFS_Source
// void simulatePFSFallback(const fs::path& pfsSourceDir) {
//     std::vector<char> buffer;
//     for (auto& kv : metadataMap) {
//         const std::string& fileKey = kv.first;
//         fs::path src = pfsSourceDir / fileKey;
//         std::ifstream in(src, std::ios::binary);
//         if (!in) continue;
//         // read entire file into buffer (just to simulate I/O)
//         in.seekg(0, std::ios::end);
//         size_t size = in.tellg();
//         in.seekg(0, std::ios::beg);
//         buffer.resize(size);
//         in.read(buffer.data(), size);
//     }
// }


// int rebalanceCache(const fs::path& cacheDir, ConsistentHashRing& ring){
//     int movedCount = 0;
//     for (auto& kv : metadataMap){
//         auto& meta = kv.second;
//         std::string newOwner = ring.getNodeForKey(meta.fileKey);
//         if (newOwner != meta.owner){
//             fs::path oldPath = cacheDir / meta.owner / meta.fileKey;
//             fs::path newDir = cacheDir / newOwner;
//             fs::create_directories(newDir);
//             fs::path newPath = newDir / meta.fileKey;
//             if (fs::exists(oldPath)) {
//                 fs::rename(oldPath, newPath);
//                 std::cout << "Moved " << meta.fileKey << " from " << meta.owner << " → " << newOwner << "\n";
//                 meta.owner = newOwner;
//                 ++movedCount;
//             }
//         }
//     }
//     std::cout << "Rebalance: moved " << movedCount << " files.\n";
//     return movedCount;
// }

// // Evicts cached files older than ttlSeconds
// void evictExpired(const fs::path& cacheDir,
//                   int ttlSeconds) {
//     auto now = std::chrono::system_clock::now();
//     for (auto it = metadataMap.begin(); it != metadataMap.end(); ) {
//         const auto& meta = it->second;
//         int age = std::chrono::duration_cast<std::chrono::seconds>(now - meta.timestamp).count();
//         if (age > ttlSeconds) {
//             // Use stored owner and fileKey for correct path
//             fs::path fullPath = cacheDir / meta.owner / meta.fileKey;
//             if (fs::exists(fullPath)) {
//                 fs::remove(fullPath);
//                 std::cout << "Evicted expired file: " << fullPath << std::endl;
//             }
//             it = metadataMap.erase(it);
//         } else {
//             ++it;
//         }
//     }
// }

// int main(int argc, char* argv[]) {
//     // Default parameters
//     int ttlSeconds = 10;
//     std::string failNode = "NodeB";
//     // Default mode: nvme; alternative “pfs” will do pure PFS reads
//     std::string mode = "nvme";

//     // Simple command-line parsing
//     for (int i = 1; i < argc; ++i) {
//         std::string arg = argv[i];
//         if (arg == "--ttl" && i + 1 < argc) {
//             ttlSeconds = std::stoi(argv[++i]);
//         } else if (arg == "--fail-node" && i + 1 < argc) {
//             failNode = argv[++i];
//         } else if (arg == "--help") {
//             std::cout << "Usage: " << argv[0] << " [--ttl <seconds>] [--fail-node <nodeID>]" << std::endl;
//             return 0;
//         }
//         else if (arg == "--mode" && i+1 < argc) {
//             mode = argv[++i];
//         }
//     }
//     std::cout << "Mode: " << mode << std::endl;
//     std::cout << "Using TTL=" << ttlSeconds << "s, failing node=" << failNode << std::endl;
//     fs::path projectDir    = fs::current_path();
//     fs::path pfsSourceDir  = projectDir / "PFS_Source";
//     fs::path localCacheDir = projectDir / "Local_Cache";

//     // Ensure directories exist
//     if (!fs::exists(pfsSourceDir))  fs::create_directory(pfsSourceDir);
//     if (!fs::exists(localCacheDir)) fs::create_directory(localCacheDir);

//     // Sample test files
//     std::vector<fs::path> testFiles = {
//         pfsSourceDir / "example.txt",
//         pfsSourceDir / "data.bin",
//         pfsSourceDir / "image.png"
//     };
//     for (const auto& tf : testFiles) {
//         if (!fs::exists(tf)) {
//             std::ofstream(tf) << "Dummy content for " << tf.filename().string() << std::endl;
//             std::cout << "Created test file: " << tf << std::endl;
//         }
//     }

//     // Build consistent hash ring
//     std::vector<std::string> nodes = {"NodeA", "NodeB", "NodeC"};
//     ConsistentHashRing ring(nodes, 100);

//     // Cache all files in PFS_Source
//     for (auto& entry : fs::directory_iterator(pfsSourceDir)) {
//         if (!entry.is_regular_file()) continue;
//         cacheFile(entry.path(), localCacheDir, ring);
//     }

//     // TTL eviction test: wait then evict
//     // int ttlSeconds = 10;
//     std::cout << "Waiting " << ttlSeconds << " seconds before eviction..." << std::endl;
//     std::this_thread::sleep_for(std::chrono::seconds(ttlSeconds));

//     std::cout << "Running eviction (TTL=" << ttlSeconds << "s)..." << std::endl;
//     evictExpired(localCacheDir, ttlSeconds);

//     return 0;
// }
