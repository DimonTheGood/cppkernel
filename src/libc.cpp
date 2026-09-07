#include "libc.hpp"

void* memset(void* dest, int value, uint32_t count){
    auto* temp_dest = reinterpret_cast<uint8_t*>(dest);
    for(uint32_t i = 0; i < count; ++i){
        *(temp_dest + i) = value;
    }
    return dest; 
}

void* memcpy(void* dest, const void* src, uint32_t count){
    auto* temp_dest = reinterpret_cast<uint8_t*>(dest);
    auto* temp_src = reinterpret_cast<const uint8_t*>(src);
    for(uint32_t i = 0; i < count; ++i){
        *(temp_dest + i) = *(temp_src + i);
    }
    return dest;
}

void* memmove(void* dest, const void* src, uint32_t count){
    auto* temp_dest = reinterpret_cast<uint8_t*>(dest);
    auto* temp_src = reinterpret_cast<const uint8_t*>(src);
    if(dest < src){
        for(uint32_t i = 0; i < count; ++i){
            *(temp_dest + i) = *(temp_src + i);
        }
    }else{
        for(uint32_t i = 0; i < count; ++i){
            *(temp_dest + (count - i - 1)) = *(temp_src + (count - i - 1));
        }
    }
    return dest;
}

int memcmp(const void* a, const void* b, uint32_t count){
    auto* temp_a = reinterpret_cast<const uint8_t*>(a);
    auto* temp_b = reinterpret_cast<const uint8_t*>(b);
    for(uint32_t i = 0; i < count; ++i){
        if(*(temp_a + i) < *(temp_b + i)) return -1;
        if(*(temp_a + i) > *(temp_b + i)) return 1;
    }
    return 0;
}

uint32_t strlen(const char* str){
    const char* iter = str;
    uint32_t result = 0;
    while(*iter != '\0'){
        result++;
        iter++;
    }
    return result;
}