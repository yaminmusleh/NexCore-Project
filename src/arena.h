#pragma once

#include <cstddef>
#include <cstdlib>
#include <memory>
#include <new>

class ArenaAllocator {
public:
    //i need a constructor and a destructor and a copy constructor
    inline explicit ArenaAllocator(size_t bytes)
        : m_size(bytes) //initializer
    {
        m_buffer = static_cast<std::byte *>(std::malloc(m_size));
        if (!m_buffer)
            throw std::bad_alloc{};
        m_offset = m_buffer; // this offset is now pointing at the buffer's location.
    }

    template<typename T> //needing a template that returns a pointer

    T *alloc() {
        std::size_t space = static_cast<std::size_t>((m_buffer + m_size) - m_offset);
        void *current = m_offset;

        if (!std::align(alignof(T), sizeof(T), current, space))
            return nullptr;

        T *ptr = static_cast<T *>(current);
        m_offset = static_cast<std::byte *>(current) + sizeof(T);

        return ptr;
    }

    inline ArenaAllocator(const ArenaAllocator &) = delete;

    inline ArenaAllocator &operator=(const ArenaAllocator &) = delete;

    inline ~ArenaAllocator() {
        std::free(m_buffer);
    }

private:
    size_t m_size;
    std::byte *m_buffer; //  a member (pointer) that points to the beginning of the allocated block.
    std::byte *m_offset; // this pointer points at where the buffer is located.
};
