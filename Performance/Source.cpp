#include <chrono>
#include <iostream>
#include <sstream>

static double fastPow(double val, int exponent) {
    if (exponent < 0) {
        val = 1.0 / val;
        exponent = -exponent - 1;
    }

    double result(1.0), currVal(val);
    int currValExp(1), remainingExp(exponent - 1);

    while (remainingExp > 0) {
        while (currValExp <= remainingExp) {
            currVal *= currVal;
            remainingExp -= currValExp;
            currValExp *= 2;
        }
        result *= currVal;
        currVal = val;
        currValExp = 1;
        --remainingExp;
    };

    return result;
}

static inline double fasterPow(double val, unsigned int exp) {
    double currVal(val);
    unsigned int currValExp(1), remainingExp(exp - 1);

    while (currValExp <= remainingExp) {
        currVal *= currVal;
        remainingExp -= currValExp;
        currValExp *= 2;
    }

    if (remainingExp) {
        return currVal * fasterPow(val, remainingExp);
    }
    else {
        return currVal;
    }
}

static double fasterPow(double val, int exp) {
    if (exp > 0) {
        return fasterPow(val, unsigned int(exp));
    }
    else if (exp < 0) {
        return 1.0 / fasterPow(val, unsigned int(-exp));
    }
    else {
        return 1.0;
    }
}

static inline double fastestPow(double val, unsigned int exp) {
    double res(1.0);

    do {
        if (exp & 1) res *= val; // exponent is odd
        exp >>= 1;
        val *= val;
    } while (exp);

    return res;
}

static inline double fastestPow(double val, int exp) {
    if (exp >= 0) {
        return fastestPow(val, unsigned int(exp));
    }
    else {
        return fastestPow(1.0 / val, unsigned int(-exp));
    }
}

int main() {
    constexpr int k_sets(100), k_reps(100000);
    double * vals = new double[k_reps];
    int64_t * exponents = new int64_t[k_reps];
    double * dexponents = new double[k_reps];
    for (int i(0); i < k_reps; ++i) {
        vals[i] = double(std::rand() % 10000);
        exponents[i] = std::rand() % 200 - 100;
        dexponents[i] = double(exponents[i]);
    }

    
    double a(fastestPow(10.0, 1000000));
    double b(fastestPow(1.0, 0));
    double c(fastestPow(0.0, 0));

    /*if (true) {
        for (int i = 0; i < k_reps; ++i) {
            double d1(std::pow(vals[i], dexponents[i]));
            double d2(fastestPow(vals[i], int(exponents[i])));
            int x = 9;
        }
    }*/

    double t1(0.0), t2(0.0);
    volatile double v = 0.0;
    for (int i = 0; i < k_sets; ++i) {
        {
            auto then(std::chrono::high_resolution_clock::now());
            for (int j = 0; j < k_reps; ++j) {
                v = std::pow(vals[j], dexponents[j]);
            }
            t1 += (std::chrono::high_resolution_clock::now() - then).count() * 1.0e-9;
        }
        {
            auto then(std::chrono::high_resolution_clock::now());
            for (int j = 0; j < k_reps; ++j) {
                v = fastestPow(vals[j], int(exponents[j]));
            }
            t2 += (std::chrono::high_resolution_clock::now() - then).count() * 1.0e-9;
        }
    }
    std::cout << v << std::endl;
    std::cout << t1 << " " << t2 << std::endl;
    std::cin.get();

    return 0;
}