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

// Metadata per cached file
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

int main(int argc, char *argv[]) {
    bool batch = false;
    int runs = 5;
    int ttl = 0;
    std::vector<std::string> nodes = {"NodeA","NodeB","NodeC"};
    for (int i = 1; i < argc; ++i) {
        std::string a = argv[i];
        if (a == "--batch") batch = true;
        else if (a == "--runs" && i+1<argc) runs = std::stoi(argv[++i]);
        else if (a == "--ttl" && i+1<argc) ttl = std::stoi(argv[++i]);
    }
    fs::path proj = fs::current_path();
    fs::path pfsDir = proj / "PFS_Source";
    fs::path cacheDir = proj / "Local_Cache";
    fs::create_directories(pfsDir);
    fs::create_directories(cacheDir);
    // ensure PFS files
    for (auto &f : {"example.txt","data.bin","image.png"}) {
        fs::path p = pfsDir / f;
        if (!fs::exists(p)) std::ofstream(p) << "data";
    }
    std::ofstream csv("results.csv");
    csv << "FailNode,Run,Epoch1,Epochs2-5_NVMe,Epochs2-5_PFS,DistA,DistB,DistC\n";
    if (batch) {
        for (auto &failNode : nodes) {
            for (int r = 1; r <= runs; ++r) {
                // reset
                metadataMap.clear(); fs::remove_all(cacheDir); fs::create_directories(cacheDir);
                // init ring
                ConsistentHashRing ring(nodes,100);
                // epoch1 warmup
                auto t0 = Clock::now();
                for (auto &e : fs::directory_iterator(pfsDir)) if (e.is_regular_file()) cacheFile(e.path(), cacheDir, ring);
                ring.removeNode(failNode); fs::remove_all(cacheDir/ failNode);
                recacheLostFiles(pfsDir, cacheDir, ring);
                ring.addNode(failNode); rebalanceCache(cacheDir, ring);
                long e1 = std::chrono::duration_cast<std::chrono::milliseconds>(Clock::now() - t0).count();
                // tally distribution
                std::map<std::string,int> tally;
                for (auto &kv : metadataMap) tally[kv.second.owner]++;
                // epochs 2-5
                long nv=0, pf=0;
                for (int ep=2; ep<=5; ++ep) { nv+= runEpoch("nvme", pfsDir, cacheDir); pf+= runEpoch("pfs", pfsDir, cacheDir); }
                csv << failNode << "," << r << "," << e1 << "," << nv << "," << pf << "," << tally["NodeA"]
                    << "," << tally["NodeB"] << "," << tally["NodeC"] << "\n";
            }
        }
        csv.close();
        return 0;
    }
    // normal single-run mode omitted for brevity
    return 0;
}
