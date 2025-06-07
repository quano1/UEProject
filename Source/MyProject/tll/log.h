#pragma once

// #include <functional>
// #include <sched.h>
// #include <unistd.h>
// #include <errno.h>

// #include <cstdio>
// #include <cstring>

// #include <sstream>
// #include <thread>
// #include <string>
// #include <chrono>
// #include <array>

#include "HAL/PlatformTime.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Containers/StringConv.h"

DEFINE_LOG_CATEGORY_STATIC(TLL, Log, All);
#define TLL_DEV

#define FILEW_NAME ::utils::fileName(__FILEW__)
#define DEFAULT_LOG_TIMEOUT 5

#define TLL_LOG(severity, format, ...) UE_LOG(TLL, severity, TEXT("%.9f %d %s:%d (%s) " format), FPlatformTime::Seconds(), FPlatformTLS::GetCurrentThreadId(), FILEW_NAME, __LINE__, UTF8_TO_TCHAR(__func__), ##__VA_ARGS__)

#define TLL_LOG_VERB(format, ...) TLL_LOG(Verbose, format, ##__VA_ARGS__)
#define TLL_LOG_LOG(format, ...) TLL_LOG(Log, format, ##__VA_ARGS__)
#define TLL_LOG_WARN(format, ...) TLL_LOG(Warning, format, ##__VA_ARGS__)
#define TLL_LOG_ERROR(format, ...) TLL_LOG(Error, format, ##__VA_ARGS__)

#define TLL_PRINT_NO_LOG(color, seconds, name, format, ...) if(GetWorld()) { UKismetSystemLibrary::PrintString(GetWorld(), FString::Printf(TEXT("%s:%d (%s) "format), FILEW_NAME, __LINE__, UTF8_TO_TCHAR(__func__), ##__VA_ARGS__), true, false, color, seconds, FName(name)); }

#define TLL_PRINT_NO_LOG_UNIQUE(seconds, format, ...) if(GetWorld()) { UKismetSystemLibrary::PrintString(GetWorld(), FString::Printf(TEXT("%s:%d (%s) "format), FILEW_NAME, __LINE__, UTF8_TO_TCHAR(__func__), ##__VA_ARGS__), true, false, utils::LogColors[__LINE__%UE_ARRAY_COUNT(utils::LogColors)], seconds, FName(FString::Printf(TEXT("%s:%d"), FILEW_NAME, __LINE__))); }

#define TLL_PRINT_NO_LOG_UNIQUE0() if(GetWorld()) { UKismetSystemLibrary::PrintString(GetWorld(), FString::Printf(TEXT("%s:%d (%s)"), FILEW_NAME, __LINE__, UTF8_TO_TCHAR(__func__)), true, false, utils::LogColors[__LINE__%UE_ARRAY_COUNT(utils::LogColors)], 3, FName(FString::Printf(TEXT("%s%d"), FILEW_NAME, __LINE__))); }

#define TLL_PRINT_NO_LOG_IF(condition, color, seconds, name, format, ...) if(GetWorld() && (condition)) { UKismetSystemLibrary::PrintString(GetWorld(), FString::Printf(TEXT("%s:%d (%s) "format), FILEW_NAME, __LINE__, UTF8_TO_TCHAR(__func__), ##__VA_ARGS__), true, false, color, seconds, name); }

#define TLL_PRINT_TIME(color, seconds, format, ...) if(GetWorld()) { UKismetSystemLibrary::PrintString(GetWorld(), FString::Printf(TEXT("%s:%d (%s) "format), FILEW_NAME, __LINE__, UTF8_TO_TCHAR(__func__), ##__VA_ARGS__), true, false, color, seconds, NAME_None); } TLL_LOG_LOG(format, ##__VA_ARGS__)

#define TLL_PRINT(color, format, ...) TLL_PRINT_TIME(color, DEFAULT_LOG_TIMEOUT, format, ##__VA_ARGS__)
#define TLL_PRINT_DEFAULT(format, ...) TLL_PRINT_TIME(FLinearColor(0.0, 0.66, 1.0), DEFAULT_LOG_TIMEOUT, format, ##__VA_ARGS__)
#define TLL_PRINT_RED(format, ...) TLL_PRINT_TIME(FColor::Red, DEFAULT_LOG_TIMEOUT, format, ##__VA_ARGS__)
#define TLL_PRINT_GREEN(format, ...) TLL_PRINT_TIME(FColor::Green, DEFAULT_LOG_TIMEOUT, format, ##__VA_ARGS__)
#define TLL_PRINT_YELLOW(format, ...) TLL_PRINT_TIME(FColor::Yellow, DEFAULT_LOG_TIMEOUT, format, ##__VA_ARGS__)

#define TLL_LOG_AND_DO_IF(condition, doSomething) \
        if((condition)) { \
            TLL_LOG_WARN("FAILED, "#condition); \
            doSomething; \
        }

#define TLL_DO_IF(condition, doSomething) \
        if((condition)) { \
            doSomething; \
        }

namespace utils {

    inline constexpr wchar_t const* fileName(wchar_t const* path) {
        wchar_t const* file = path;
        while (*path) {
            if (*path++ == L'\\') {
                file = path;
            }
        }
        return file;
    }

    // template <typename T>
    // inline std::wstring toWstring(T val) {
    //     std::wstringstream ss;
    //     ss << val;
    //     return ss.str();
    // }

    static const FColor LogColors[] = {
        FColor::White,
        FColor::Black,
        FColor::Red,
        FColor::Green,
        FColor::Blue,
        FColor::Yellow,
        FColor::Cyan,
        FColor::Magenta,
        FColor::Orange,
        FColor::Purple,
        FColor::Turquoise,
        FColor::Silver,
        FColor::Emerald
    };


} // log


