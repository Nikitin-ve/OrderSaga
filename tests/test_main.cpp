#include "test_framework.hpp"
#include <cstring>

int main(int argc, char* argv[]) {
    std::string filter;
    if (argc > 1) {
        filter = argv[1];
    }
    return RunAllTests(filter);
}
