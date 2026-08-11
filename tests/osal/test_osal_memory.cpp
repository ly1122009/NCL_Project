#include "../../tools/test-support/catch2/catch.hpp"
#include "../../feature/OSAL/OSAL_Memory.h"
#include <string.h>

TEST_CASE("OSAL_Memory: Malloc and Free", "[osal][memory]") {
    size_t size = 1024;
    NCL_PTR ptr = NCL_OSAL_Malloc(size);
    
    REQUIRE(ptr!= NULL);
    
    NCL_OSAL_Free(ptr);
}

TEST_CASE("OSAL_Memory: Memset", "[osal][memory]") {
    size_t size = 100;
    NCL_PTR ptr = NCL_OSAL_Malloc(size);
    REQUIRE(ptr!= NULL);

    unsigned char pattern = 0xAA;
    NCL_OSAL_Memset(ptr, pattern, size);

    unsigned char* byte_ptr = (unsigned char*)ptr;
    for (size_t i = 0; i < size; ++i) {
        REQUIRE(byte_ptr[i] == pattern);
    }

    NCL_OSAL_Free(ptr);
}

TEST_CASE("OSAL_Memory: Memcpy", "[osal][memory]") {
    size_t size = 50;
    NCL_PTR src = NCL_OSAL_Malloc(size);
    NCL_PTR dest = NCL_OSAL_Malloc(size);
    REQUIRE(src!= NULL);
    REQUIRE(dest!= NULL);

    // Initialize src with pattern
    for (size_t i = 0; i < size; ++i) {
        ((unsigned char*)src)[i] = (unsigned char)i;
    }

    NCL_OSAL_Memcpy(dest, src, size);

    unsigned char* src_ptr = (unsigned char*)src;
    unsigned char* dest_ptr = (unsigned char*)dest;
    for (size_t i = 0; i < size; ++i) {
        REQUIRE(dest_ptr[i] == src_ptr[i]);
    }

    NCL_OSAL_Free(src);
    NCL_OSAL_Free(dest);
}

TEST_CASE("OSAL_Memory: Memmove (Overlapping)", "[osal][memory]") {
    size_t size = 20;
    NCL_PTR ptr = NCL_OSAL_Malloc(size);
    REQUIRE(ptr!= NULL);

    unsigned char* data = (unsigned char*)ptr;
    for (size_t i = 0; i < size; ++i) {
        data[i] = (unsigned char)i;
    }

    // Overlap: move data from index 0 to index 5
    // Original: 0, 1, 2, 3, 4, 5, 6, 7...
    // Target:   ,?,?,?,?, 0, 1, 2, 3, 4...
    NCL_OSAL_Memmove(data + 5, data, 10);

    for (size_t i = 0; i < 10; ++i) {
        REQUIRE(data[5 + i] == (unsigned char)i);
    }

    NCL_OSAL_Free(ptr);
}
