#pragma once


class ArenaAllocator {
public:
    //i need a constructor and a destructor and a copy constructor
    inline explicit ArenaAllocator(size_t bytes)
        : m_size(bytes) //initializer
    {
        m_buffer = malloc(m_size);
    }
    inline ArenaAllocator(ArenaAllocator& other) = delete;

    inline ArenaAllocator operator = (const ArenaAllocator& other) = delete;

    inline ~ArenaAllocator() {
        free(m_buffer);
    }

private:
    size_t m_size;
    void* m_buffer; //  a member (pointer) that points to the beginning of the allocated block.
};
