#pragma once

#define PTR_RETURN(ptr, value)                                                 \
    if ((ptr) != NULL) {                                                       \
        *(ptr) = value;                                                        \
    }
