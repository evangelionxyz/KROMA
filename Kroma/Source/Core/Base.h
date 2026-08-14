// Copyright (c) 2026 Evangelion Manuhutu

#ifndef _BASE_H
#define _BASE_H

#define KR_SUCCESS 1
#define KR_FAILURE 0

#define KR_TRUE 1
#define KR_FALSE 0

#define KR_RESULT int

#define ARRAY_SIZE(_ARR) ((int)(sizeof(_ARR) / sizeof(*(_ARR))))

#if defined(_WIN32)
    #if defined(KROMA_BUILD_SHARED)
        #define KR_API __declspec(dllexport)
    #else
        #define KR_API __declspec(dllimport)
    #endif
#elif defined(__GNUC__) || defined(__clang__)
    #if defined(KROMA_BUILD_SHARED)
        #define KR_API __attribute__((visibility("default")))
    #else
        #define KR_API
    #endif
#endif

#endif