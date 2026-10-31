#include <iostream>
#include <string_view>
#include <vector>

import craft.cli;

int main(int argc, char* argv[]) {
    std::vector<std::string_view> args;
    for (int i = 1; i < argc; ++i) {
        args.emplace_back(argv[i]);
    }
    return craft::cli::Run(args, std::cout, std::cerr);
}
