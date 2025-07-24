#pragma once

#ifdef __cplusplus
#define MECS_EXTERNCPP()    extern "C" {
#define MECS_ENDEXTERNCPP() }
#else
#define MECS_EXTERNCPP(...)
#define MECS_ENDEXTERNCPP()
#endif

#ifdef WIN32
#ifdef MECS_EXPORT
#define MECS_API __declspec(dllexport)
#else
#define MECS_API __declspec(dllimport)
#endif
#else
#define MECS_API
#endif