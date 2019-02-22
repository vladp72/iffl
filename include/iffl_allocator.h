#pragma once
//
// Author: Vladimir Petter
// github: https://github.com/vladp72/iffl
//
//
// This module implements
// debug_memory_resource a memory resource that can be used along
// with polimorfic allocator for debugging contained.
//
//

#include <iffl_config.h>
#include <iffl_common.h>

//
// intrusive flat forward list
//
namespace iffl {

//
// This class can be used with Polimorfic Memory Allocator.
// Forward Linked List has typedef for PMR called pmr_flat_forward_list.
//
// Sample usage:
// iffl::debug_memory_resource dbg_memory_resource;
// iffl::pmr_flat_forward_list<FLAT_FORWARD_LIST_TEST> ffl{ &dbg_memory_resource };
// Code should guarantee that memory resource is destroyed after all containers that 
// use it are destroyed.
//
// This calss provides some basic buffer overrun and underrun detection, and
// also diring destruction checks that we did not leak any allocated blocks.
//
//   |---------------------|-------------------|----------------------------------|------------|----------------------------------|
//   sizeof(size_t) bytes  pointer to          sizeof(size_t) prefix               user data       sizeof(size_t) suffix
//   offset to the end of  memory resource     0xBEEFABCD - allocated block        filled with     0xDEADABCD - allocated block
//   allocation                                0xBEEFABCE - deallocated block      0xFE            0xDEADABCE - deallocated block
//
// On deallocation it checks that prefix and suffix contain expected pattern
// On allocation user buffer is filled with 0xFE to help detecting use of uninitialized memory
//
class debug_memory_resource 
    : public std::pmr::memory_resource {

public:

    debug_memory_resource() = default;

    ~debug_memory_resource() {
        validate_no_busy_blocks();
    }

    size_t get_busy_blocks_count() const {
        return busy_blocks_count_.load(std::memory_order_relaxed);
    }

    void validate_no_busy_blocks() const {
        FFL_CODDING_ERROR_IF(0 < get_busy_blocks_count());
    }

private:

    using pattern_type = size_t;   

    struct prefix_t {
        size_t size;
        size_t alignment;
        debug_memory_resource *memory_resource;
        pattern_type pattern;
    };

    struct suffix_t {
        pattern_type pattern;
    };

    constexpr static size_t min_allocation_size{ sizeof(prefix_t) + sizeof(suffix_t) };

    constexpr static pattern_type busy_block_prefix_pattern{ 0xBEEFABCD };
    constexpr static pattern_type free_block_prefix_pattern{ 0xBEEFABCE };
    constexpr static pattern_type busy_block_suffix_pattern{ 0xDEADABCD };
    constexpr static pattern_type free_block_suffix_pattern{ 0xDEADABCE };

    constexpr static int fill_pattern{ 0xFE };

    void* do_allocate(size_t bytes, size_t alignment) override {

        size_t bytes_with_debug_info = bytes + min_allocation_size;
        void *ptr = _aligned_malloc(bytes_with_debug_info, alignment);
        if (nullptr == ptr) {
            throw std::bad_alloc{};
        }

        prefix_t *prefix{ reinterpret_cast<prefix_t *>(ptr) };
        suffix_t *suffix{ reinterpret_cast<suffix_t *>( reinterpret_cast<char *>(ptr) + bytes_with_debug_info) - 1 };

        prefix->size = bytes_with_debug_info;
        prefix->alignment = alignment;
        prefix->memory_resource = this;
        prefix->pattern = busy_block_prefix_pattern;
        suffix->pattern = busy_block_suffix_pattern;

        increment_busy_block_count();

        char *user_buffer = reinterpret_cast<char *>(prefix + 1);
        fill_buffer(user_buffer, fill_pattern, bytes);

        return user_buffer;
    }

    void do_deallocate(void* p, size_t bytes, size_t alignment) noexcept override {

        decrement_busy_block_count();

        prefix_t *prefix{ reinterpret_cast<prefix_t *>(p) - 1 };
        FFL_CODDING_ERROR_IF_NOT(prefix->pattern == busy_block_prefix_pattern);
        FFL_CODDING_ERROR_IF_NOT(prefix->memory_resource == this);
        FFL_CODDING_ERROR_IF_NOT(prefix->size > min_allocation_size);
        size_t user_allocation_size = prefix->size - min_allocation_size;
        FFL_CODDING_ERROR_IF_NOT(user_allocation_size == bytes);
        FFL_CODDING_ERROR_IF_NOT(prefix->alignment == alignment);
        suffix_t *suffix{ reinterpret_cast<suffix_t *>(reinterpret_cast<char *>(prefix) + prefix->size) - 1 };
        FFL_CODDING_ERROR_IF_NOT(suffix->pattern == busy_block_suffix_pattern);
        prefix->pattern = free_block_prefix_pattern;
        suffix->pattern = free_block_suffix_pattern;

        _aligned_free(prefix);
    }

    bool do_is_equal(memory_resource const & other) const noexcept override {
        return (&other == static_cast<memory_resource const *>(this));
    }

    void increment_busy_block_count() {
        size_t prev_busy_blocks_count = busy_blocks_count_.fetch_add(1, std::memory_order_relaxed);
        FFL_CODDING_ERROR_IF_NOT(prev_busy_blocks_count >= 0);
    }

    void decrement_busy_block_count() {
        size_t prev_busy_blocks_count = busy_blocks_count_.fetch_sub(1, std::memory_order_relaxed);
        FFL_CODDING_ERROR_IF_NOT(prev_busy_blocks_count > 0);
    }

    //bool enable_detecting
    std::atomic<size_t> busy_blocks_count_{ 0 };
};

} // namespace iffl
