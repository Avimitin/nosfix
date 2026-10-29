#pragma once

// Counting test harness. Tests are built with /O2 /DNDEBUG, so assert() is
// never used for checks.

#include <atomic>
#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <new>

namespace test {

inline int& FailureCount() noexcept {
    static int failures = 0;
    return failures;
}

inline void Check(bool condition, const char* expression, const char* file,
                  int line) noexcept {
    if (condition) return;
    ++FailureCount();
    std::printf("FAIL %s:%d  %s\n", file, line, expression);
}

inline int Finish(const char* name) noexcept {
    if (FailureCount() == 0) {
        std::printf("PASS %s\n", name);
        return 0;
    }
    std::printf("FAILED %s (%d failed checks)\n", name, FailureCount());
    return 1;
}

inline std::atomic<long long>& AllocationCount() noexcept {
    static std::atomic<long long> count{0};
    return count;
}

inline std::atomic<bool>& AllocationCounting() noexcept {
    static std::atomic<bool> enabled{false};
    return enabled;
}

inline void BeginAllocationProbe() noexcept {
    AllocationCount().store(0, std::memory_order_relaxed);
    AllocationCounting().store(true, std::memory_order_release);
}

inline long long EndAllocationProbe() noexcept {
    AllocationCounting().store(false, std::memory_order_release);
    return AllocationCount().load(std::memory_order_relaxed);
}

} // namespace test

#define CHECK(expression) \
    ::test::Check(static_cast<bool>(expression), #expression, __FILE__, __LINE__)

void* operator new(std::size_t size) {
    if (test::AllocationCounting().load(std::memory_order_acquire)) {
        test::AllocationCount().fetch_add(1, std::memory_order_relaxed);
    }
    void* memory = std::malloc(size == 0 ? 1 : size);
    if (memory == nullptr) throw std::bad_alloc();
    return memory;
}

void* operator new[](std::size_t size) {
    if (test::AllocationCounting().load(std::memory_order_acquire)) {
        test::AllocationCount().fetch_add(1, std::memory_order_relaxed);
    }
    void* memory = std::malloc(size == 0 ? 1 : size);
    if (memory == nullptr) throw std::bad_alloc();
    return memory;
}

void operator delete(void* memory) noexcept { std::free(memory); }
void operator delete[](void* memory) noexcept { std::free(memory); }
void operator delete(void* memory, std::size_t) noexcept { std::free(memory); }
void operator delete[](void* memory, std::size_t) noexcept { std::free(memory); }
