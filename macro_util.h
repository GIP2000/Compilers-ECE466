#pragma once

typedef unsigned long long u64;
typedef long long i64;
#define PTR_RETURN(ptr, value)                                                 \
    if ((ptr) != NULL) {                                                       \
        *(ptr) = value;                                                        \
    }
