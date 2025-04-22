#include <iostream>
#include <fstream>
#include <filesystem>
#include <chrono>
#include <thread>
#include <unordered_map>
#include <string>
#include <vector>
#include "ConsistentHashRing.hpp"

namespace fs = std::filesystem;
using Clock = std::chrono::high_resolution_clock;
typedef Clock::time_point TimePoint;

struct FileMetadata {
    TimePoint timestamp;
    std::string fileKey;
    std::string owner;
};
static std::unordered_map<std::string, FileMetadata> metadataMap;

bool cacheFile(const fs::path &src, const fs::path &cacheDir, const ConsistentHashRing &ring) {
    if (!fs::exists(src)) return false;
    std::string owner = ring.getNodeForKey(src.string());
    fs::path nodeDir = cacheDir / owner;
    fs::create_directories(nodeDir);
    std::string fname = src.filename().string();
    fs::path dest = nodeDir / fname;
    std::ifstream in(src, std::ios::binary);
    std::ofstream out(dest, std::ios::binary);
    if (!in || !out) return false;
    out << in.rdbuf();
    metadataMap[fname] = {Clock::now(), fname, owner};
    return true;
}

void recacheLostFiles(const fs::path &pfsDir, const fs::path &cacheDir, ConsistentHashRing &ring) {
    for (auto &kv : metadataMap) {
        const auto &m = kv.second;
        fs::path p = cacheDir / m.owner / m.fileKey;
        if (!fs::exists(p)) cacheFile(pfsDir / m.fileKey, cacheDir, ring);
    }
}

void simulatePFSFallback(const fs::path &pfsDir) {
    std::vector<char> buf;
    for (auto &kv : metadataMap) {
        fs::path p = pfsDir / kv.first;
        std::ifstream in(p, std::ios::binary);
        if (!in) continue;
        in.seekg(0, std::ios::end);
        size_t n = in.tellg(); in.seekg(0, std::ios::beg);
        buf.resize(n);
        in.read(buf.data(), n);
    }
}

int rebalanceCache(const fs::path &cacheDir, ConsistentHashRing &ring) {
    int moved = 0;
    for (auto &kv : metadataMap) {
        auto &m = kv.second;
        std::string newOwner = ring.getNodeForKey(m.fileKey);
        if (newOwner != m.owner) {
            fs::path oldp = cacheDir / m.owner / m.fileKey;
            fs::path newd = cacheDir / newOwner;
            fs::create_directories(newd);
            fs::path newp = newd / m.fileKey;
            if (fs::exists(oldp)) {
                fs::rename(oldp, newp);
                m.owner = newOwner;
                ++moved;
            }
        }
    }
    return moved;
}

void evictExpired(const fs::path &cacheDir, int ttl) {
    auto now = Clock::now();
    for (auto it = metadataMap.begin(); it != metadataMap.end();) {
        int age = std::chrono::duration_cast<std::chrono::seconds>(now - it->second.timestamp).count();
        if (age >= ttl) {
            fs::remove(cacheDir / it->second.owner / it->second.fileKey);
            it = metadataMap.erase(it);
        } else ++it;
    }
}

long runEpoch(const std::string &mode, const fs::path &pfsDir, const fs::path &cacheDir) {
    auto start = Clock::now();
    for (auto &kv : metadataMap) {
        const std::string &k = kv.first;
        if (mode == "pfs") {
            std::ifstream in(pfsDir / k, std::ios::binary);
            if (!in) continue; in.seekg(0, std::ios::end);
            size_t n = in.tellg(); in.seekg(0, std::ios::beg);
            std::vector<char> buf(n); in.read(buf.data(), n);
        } else {
            std::ifstream in(cacheDir / kv.second.owner / k, std::ios::binary);
        }
    }
    return std::chrono::duration_cast<std::chrono::milliseconds>(Clock::now() - start).count();
}

