#pragma once

//!
//! @file iffl_allocator.h
//!
//! @author Vladimir Petter
//!
//! [iffl github](https://github.com/vladp72/iffl)
//!
//! @brief This module implements debug_memory_resource a memory resource that can be used along
//!        with polimorfic allocator for debugging contained.
//!

#include <iffl_config.h>
#include <iffl_common.h>

//!
//! @namespace iffl
//! @brief intrusive flat forward list
//!
namespace iffl {

//!
//! @class debug_memory_resource 
//! @brief implements std::pmr::memory_resource interface. 
//! @details This class can be used with Polimorfic Memory Allocator.
//! Forward Linked List has typedef for PMR called pmr_flat_forward_list.
//! 
//! Sample usage:
//!
//! @code
//! iffl::debug_memory_resource dbg_memory_resource;
//! iffl::pmr_flat_forward_list<FLAT_FORWARD_LIST_TEST> ffl{ &dbg_memory_resource };
//! @endcode
//!
//! Code should guarantee that memory resource is destroyed after all memory allocated 
//! from this allocator is deallocated.
//! 
//! This class provides:
//! - Basic buffer overrun and underrun detection by plang well knwn patters at the end and at the beginnig
//!   of the buffer. At deallocation time patterns we check that patterns did not change
//! - Detection that deallocation is done using same memory resource as allocation. Allocation header
//!   contains pointer to the memory resource. At deallocation time we check that pointer matches to the 
//!   object that is deallocating memoty.
//! - Helps detecting when objects use memory without previously initializing it. Allocate memory is filled with
//!   value that would trigger access violation if used as a pointer without initialization
//! - Basic memory leak and lifetime management issues by counting number of outstanding allocations.
//!   At memory resource destruction time we check that there are no outstanding allocations.
//!Table below describes layout of each allocated block.
//!
//! @code
//! --------------------------------------------------------------------------------------------------------------------
//! |sizeof(size_t) bytes | pointer to | sizeof(size_t) prefix          | user data   | sizeof(size_t) suffix          |
//! |offset to the end of | memory     | 0xBEEFABCD - allocated block   | filled with | 0xDEADABCD - allocated block   |
//! |allocation           | resource   | 0xBEEFABCE - deallocated block | 0xFE        | 0xDEADABCE - deallocated block |
//! --------------------------------------------------------------------------------------------------------------------
//! @endcode
//!
class debug_memory_resource 
    : public FFL_PMR::memory_resource {

public:

    debug_memory_resource() = default;

    //!
    //! @brief Destructor triggers fail fast if there are outstanding allocations 
    //!
    ~debug_memory_resource() noexcept {
        validate_no_busy_blocks();
    }

    //!
    //! @brief Can be used to query number of outstanding allocations
    //! @return number of outstanding allocations
    //!
    size_t get_busy_blocks_count() const noexcept {
        return busy_blocks_count_.load(std::memory_order_relaxed);
    }

    //!
    //! @brief Triggers fail fast if there are outstanding allocations
    //!
    void validate_no_busy_blocks() const noexcept {
        FFL_CODDING_ERROR_IF(0 < get_busy_blocks_count());
    }

private:

    using pattern_type = size_t;   
    //!
    //! @brief structure placed right before pointer to the user data
    //!
    struct prefix_t {
        size_t size;
        size_t alignment;
        debug_memory_resource *memory_resource;
        pattern_type pattern;
    };
    //!
    //! @brief structure placed after pointer to the user data
    //!
    struct suffix_t {
        pattern_type pattern;
    };
    //!
    //! @brief minimum allocation size always includes prefix, and suffix
    //!
    constexpr static size_t min_allocation_size{ sizeof(prefix_t) + sizeof(suffix_t) };

    constexpr static pattern_type busy_block_prefix_pattern{ 0xBEEFABCD };
    constexpr static pattern_type free_block_prefix_pattern{ 0xBEEFABCE };
    constexpr static pattern_type busy_block_suffix_pattern{ 0xDEADABCD };
    constexpr static pattern_type free_block_suffix_pattern{ 0xDEADABCE };

    constexpr static int fill_pattern{ 0xFE };
    //!
    //! @brief Overrides memory resource virtual method that performs allocation.
    //! @param bytes - number of bytes to be allocated.
    //! @param alignment - alignment requirements for the allocated buffer.
    //! @throws std::bad_alloc if allocation fails
    //! @return On success return a pointer to the buffer of requested size.
    //!         On failure throws std::bad_alloc.
    //!
    void* do_allocate(size_t bytes, [[maybe_unused]] size_t alignment) override {

        size_t bytes_with_debug_info = bytes + min_allocation_size;
        void *ptr = FFL_ALLIGNED_ALLOC(bytes_with_debug_info, alignment);
        if (nullptr == ptr) {
            throw std::bad_alloc{};
        }

        prefix_t *prefix{ static_cast<prefix_t *>(ptr) };
        suffix_t *suffix{ reinterpret_cast<suffix_t *>( static_cast<char *>(ptr) + bytes_with_debug_info) - 1 };

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
    //!
    //! @brief Overrides memory resource virtual method that performs deallocation.
    //! @param p - pointer to the user buffer that is deallocated.
    //! @param bytes - buffer size. Mast match to the size that was allocated.
    //! @param alignment - alignment of the buffer. Must match to alignment at allocation time.
    //! @return void. Checks buffer integrity and triggers fail fast if check does not pass.
    //!
    void do_deallocate(void* p, size_t bytes, [[maybe_unused]] size_t alignment) noexcept override {

        decrement_busy_block_count();

        prefix_t *prefix{ static_cast<prefix_t *>(p) - 1 };
        FFL_CODDING_ERROR_IF_NOT(prefix->pattern == busy_block_prefix_pattern);
        FFL_CODDING_ERROR_IF_NOT(prefix->memory_resource == this);
        FFL_CODDING_ERROR_IF_NOT(prefix->size > min_allocation_size);
        size_t const user_allocation_size = prefix->size - min_allocation_size;
        FFL_CODDING_ERROR_IF_NOT(user_allocation_size == bytes);
        FFL_CODDING_ERROR_IF_NOT(prefix->alignment == alignment);
        suffix_t *suffix{ reinterpret_cast<suffix_t *>(reinterpret_cast<char *>(prefix) + prefix->size) - 1 };
        FFL_CODDING_ERROR_IF_NOT(suffix->pattern == busy_block_suffix_pattern);

        prefix->memory_resource = nullptr;
        prefix->pattern = free_block_prefix_pattern;
        suffix->pattern = free_block_suffix_pattern;

        FFL_ALLIGNED_FREE(prefix);
    }
    //!
    //! @brief Validates that two memory resources are equivalent.
    //!        For this class they must be equal.
    //! @param other - reference to the other memory resource.
    //! @return true if other memory resource is the same object,
    //!         and false otherwise.
    //!
    bool do_is_equal(memory_resource const & other) const noexcept override {
        return (&other == this);
    }
    //!
    //! @brief Increases number of outstamding allocations.
    //!
    void increment_busy_block_count() noexcept {
        size_t const prev_busy_blocks_count = busy_blocks_count_.fetch_add(1, std::memory_order_relaxed);
        FFL_CODDING_ERROR_IF_NOT(prev_busy_blocks_count >= 0);
    }
    //!
    //! @brief Decreases number of outstamding allocations.
    //!
    void decrement_busy_block_count() noexcept {
        size_t const prev_busy_blocks_count = busy_blocks_count_.fetch_sub(1, std::memory_order_relaxed);
        FFL_CODDING_ERROR_IF_NOT(prev_busy_blocks_count > 0);
    }
    //!
    //! @brief number of outstanding allocations.
    //!
    std::atomic<size_t> busy_blocks_count_{ 0 };
};


//!
//! @class input_buffer_memory_resource 
//! @brief implements std::pmr::memory_resource interface. 
//! @details This class can be used with Polimorfic Memory Allocator.
//! Forward Linked List has typedef for PMR called pmr_flat_forward_list.
//! This memory resource allows constructing flat forward list on a buffer
//! owned by someone else.
//!
//! This memory resource can be instantiated to point to a buffer that
//! is owned elsewhere. 
//! Using this memory resource you can allocate buffer to a single owned
//! Consequent allocations will throw bad_alloc until buffer is deallcoated.
//! On deallocation it checks that deallocated buffer is the
//! same as allocation buffer. It will fail-fast on mismatch.
//! Deallocated buffer can be allocated again.
//! Deallocation does not free the buffer. It is assumed that
//! buffer is owned by someone else.
//! 
//! Sample usage:
//!
//! In the below example memory resource is instantiated to point to the input buffer.
//! Container is parametrized to use this memory resource.
//! We resize container buffer to consume entire input buffer in one allocation.
//! After that we keep appending data to the container try_* methods that
//! would not attempt to reallocate larger buffer, and consequently are noexcept as long 
//! as caller's functor does not throw. Instead they return false when buffer cannot 
//! fit new element.
//! Once a try_* function returns false, we know that buffer is filled, and we can exit.
//! To let caller know how much of the buffer was filled with data we will set
//! output_size to the size of part of the buffer used by the inserted elements.
//! Optionally, if we want caller to adopt buffer without verification, we can return 
//! offset to the last element in the buffer.
//! Container destructor will deallocate memory back to resource.
//! Memory resource destructor will detach from the buffer.
//! Buffer content remains untached, and still contains a valid list.
//! Once method returns, caller can examine buffer using flat forward list reference or
//! view or attach it to the container with allocator compatibe to how buffer was allocated.
//!
//! @code
//! void build_result(char *buffer, size_t buffer_size, size_t *output_size) {
//!     iffl::input_buffer_memory_resource buffer_memory_resource{buffer, buffer_size};
//!     iffl::pmr_flat_forward_list<FLAT_FORWARD_LIST_TEST> ffl{ &buffer_memory_resource };
//!     ffl.resize_buffer(buffer_size);
//!     for(;;) {
//!         if(!ffl.try_push_back(<data>, <data_size>) {
//!             break;
//!         }
//!     }
//!     *output_size = ffl.used_capacity();
//! }
//! @endcode
//!
//! Thread safety:
//!
//! This class is not thread safe and cannot be used to perform concurent 
//! allocations from multiple threads.
//!
class input_buffer_memory_resource
    : public FFL_PMR::memory_resource {

public:
    //!
    //! @brief Constructs memory resource with information
    //! about buffer that should be used for allocation
    //! @param buffer - pointer to the buffer that we will return on allocation
    //! @param buffer_size - size of the buffer
    //!
    explicit input_buffer_memory_resource(void *buffer, 
                                          size_t buffer_size) noexcept
        : used_(buffer ? true : false)
        , buffer_{buffer}
        , buffer_size_{ buffer_size } {
    }

    //!
    //! @brief Destructor verifies that there are no outstanding allocations
    //!
    ~input_buffer_memory_resource() noexcept {
        validate_no_busy_blocks();
    }

    //!
    //! @brief Can be used to query number of outstanding allocations
    //! @return number of outstanding allocations
    //!
    size_t get_busy_blocks_count() const noexcept {
        return (used_ ? 1 : 0);
    }

    //!
    //! @brief Triggers fail fast if there are outstanding allocations
    //!
    void validate_no_busy_blocks() const noexcept {
        FFL_CODDING_ERROR_IF(0 < get_busy_blocks_count());
    }

protected:

    //!
    //! @brief Overrides memory resource virtual method that performs allocation.
    //! @param bytes - number of bytes to be allocated.
    //! @param alignment - alignment requirements for the allocated buffer.
    //! @throws std::bad_alloc if allocation fails
    //! @return On success return a pointer to the buffer of requested size.
    //!         On failure throws std::bad_alloc.
    //!
    void* do_allocate(size_t bytes, [[maybe_unused]] size_t alignment) override {
        alignment;
        if (!used_ && buffer_ && bytes <= buffer_size_) {
            used_ = true;
            return buffer_;
        } else {
            throw std::bad_alloc{};
        }
    }
    //!
    //! @brief Overrides memory resource virtual method that performs deallocation.
    //! @param p - pointer to the user buffer that is deallocated.
    //! @param bytes - buffer size. Mast match to the size that was allocated.
    //! @param alignment - alignment of the buffer. Must match to alignment at allocation time.
    //! @return void. Checks buffer that buffer was allocated, and that deallocated buffer 
    //! match to the buffer that we own. Triggers fail fast if check does not pass.
    //!
    void do_deallocate(void* p, size_t bytes, [[maybe_unused]] size_t alignment) noexcept override {
        FFL_CODDING_ERROR_IF_NOT(p == buffer_ && bytes <= buffer_size_);
        used_ = false;
    }

    //!
    //! @brief Validates that two memory resources are equivalent.
    //!        For this class they must be equal.
    //! @param other - reference to the other memory resource.
    //! @return true if other memory resource is the same object,
    //!         and false otherwise.
    //!
    bool do_is_equal(memory_resource const & other) const noexcept override {
        return (&other == this);
    }

private:
    bool used_{ false };
    void *buffer_{ nullptr };
    size_t buffer_size_{ 0 };
};

} // namespace iffl
