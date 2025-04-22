#include <filesystem>
#include <fstream>
#include <random>
#include <vector>
#include <iostream>

int main() {
    namespace fs = std::filesystem;
    fs::path dir = fs::current_path() / "PFS_Source";
    if (!fs::exists(dir)) {
        fs::create_directory(dir);
        std::cout << "Created directory: " << dir << "\n";
    }

    // RNG setup
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> sizeDist(1024, 10*1024);        // 1 KiB–10 KiB
    std::uniform_int_distribution<unsigned char> byteDist(0, 255);  // full byte range

    for (int i = 1; i <= 100; ++i) {
        int len = sizeDist(gen);
        std::vector<unsigned char> buf(len);
        for (auto &b : buf) b = byteDist(gen);

        fs::path file = dir / ("file_" + std::to_string(i) + ".bin");
        std::ofstream ofs(file, std::ios::binary);
        ofs.write(reinterpret_cast<char*>(buf.data()), buf.size());
        std::cout << "Wrote " << buf.size() << " bytes to " << file << "\n";
    }

    std::cout << "Done: 100 random files created in " << dir << "\n";
    return 0;
}
