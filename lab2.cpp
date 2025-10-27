#include <iostream>
#include <vector>
#include <algorithm>
#include <execution>
#include <thread>
#include <random>
#include <chrono>
#include <iomanip>
#include <windows.h>

using namespace std;
using namespace chrono;

// --- Генерація випадкових даних ---
vector<int> generate_data(size_t n) {
    random_device rd;
    mt19937 gen(rd());
    uniform_int_distribution<int> dist(0, 1000);
    vector<int> v(n);
    for (auto& x : v) x = dist(gen);
    return v;
}

// --- Власна паралельна версія replace_if ---
void parallel_replace_if(vector<int>& data, int K, auto pred, int new_val) {
    size_t n = data.size();
    if (K <= 1 || n < K) {
        replace_if(data.begin(), data.end(), pred, new_val);
        return;
    }

    vector<thread> threads;
    size_t chunk = n / K;
    for (int i = 0; i < K; ++i) {
        size_t start = i * chunk;
        size_t end = (i == K - 1) ? n : start + chunk;
        threads.emplace_back([&, start, end]() {
            replace_if(data.begin() + start, data.begin() + end, pred, new_val);
            });
    }

    for (auto& t : threads) t.join();
}

// --- Вимірювання часу ---
template <typename Func>
long long measure_ms(Func f) {
    auto t1 = high_resolution_clock::now();
    f();
    auto t2 = high_resolution_clock::now();
    return duration_cast<milliseconds>(t2 - t1).count();
}

// --- Основна програма ---
int main() {
    SetConsoleOutputCP(65001);  
    SetConsoleCP(65001);
    ios::sync_with_stdio(false);
    cin.tie(nullptr);

    cout << fixed << setprecision(3);
    unsigned threads = thread::hardware_concurrency();
    if (threads == 0) threads = 4;

    cout << "Hardware threads: " << threads << "\n\n";
    vector<size_t> sizes = { 100000, 1000000, 10000000 };

    for (size_t n : sizes) {
        cout << "---- N = " << n << " ----\n";
        auto base = generate_data(n);

        auto pred = [](int x) { return x % 7 == 0; };
        int new_val = -1;

        // 1) Без політики
        {
            auto data = base;
            auto t = measure_ms([&]() {
                replace_if(data.begin(), data.end(), pred, new_val);
                });
            cout << "No policy: " << t << " ms\n";
        }

        // 2) З політиками
        {
            auto data = base;
            auto t = measure_ms([&]() {
                replace_if(execution::seq, data.begin(), data.end(), pred, new_val);
                });
            cout << "seq: " << t << " ms\n";
        }

        {
            auto data = base;
            auto t = measure_ms([&]() {
                replace_if(execution::par, data.begin(), data.end(), pred, new_val);
                });
            cout << "par: " << t << " ms\n";
        }

        {
            auto data = base;
            auto t = measure_ms([&]() {
                replace_if(execution::par_unseq, data.begin(), data.end(), pred, new_val);
                });
            cout << "par_unseq: " << t << " ms\n";
        }

        // 3) Власний паралельний алгоритм
        cout << "\nCustom parallel replace_if (K threads):\n";
        cout << setw(8) << "K" << setw(12) << "       time (мс)\n";
        cout << "------------------\n";

        long long best_time = LLONG_MAX;
        int best_K = 1;
           
        for (int K = 1; K <= (int)threads * 2; ++K) {
            auto data = base;
            auto t = measure_ms([&]() {
                parallel_replace_if(data, K, pred, new_val);
                });
            cout << setw(8) << K << setw(12) << t << "\n";
            if (t < best_time) {
                best_time = t;
                best_K = K;
            }
        }

        cout << "\nBest K = " << best_K
            << " (≈ " << fixed << setprecision(2)
            << (double)best_K / threads << "× CPU threads)\n\n";


    }
    return 0;
}
