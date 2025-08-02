// #include <iostream>                                 
// #include <fstream>
// #include <filesystem>
// #include <chrono>
// #include <unordered_map>
// #include <string>
// #include <thread>
// #include "ConsistentHashRing.hpp"

// namespace fs = std::filesystem;

// struct FileMetadata{
//     std::chrono::system_clock::time_point timestamp;            // time when file was cached
//     std::string fileKey;                                        // file identifier - file name
//     std::string owner;
//     // Additional fields for metadata will be added here later
// };

// // Global Map that contains the metadata for each cached file using filekey as identifier
// std::unordered_map<std::string, FileMetadata> metadataMap;

// bool cacheFile(const fs::path& sourceFile, const fs::path& cacheDir, const ConsistentHashRing& ring){

//     // Verify source exists
//     if (!fs::exists(sourceFile)){
//         std::cerr <<"Source file: " << sourceFile << " does not exist" << std::endl;
//         return false;
//     }

//     // Find node via hash ring
//     std::string owner = ring.getNodeForKey(sourceFile.string());

//     fs::path nodeDir = cacheDir / owner;
//     if (!fs::exists(nodeDir)){
//         fs::create_directories(nodeDir);
//     }
//     std::string fileName = sourceFile.filename().string();
//     fs::path destFile = nodeDir / fileName;

//     // fs::path cacheFile = nodeDir / sourceFile.filename();      // extracts filename from sourcefile and creates new path with cacheDir

//     std::ifstream inFile(sourceFile, std::ios::binary);         // opens sourcefile in binary mode to read file exactly as is
//     std::ofstream outFile(destFile, std::ios::binary);         // opens cachefile in binary mode to write read file exactly as is
//     if (!inFile || !outFile){
//         std::cerr << "Error opening files: " << sourceFile << " or " << destFile << std::endl;
//         return false;
//     }
//     outFile << inFile.rdbuf();                                      // writes sourcefile into cachefile

//     FileMetadata meta;
//     meta.timestamp = std::chrono::system_clock::now();
//     meta.fileKey = fileName;
//     meta.owner = owner;
//     metadataMap[sourceFile.filename().string()] = meta;


//     // std::cout << "Cached file " << sourceFile << " to " << destFile << std::endl;
//     return true;
// }
// void evictExpired(const fs::path& cacheDir, int ttlSeconds){
//     auto now = std::chrono::system_clock::now();
//     for (auto it = metadataMap.begin(); it != metadataMap.end();){
//         const std::string& filename = it->first;
//         const FileMetadata& meta = it->second;
//         int age = std::chrono::duration_cast<std::chrono::seconds>(now - meta.timestamp).count();

//         if (age >= ttlSeconds){
//             fs::path fullPath = cacheDir / meta.owner / meta.fileKey;

//             if (fs::exists(fullPath)){
//                 fs::remove(fullPath);
//                 std::cout << "Evicted expired file: " << fullPath << std::endl;
//             }
//             it = metadataMap.erase(it);
//         } else{
//             ++it;
//         }
//     }
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
// int main(){
//     std::cout << "Yes, I am working!" << std::endl;

//     fs::path projectDir = fs::current_path();
//     fs::path pfsSourceDir = projectDir / "PFS_Source";
//     fs::path localCacheDir = projectDir / "Local_Cache";

//     if (!fs::exists(pfsSourceDir)) fs::create_directory(pfsSourceDir);
//     if (!fs::exists(localCacheDir)) fs::create_directory(localCacheDir);

//         // Sample test files
//     std::vector<fs::path> testFiles = {
//             pfsSourceDir / "example.txt",
//             pfsSourceDir / "data.bin",
//             pfsSourceDir / "image.png"
//     };
//     for (auto& tf : testFiles) {
//         if (!fs::exists(tf)) {
//             std::ofstream(tf) << "Dummy content for " << tf.filename().string() << std::endl;
//         }
//     }

//     // fs::path testFile = pfsSourceDir / "example.txt";
//     // if (!fs::exists(testFile)){
//     //     std::ofstream(testFile) << "This is a test file from the simulated PFS" << std::endl;
//     //     std:: cout << "Created test file: " << testFile << std::endl;
//     // }

//     std::vector<std::string> nodes = {"NodeA", "NodeB", "NodeC"};
//     ConsistentHashRing ring(nodes, 100);
    
//     // Cache every file in PFS_Source
//     for (auto& entry : fs::directory_iterator(pfsSourceDir)){
//         if(!entry.is_regular_file()) continue;

//         const fs::path sourcePath = entry.path();
//         if(!cacheFile(sourcePath, localCacheDir, ring)){
//             std::cerr << "Failed to cache: " << sourcePath << std::endl;
//         }
//     }

//     // --- simulate a node failure ---
//     std::string failedNode = "NodeB";  // pick any from {"NodeA","NodeB","NodeC"}
//     std::cout << ">>> Simulating failure of " << failedNode << "\n";
//     ring.removeNode(failedNode);

//     // Remove its on‑disk cache directory
//     fs::path failedDir = localCacheDir / failedNode;
//     if (fs::exists(failedDir)) {
//         fs::remove_all(failedDir);
//         std::cout << "Removed directory: " << failedDir << "\n";
//     }

//     // Now recache anything that got lost
//     recacheLostFiles(pfsSourceDir, localCacheDir, ring);
//     // … after you recacheLostFiles for the failed node …

//     // 4) Simulate the node coming back online:
//     std::cout << ">>> Bringing " << failedNode << " back online\n";
//     ring.addNode(failedNode);

//     // 5) Rebalance existing cache onto the returned node
//     int moves = rebalanceCache(localCacheDir, ring);
//     std::cout << "After node addition, " << moves << " files were relocated.\n";


//     // Wait for TTL to expire
//     // int ttlSeconds = 3;
//     // std::cout << "Waiting " << ttlSeconds << " seconds before eviction..." << std::endl;
//     // std::this_thread::sleep_for(std::chrono::seconds(ttlSeconds));

//     // // Evict expired files (TTL=10 seconds)
//     // std::cout << "Running eviction (TTL=" << ttlSeconds << "s)..." << std::endl;
//     // evictExpired(localCacheDir, ttlSeconds);


//     // if (!cacheFile(testFile, localCacheDir, ring)){
//     //     std::cerr << "Failed to cache file." << std::endl;
//     //     return 1;
//     // }



//     // std::vector<std::string> fileKeys = {
//     //     "PFS_Source/example.txt",
//     //     "PFS_Source/image.png",
//     //     "PFS_Source/data.bin"
//     // };

//     // for (const auto &key : fileKeys){
//     //     try{
//     //         std::string owner = ring.getNodeForKey(key);
//     //         std::cout << "File key \"" << key << "\" maps to node: " << owner << "\n";
//     //     }catch (const std::exception &e) {
//     //         std::cerr << "Error mapping key \"" << key << "\": " << e.what() << "\n";
//     //     }
//     // }

//     // Distribution tally
//     // std::map<std::string, int> tally;
//     // int total_keys = 1000;
//     // for (int i = 0; i < total_keys; ++i) {
//     //     std::string key = "file_" + std::to_string(i);
//     //     std::string node = ring.getNodeForKey(key);
//     //     tally[node]++;
//     // }

//     // std::cout << "Distribution of " << total_keys << " keys across nodes:\n";
//     // for (const auto& kv : tally) {
//     //     std::cout << kv.first << "," << kv.second << std::endl;
//     // }

    
//     return 0;
// }
