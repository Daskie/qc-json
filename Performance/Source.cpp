#include <chrono>
#include <iostream>
#include <sstream>
#include <variant>
#include <unordered_map>
#include <vector>

int main() {
    std::cout << std::numeric_limits<int64_t>::min() << " " << -std::numeric_limits<int64_t>::min() << std::endl;

    constexpr int k_sets(100), k_reps(10000);
    uint64_t *  vals = new uint64_t[k_reps];
    for (int i(0); i < k_reps; ++i) {
        vals[i] = std::rand() | (uint64_t(std::rand()) << 32);
    }

    double t1(0.0), t2(0.0);
    std::stringstream ss;
    for (int i = 0; i < k_sets; ++i) {
        {
            auto then(std::chrono::high_resolution_clock::now());
            for (int j = 0; j < k_reps; ++j) {
                //f1(ss, vals[j]);
            }
            t1 += (std::chrono::high_resolution_clock::now() - then).count() * 1.0e-9;
        }
        {
            auto then(std::chrono::high_resolution_clock::now());
            for (int j = 0; j < k_reps; ++j) {
                //f2(ss, vals[j]);
            }
            t2 += (std::chrono::high_resolution_clock::now() - then).count() * 1.0e-9;
        }
        ss.str("");
        ss.clear();
    }
    std::cout << t1 << " " << t2 << std::endl;
    std::cin.get();

    return 0;
}