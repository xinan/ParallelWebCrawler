#include <iostream>
#include <fstream>
#include <algorithm>
#include "WebCrawler.h"

void printUsage(const char* executable) {
    fprintf(stderr, "Usage: ./%s <target amount> <seed file>\n", executable);
    exit(EXIT_FAILURE);
}

int main(int argc, const char** argv) {
    if (argc != 3) {
        printUsage(argv[0]);
    }

    const int target_amount = atoi(argv[1]);
    if (target_amount == 0) {
        printUsage(argv[0]);
    }

    std::ifstream seed_file(argv[2]);
    if (!seed_file.is_open()) {
        fprintf(stderr, "Seed file not found!\n");
        printUsage(argv[0]);
    }

    std::vector<std::string> seeds;

    std::string line;
    while (std::getline(seed_file, line)) {
        line.erase(std::remove_if(line.begin(), line.end(), [](char x) { return std::isspace(x); }));
        if (!line.empty()) {
            seeds.push_back(line);
        }
    }

    const auto start = std::chrono::steady_clock::now();
    WebCrawler crawler(target_amount, seeds);
    crawler.start();
    const auto end = std::chrono::steady_clock::now();

    fprintf(stderr, "\nTime taken: %llims\n", std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count());

    return 0;
}