void runSmallExperiment() {
    std::vector<std::string> nodes = {"NodeA", "NodeB", "NodeC"};
    ConsistentHashRing ring(nodes, 100);
    fs::path proj = fs::current_path();
    fs::path pfs = proj / "PFS_Source";
    fs::path cache = proj / "Local_Cache";

    metadataMap.clear();
    fs::remove_all(cache); 
    fs::create_directories(cache);
    fs::create_directories(pfs);

    // Generate files only if PFS_Source is empty
    if (fs::is_empty(pfs)) {
        std::cout << "Generating 100 sample files in PFS_Source...\n";
        for (int i = 1; i <= 100; ++i) {
            std::ofstream(pfs / ("file_" + std::to_string(i) + ".txt")) << "content " << i;
        }
    } else {
        std::cout << "PFS_Source already contains files.\n";
    }
    
    auto t0 = Clock::now();
    for (const auto &f : fs::directory_iterator(pfs))
        if (f.is_regular_file()) cacheFile(f.path(), cache, ring);

    std::string failNode = "NodeC";
    ring.removeNode(failNode); fs::remove_all(cache / failNode);
    recacheLostFiles(pfs, cache, ring);
    ring.addNode(failNode);
    int moved = rebalanceCache(cache, ring);
    auto t1 = Clock::now();
    long e1 = std::chrono::duration_cast<std::chrono::milliseconds>(t1 - t0).count();

    long nvme = 0, pfsread = 0;
    for (int i = 0; i < 4; ++i) nvme += runEpoch("nvme", pfs, cache);
    for (int i = 0; i < 4; ++i) pfsread += runEpoch("pfs", pfs, cache);

    std::map<std::string, int> tally;
    for (const auto &kv : metadataMap) tally[kv.second.owner]++;

    std::cout << "==== Single Experiment ====\n";
    std::cout << "Failed Node: " << failNode << "\n";
    std::cout << "Epoch 1 (recache + rebalance): " << e1 << " ms\n";
    std::cout << "Rebalanced moved: " << moved << " files\n";
    std::cout << "Epochs 2–5 NVMe total: " << nvme << " ms\n";
    std::cout << "Epochs 2–5 PFS  total: " << pfsread << " ms\n";
    std::cout << "Amortized speedup: " << ((1 - ((double)nvme / pfsread)) * 100) << "%\n";
    std::cout << "Key Distribution: A=" << tally["NodeA"] << ", B=" << tally["NodeB"] << ", C=" << tally["NodeC"] << "\n";
    std::cout << "===========================\n";
}

void runExperimentBatch(int trials, const std::string &failNode, const fs::path &csvPath) {
    bool fileExists = fs::exists(csvPath);
    std::ofstream log(csvPath, std::ios::app);

    if (!fileExists) {
        log << "Trial,FailNode,Epoch1_ms,NVMe_ms,PFS_ms,Speedup_pct,Moved,A,B,C\n";
    }

    for (int t = 1; t <= trials; ++t) {
        std::vector<std::string> nodes = {"NodeA", "NodeB", "NodeC"};
        ConsistentHashRing ring(nodes, 100);
        fs::path proj = fs::current_path();
        fs::path pfs = proj / "PFS_Source";
        fs::path cache = proj / "Local_Cache";

        metadataMap.clear();
        fs::remove_all(cache); fs::create_directories(cache);

        if (fs::is_empty(pfs)) {
            for (int i = 1; i <= 204; ++i)
                std::ofstream(pfs / ("file_" + std::to_string(i) + ".txt")) << "content " << i;
        }

        auto t0 = Clock::now();
        for (const auto &f : fs::directory_iterator(pfs))
            if (f.is_regular_file()) cacheFile(f.path(), cache, ring);

        std::map<std::string, int> tally;
        for (const auto &kv : metadataMap) tally[kv.second.owner]++;

        ring.removeNode(failNode); fs::remove_all(cache / failNode);
        recacheLostFiles(pfs, cache, ring);
        ring.addNode(failNode);
        int moved = rebalanceCache(cache, ring);
        auto t1 = Clock::now();
        long e1 = std::chrono::duration_cast<std::chrono::milliseconds>(t1 - t0).count();

        long nvme = 0, pfsread = 0;
        for (int i = 0; i < 4; ++i) nvme += runEpoch("nvme", pfs, cache);
        for (int i = 0; i < 4; ++i) pfsread += runEpoch("pfs", pfs, cache);

        double speedup = (1 - ((double)nvme / pfsread)) * 100;

        log << t << "," << failNode << "," << e1 << "," << nvme << "," << pfsread << ","
            << speedup << "," << moved << ","
            << tally["NodeA"] << "," << tally["NodeB"] << "," << tally["NodeC"] << "\n";
    }

    std::cout << "Batch experiment complete. Results saved to " << csvPath << "\n";
}

int main() {
    // runSmallExperiment();
    runExperimentBatch(0, "NodeC", "result.csv");
    return 0;
}
