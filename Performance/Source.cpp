#include <chrono>
#include <iostream>
#include <sstream>

static std::ostream & do0(std::ostream & os, uint64_t v) {
    return os << v;
}

static std::ostream & do3(std::ostream & os, uint64_t v) {
    static constexpr char k_hexChars[16]{'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F'};

    char buffer[16];

    for (int i(15); i >= 0; --i) {
        buffer[i] = k_hexChars[v & 0xF];
        v >>= 4;
    };

    return os << std::string_view(buffer, 16);
}

static std::ostream & do1(std::ostream & os, uint64_t v) {
    static constexpr char k_hexChars[16]{'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F'};

    char buffer[16]{
        k_hexChars[(v >> 60) & 0xF],
        k_hexChars[(v >> 56) & 0xF],
        k_hexChars[(v >> 52) & 0xF],
        k_hexChars[(v >> 48) & 0xF],
        k_hexChars[(v >> 44) & 0xF],
        k_hexChars[(v >> 40) & 0xF],
        k_hexChars[(v >> 36) & 0xF],
        k_hexChars[(v >> 32) & 0xF],
        k_hexChars[(v >> 28) & 0xF],
        k_hexChars[(v >> 24) & 0xF],
        k_hexChars[(v >> 20) & 0xF],
        k_hexChars[(v >> 16) & 0xF],
        k_hexChars[(v >> 12) & 0xF],
        k_hexChars[(v >>  8) & 0xF],
        k_hexChars[(v >>  4) & 0xF],
        k_hexChars[(v >>  0) & 0xF],
    };

    return os << std::string_view(buffer, 16);
}

static std::ostream & do2(std::ostream & os, uint64_t v) {
    static constexpr char k_hexChars[16]{'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F'};

    char buffer[17]{
        k_hexChars[(v >> 60) & 0xF],
        k_hexChars[(v >> 56) & 0xF],
        k_hexChars[(v >> 52) & 0xF],
        k_hexChars[(v >> 48) & 0xF],
        k_hexChars[(v >> 44) & 0xF],
        k_hexChars[(v >> 40) & 0xF],
        k_hexChars[(v >> 36) & 0xF],
        k_hexChars[(v >> 32) & 0xF],
        k_hexChars[(v >> 28) & 0xF],
        k_hexChars[(v >> 24) & 0xF],
        k_hexChars[(v >> 20) & 0xF],
        k_hexChars[(v >> 16) & 0xF],
        k_hexChars[(v >> 12) & 0xF],
        k_hexChars[(v >>  8) & 0xF],
        k_hexChars[(v >>  4) & 0xF],
        k_hexChars[(v >>  0) & 0xF],
        '\0'
    };

    return os << buffer;
}

int main() {
    constexpr int k_sets(100), k_reps(100000);
    uint64_t vals[k_reps];
    for (int i(0); i < k_reps; ++i) {
        vals[i] = (uint64_t(std::rand()) << 32) | std::rand();
    }

    double t1(0.0), t2(0.0);
    for (int i = 0; i < k_sets; ++i) {
        {
            auto then(std::chrono::high_resolution_clock::now());
            std::ostringstream os;
            for (int j = 0; j < k_reps; ++j) {
                do1(os, vals[j]);
            }
            volatile std::string s(os.str());
            t1 += (std::chrono::high_resolution_clock::now() - then).count() * 1.0e-9;
        }
        {
            auto then(std::chrono::high_resolution_clock::now());
            std::ostringstream os;
            for (int j = 0; j < k_reps; ++j) {
                do2(os, vals[j]);
            }
            volatile std::string s(os.str());
            t2 += (std::chrono::high_resolution_clock::now() - then).count() * 1.0e-9;
        }
    }
    std::cout << t1 << " " << t2 << std::endl;
    std::cin.get();

    return 0;
}