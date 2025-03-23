#include <iostream>                                 
#include <fstream>
#include <filesystem>
#include <chrono>
#include <unordered_map>
#include <string>

namespace fs = std::filesystem;

struct FileMetadata{
    std::chrono::system_clock::time_point timestamp;            // time when file was cached
    std::string fileKey;                                        // file identifier - file name
    // Additional fields for metadata will be added here later
};

// Global Map that contains the metadata for each cached file using filekey as identifier
std::unordered_map<std::string, FileMetadata> metadataMap;

bool cacheFile(const fs::path& sourceFile, const fs::path& cacheDir){
    if (!fs::exists(sourceFile)){
        std::cerr <<"Source file: " << sourceFile << " does not exist" << std::endl;
        return false;
    }

    fs::path cacheFile = cacheDir / sourceFile.filename();      // extracts filename from sourcefile and creates new path with cacheDir

    std::ifstream inFile(sourceFile, std::ios::binary);         // opens sourcefile in binary mode to read file exactly as is
    if (!inFile) {
        std::cerr << "Error opening source file: " << sourceFile << std::endl;
        return false;
    }

    std::ofstream outFile(cacheFile, std::ios::binary);         // opens cachefile in binary mode to write read file exactly as is
    if (!outFile){
        std::cerr << "Error opening cache file: " << cacheFile << std::endl;
        return false;
    }

    outFile << inFile.rdbuf();                                      // writes sourcefile into cachefile

    FileMetadata meta;
    meta.timestamp = std::chrono::system_clock::now();
    meta.fileKey = sourceFile.filename().string();
    metadataMap[sourceFile.filename().string()] = meta;

    std::cout << "Cached file " << sourceFile << " to " << cacheFile << std::endl;
    return true;
}

int main(){
    fs::path projectDir = fs::current_path();
    fs::path pfsSourceDir = projectDir / "PFS_Source";
    fs::path localCacheDir = projectDir / "Local_Cache";

    if (!fs::exists(pfsSourceDir)){
        fs::create_directory(pfsSourceDir);
        std::cout << "Created directory: " << pfsSourceDir << std::endl;
    }

    if (!fs::exists(localCacheDir)){
        fs::create_directory(localCacheDir);
        std::cout << "Created directory: " << localCacheDir << std::endl;
    }

    fs::path testFile = pfsSourceDir / "example.txt";
    if (!fs::exists(testFile)){
        std::ofstream(testFile) << "This is a test file from the simulated PFS" << std::endl;
        std:: cout << "Created test file: " << testFile << std::endl;
    }

    if (!cacheFile(testFile, localCacheDir)){
        std::cerr << "Failed to cache file." << std::endl;
        return 1;
    }
    
    return 0;
}
