#include <algorithm>
#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <numeric>
#include <random>
#include <vector>


using Clock = std::chrono::steady_clock;

template <typename F>
double time_ms(F&& f)
{
    const auto start = Clock::now();
    f();
    const auto end = Clock::now();

    return std::chrono::duration<double, std::milli>(
        end - start
    ).count();
}

template <std::size_t PAYLOAD_SIZE>
struct Record {
    uint64_t key;
    uint64_t payload[PAYLOAD_SIZE];
};

template <std::size_t PAYLOAD_SIZE>
int run_benchmark()
{
    using Rec = Record<PAYLOAD_SIZE>;

    constexpr std::size_t N = 10'000'00;

    std::mt19937_64 rng(12345);

    // ------------------------------------------------------------
    // Generate one canonical input dataset.
    // ------------------------------------------------------------

    std::vector<Rec> original(N);

    for (Rec& r : original) {
        r.key = rng();

        for (uint64_t& x : r.payload) {
            x = rng();
        }
    }

    std::cout << "N            : " << N << '\n';
    std::cout << "Payload words: " << PAYLOAD_SIZE << '\n';
    std::cout << "Payload size : "
              << PAYLOAD_SIZE * sizeof(uint64_t)
              << " bytes\n";
    std::cout << "Record size  : "
              << sizeof(Rec)
              << " bytes\n";
    std::cout << "Data size    : "
              << (N * sizeof(Rec)) / (1024.0 * 1024.0)
              << " MiB\n\n";

    // ============================================================
    // METHOD 1:
    // Directly sort the objects themselves.
    // ============================================================

    std::vector<Rec> direct = original;

    double direct_sort_ms = time_ms([&]() {
        std::sort(
            direct.begin(),
            direct.end(),
            [](const Rec& a, const Rec& b) {
                return a.key < b.key;
            }
        );
    });

    uint64_t direct_checksum = 0;

    double direct_consume_ms = time_ms([&]() {
        for (const Rec& r : direct) {
            for (std::size_t j = 0; j < PAYLOAD_SIZE; ++j) {
                direct_checksum += r.payload[j];
            }
        }
    });

    // ============================================================
    // METHOD 2:
    // Sort only an index/permutation vector.
    // ============================================================

    const std::vector<Rec>& indirect = original;

    std::vector<uint32_t> permutation(N);

    std::iota(
        permutation.begin(),
        permutation.end(),
        0
    );

    double index_sort_ms = time_ms([&]() {
        std::sort(
            permutation.begin(),
            permutation.end(),
            [&](uint32_t a, uint32_t b) {
                return indirect[a].key < indirect[b].key;
            }
        );
    });

    uint64_t indirect_checksum = 0;

    double indirect_consume_ms = time_ms([&]() {
        for (std::size_t i = 0; i < N; ++i) {
            const Rec& r = indirect[permutation[i]];

            for (std::size_t j = 0; j < PAYLOAD_SIZE; ++j) {
                indirect_checksum += r.payload[j];
            }
        }
    });

    // ============================================================
    // Sanity checks
    // ============================================================

    bool direct_sorted = std::is_sorted(
        direct.begin(),
        direct.end(),
        [](const Rec& a, const Rec& b) {
            return a.key < b.key;
        }
    );

    bool indirect_sorted = true;

    for (std::size_t i = 1; i < N; ++i) {
        if (indirect[permutation[i - 1]].key >
            indirect[permutation[i]].key) {

            indirect_sorted = false;
            break;
        }
    }

    if (direct_checksum != indirect_checksum) {
        std::cerr << "ERROR: checksum mismatch\n";
        return 1;
    }

    // ============================================================
    // Results
    // ============================================================

    const double direct_total_ms =
        direct_sort_ms + direct_consume_ms;

    const double indirect_total_ms =
        index_sort_ms + indirect_consume_ms;

    std::cout << std::fixed << std::setprecision(3);

    std::cout
        << "================ DIRECT OBJECT SORT ================\n";
    std::cout << "Sort:       "
              << direct_sort_ms << " ms\n";
    std::cout << "Consume:    "
              << direct_consume_ms << " ms\n";
    std::cout << "Total:      "
              << direct_total_ms << " ms\n";
    std::cout << "Sorted:     "
              << (direct_sorted ? "yes" : "NO") << "\n\n";

    std::cout
        << "================ INDIRECT INDEX SORT ===============\n";
    std::cout << "Index sort: "
              << index_sort_ms << " ms\n";
    std::cout << "Consume:    "
              << indirect_consume_ms << " ms\n";
    std::cout << "Total:      "
              << indirect_total_ms << " ms\n";
    std::cout << "Sorted:     "
              << (indirect_sorted ? "yes" : "NO") << "\n\n";

    std::cout
        << "==================== COMPARISON ====================\n";

    std::cout << "Sort speedup (direct / index): "
              << direct_sort_ms / index_sort_ms
              << "x\n";

    std::cout << "Consumption slowdown (indirect / direct): "
              << indirect_consume_ms / direct_consume_ms
              << "x\n";

    std::cout << "End-to-end speedup (direct / indirect): "
              << direct_total_ms / indirect_total_ms
              << "x\n";

    std::cout << "\nchecksum = "
              << direct_checksum << '\n';

    return 0;
}

int main(int argc, char** argv)
{
    if (argc != 2) {
        std::cerr
            << "Usage: " << argv[0]
            << " <payload_size_in_uint64s>\n\n"
            << "Supported sizes: "
            << "1 3 7 15 31 63 127\n";
        return 1;
    }

    const int payload_size = std::atoi(argv[1]);

    switch (payload_size) {
        case 1:
            return run_benchmark<1>();

        case 3:
            return run_benchmark<3>();

        case 7:
            return run_benchmark<7>();

        case 15:
            return run_benchmark<15>();

        case 31:
            return run_benchmark<31>();

        case 63:
            return run_benchmark<63>();

        case 127:
            return run_benchmark<127>();

        default:
            std::cerr
                << "Unsupported payload size: "
                << payload_size << '\n'
                << "Supported sizes: "
                << "1 3 7 15 31 63 127\n";
            return 1;
    }
}