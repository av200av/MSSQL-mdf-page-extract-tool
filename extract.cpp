#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <cstdint>
#include <cstring>
#include <iomanip>

struct CheckRule {
    size_t offset;
    std::vector<uint8_t> expected;
};

static std::vector<uint8_t> hexToBytes(const std::string& hex) {
    std::vector<uint8_t> bytes;
    for (size_t i = 0; i + 1 < hex.size(); i += 2) {
        std::string byteString = hex.substr(i, 2);
        uint8_t byte = static_cast<uint8_t>(std::stoul(byteString, nullptr, 16));
        bytes.push_back(byte);
    }
    return bytes;
}

static void printUsage(const char* prog) {
    std::cerr << "SQL MDF Valid Block Extractor" << std::endl;
    std::cerr << "Usage: " << prog << " <input> <output> [block_size] [offset=hex ...]" << std::endl;
    std::cerr << std::endl;
    std::cerr << "Default: block_size=8192, checks 6=0001, 24=B0020000" << std::endl;
    std::cerr << "Example: " << prog << " test.mdf d.dd" << std::endl;
    std::cerr << "         " << prog << " test.mdf d.dd 8192 6=0001 24=B0020000" << std::endl;
    std::cerr << "         " << prog << " test.mdf d.dd 8192 0=010F" << std::endl;
}

int main(int argc, char* argv[]) {
    if (argc < 3) {
        printUsage(argv[0]);
        return 1;
    }

    std::string inputFile  = argv[1];
    std::string outputFile = argv[2];
    size_t blockSize = 8192;

    std::vector<CheckRule> rules;
    // Default: offset 6 = 00 01, offset 24 = B0 02 00 00
    rules.push_back({6,  {0x00, 0x01}});
    rules.push_back({24, {0xB0, 0x02, 0x00, 0x00}});

    int argIdx = 3;

    // 3rd arg: if pure number, treat as block size
    if (argc > argIdx) {
        std::string a3 = argv[argIdx];
        bool isNum = !a3.empty();
        for (char c : a3) {
            if (!isdigit(static_cast<unsigned char>(c))) { isNum = false; break; }
        }
        if (isNum) {
            blockSize = std::stoul(a3);
            argIdx++;
        }
    }

    // Parse custom rules offset=hex
    bool hasCustom = false;
    for (; argIdx < argc; argIdx++) {
        std::string arg = argv[argIdx];
        size_t eqPos = arg.find('=');
        if (eqPos == std::string::npos) continue;

        if (!hasCustom) {
            rules.clear();
            hasCustom = true;
        }

        size_t offset = std::stoul(arg.substr(0, eqPos));
        std::string hex = arg.substr(eqPos + 1);
        // strip optional 0x prefix
        if (hex.size() >= 2 && (hex[0] == '0') && (hex[1] == 'x' || hex[1] == 'X')) {
            hex = hex.substr(2);
        }
        if (hex.size() % 2 != 0) {
            std::cerr << "Warning: hex length not even '" << hex << "', ignored" << std::endl;
            continue;
        }
        rules.push_back({offset, hexToBytes(hex)});
    }

    if (rules.empty()) {
        std::cerr << "Error: no check rules" << std::endl;
        return 1;
    }

    std::ifstream inFile(inputFile, std::ios::binary);
    if (!inFile) {
        std::cerr << "Error: cannot open input file: " << inputFile << std::endl;
        return 1;
    }

    std::ofstream outFile(outputFile, std::ios::binary | std::ios::trunc);
    if (!outFile) {
        std::cerr << "Error: cannot open output file: " << outputFile << std::endl;
        return 1;
    }

    // Print config
    std::cout << "Input file:  " << inputFile << std::endl;
    std::cout << "Output file: " << outputFile << std::endl;
    std::cout << "Block size:  " << blockSize << " (0x" << std::hex << blockSize << std::dec << ")" << std::endl;
    std::cout << "Check rules:" << std::endl;
    for (const auto& r : rules) {
        std::cout << "  offset " << r.offset << " =";
        for (uint8_t b : r.expected) {
            std::cout << " " << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(b);
        }
        std::cout << std::dec << std::endl;
    }
    std::cout << "----------------------------------------" << std::endl;

    std::vector<uint8_t> buffer(blockSize);
    size_t blockNum = 0;
    size_t validCount = 0;

    while (true) {
        inFile.read(reinterpret_cast<char*>(buffer.data()), blockSize);
        std::streamsize bytesRead = inFile.gcount();
        if (bytesRead <= 0) break;

        bool valid = true;
        for (const auto& rule : rules) {
            if (rule.offset + rule.expected.size() > static_cast<size_t>(bytesRead)) {
                valid = false;
                break;
            }
            if (std::memcmp(buffer.data() + rule.offset, rule.expected.data(), rule.expected.size()) != 0) {
                valid = false;
                break;
            }
        }

        if (valid) {
            outFile.write(reinterpret_cast<char*>(buffer.data()), bytesRead);
            validCount++;
            std::cout << "[VALID] block #" << blockNum
                      << " file_offset 0x" << std::hex << (blockNum * blockSize)
                      << std::dec << " (" << blockNum * blockSize << ")" << std::endl;
        }

        blockNum++;

        if (static_cast<size_t>(bytesRead) < blockSize) break;
    }

    std::cout << "----------------------------------------" << std::endl;
    std::cout << "Done: scanned " << blockNum << " blocks, extracted " << validCount << " valid blocks" << std::endl;
    std::cout << "Output: " << outputFile << std::endl;

    return 0;
}
