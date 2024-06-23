#pragma once

#include <atomic>
#include <cstddef>
#include <vector>

template<typename T>
class SPSCBuffer {
  public:
    /* +1 required to differentiate between empty and full */
    SPSCBuffer(size_t capacity)
      : buffer(capacity + 1) {}

    bool push(const T in) {
        const size_t write = write_.load(std::memory_order_relaxed);
        const size_t next  = increment(write);

        if (next == read_.load(std::memory_order_acquire)) {
            return false; /* buffer full */
        }

        buffer[write] = in;
        write_.store(next, std::memory_order_release);
        return true;
    }

    bool pop(T& out) {
        const size_t read = read_.load(std::memory_order_relaxed);

        if (read == write_.load(std::memory_order_acquire)) {
            return false; /* buffer empty */
        }

        out = buffer[read];
        read_.store(increment(read), std::memory_order_release);
        return true;
    }

  private:
    size_t increment(size_t i) const { return (i + 1) % buffer.size(); }

    std::vector<T> buffer;

    std::atomic<size_t> write_{ 0 };
    std::atomic<size_t> read_{ 0 };
};
