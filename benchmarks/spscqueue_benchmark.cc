#include <atomic>
#include <benchmark/benchmark.h>
#include <cstdint>
#include <iostream>
#include <thread>

#include <fiah/structs/SPSCQueue.hh>
#include <fiah/structs/ThreadSafeQueue.hh>

static void BM_SPSCQueue_Push(benchmark::State &state)
{
    fiah::SPSCQueue<int, 1024> queue;
    int value = 42;

    for (auto _ : state)
    {
        queue.push(value);
    }
}

static void BM_SPSCQueue_Fill_Milli(benchmark::State &state)
{
    constexpr size_t N = 1 << 20; // A milli
    fiah::SPSCQueue<int, N> queue;

    for (auto _ : state)
    {
        for (size_t i = 0; i < N; ++i)
            queue.push(static_cast<int>(i));
        benchmark::ClobberMemory(); // force materialization
    }
}

static void BM_SPSCQueue_Multithreaded(benchmark::State &state)
{
    using ElementType = std::uint64_t;
    constexpr std::size_t N = (1 << 30) / sizeof(ElementType); // 32MB for 4byte type
    static fiah::SPSCQueue<ElementType, N> queue;
    std::cout << "element size spsc: " << sizeof(ElementType) << ", struct size: " << sizeof(queue) << std::endl;
    for (auto _ : state)
    {
        std::atomic<bool> done{false};
        std::atomic<bool> start{false};

        std::jthread producer([&]() {
            while (!start.load(std::memory_order_relaxed))
                ;

            for (std::size_t i{}; i < N; ++i)
                while (!queue.push(i))
                    ;

            done.store(true, std::memory_order_release);
            benchmark::ClobberMemory();
        });

        std::jthread consumer([&]() {
            ElementType tmp;
            int popped{};

            start.store(true, std::memory_order_release);
            while (!done.load(std::memory_order_acquire) || !queue.empty())
            {
                if (queue.pop(tmp))
                {
                    benchmark::DoNotOptimize(tmp);
                    ++popped;
                    // benchmark::DoNotOptimize(popped);
                }
            }
            benchmark::ClobberMemory();
        });
    }
    state.SetItemsProcessed(N * state.iterations());
}

static void BM_ThreadSafeQueue_Multithreaded(benchmark::State &state)
{
    using ElementType = int;
    constexpr std::size_t N = (1 << 30) / sizeof(ElementType); // 32MB
    static fiah::ThreadSafeQueue<ElementType> queue;
    std::cout << "element size threadsafeq: " << sizeof(ElementType) << ", struct size: " << sizeof(queue) << std::endl;
    for (auto _ : state)
    {
        std::atomic<bool> done{false};
        std::atomic<bool> start{false};

        std::jthread producer([&]() {
            ElementType someval{1337UL};
            while (!start.load(std::memory_order_relaxed))
                ;

            for (std::size_t i{}; i < N; ++i)
                queue.push(someval);

            done.store(true, std::memory_order_release);
        });

        std::jthread consumer([&]() {
            ElementType tmp;
            int popped{};

            start.store(true, std::memory_order_release);
            while (!done.load(std::memory_order_acquire) || !queue.empty())
            {
                // Cool C++17 syntax, but won't compile if T is no
                // convertible to bool
                // if (tmp = queue.wait_and_pop(); tmp)
                if (queue.empty())
                {
                    // This might bias the benchmark
                    // std::this_thread::yield();
                    continue;
                }
                tmp = queue.wait_and_pop();
                benchmark::DoNotOptimize(tmp);
                ++popped;
            }
            benchmark::ClobberMemory();
        });
        // benchmark::ComplexityN();
    }
    state.SetItemsProcessed(N * state.iterations());
}

BENCHMARK(BM_SPSCQueue_Push);
BENCHMARK(BM_SPSCQueue_Fill_Milli);
BENCHMARK(BM_SPSCQueue_Multithreaded);
BENCHMARK(BM_ThreadSafeQueue_Multithreaded);

BENCHMARK_MAIN();