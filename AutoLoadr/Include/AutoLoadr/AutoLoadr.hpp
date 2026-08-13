// Copyright (c) 2026 Evangelion Manuhutu
#pragma once
#ifndef _AUTOLOADR_H
#define _AUTOLOADR_H

#ifdef _WIN32
    #if defined(AUTOLOADR_SHARED_BUILD)
        #define AUTOLOADR_API __declspec(dllexport)
    #else
        #define AUTOLOADR_API __declspec(dllimport)
    #endif
#elif defined(__GNUC__) || defined(__clang__)
    #if defined(AUTOLOADR_SHARED_BUILD)
        #define AUTOLOADR_API __attribute__((visibility("default")))
    #else
        #define AUTOLOADR_API
    #endif
#endif

#endif