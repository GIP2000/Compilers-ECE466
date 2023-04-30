#pragma once

typedef unsigned long long u64;
typedef long long i64;
typedef unsigned int u32;
typedef int i32;
#define PTR_RETURN(ptr, value)                                                 \
    if ((ptr) != NULL) {                                                       \
        *(ptr) = value;                                                        \
    }

#define MIN(A, B) (A) < (B) ? (A) : (B)

#define MAX(A, B) (A) > (B) ? (A) : (B)
