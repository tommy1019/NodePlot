#pragma once

#include <expected>
#include <optional>
#include <string>

#ifdef SIG_INT_ON_ERROR
#include <csignal>
#endif

template <typename T>
using ErrorOr = std::expected<T, std::string>;

template <typename T>
std::optional<T> to_optional(ErrorOr<T> v) {
    if (v.has_value())
        return *v;
    return {};
}

#ifndef WASM

#define COLOR_RED "\x1B[31m"
#define COLOR_GRN "\x1B[32m"
#define COLOR_YEL "\x1B[33m"
#define COLOR_BLU "\x1B[34m"
#define COLOR_MAG "\x1B[35m"
#define COLOR_CYN "\x1B[36m"
#define COLOR_WHT "\x1B[37m"
#define COLOR_RESET "\x1B[0m"

#else

#define COLOR_RED ""
#define COLOR_GRN ""
#define COLOR_YEL ""
#define COLOR_BLU ""
#define COLOR_MAG ""
#define COLOR_CYN ""
#define COLOR_WHT ""
#define COLOR_RESET ""

#endif

#ifdef SIG_INT_ON_ERROR
#define ERR(...)                                                                                                                                                                                       \
    ({                                                                                                                                                                                                 \
        std::raise(SIGINT);                                                                                                                                                                            \
        std::unexpected(__VA_ARGS__);                                                                                                                                                                  \
    })

#else
#define ERR(...) std::unexpected(__VA_ARGS__);
#endif

#define MAYBE(...) (void)!__VA_ARGS__

#define ABORT(error)                                                                                                                                                                                   \
    std::print("Error: {}\n", error);                                                                                                                                                                  \
    std::print(                                                                                                                                                                                        \
        "    The program aborted at line " COLOR_RED "{}" COLOR_RESET " of function " COLOR_YEL "{}" COLOR_RESET " in file " COLOR_CYN "{}" COLOR_RESET "\n", __LINE__, __FUNCTION__, __FILE__);       \
    std::fflush(stdout);                                                                                                                                                                               \
    abort();

#define MUST(...)                                                                                                                                                                                      \
    ({                                                                                                                                                                                                 \
        auto RESULT = __VA_ARGS__;                                                                                                                                                                     \
        if (!RESULT.has_value()) {                                                                                                                                                                     \
            auto error = std::move(RESULT.error());                                                                                                                                                    \
            ABORT(error);                                                                                                                                                                              \
        }                                                                                                                                                                                              \
        *std::move(RESULT);                                                                                                                                                                            \
    })

#define TRY(...)                                                                                                                                                                                       \
    ({                                                                                                                                                                                                 \
        auto RESULT = __VA_ARGS__;                                                                                                                                                                     \
        if (!(RESULT).has_value()) {                                                                                                                                                                   \
            return std::unexpected(std::move(RESULT.error()));                                                                                                                                         \
        }                                                                                                                                                                                              \
        *std::move(RESULT);                                                                                                                                                                            \
    })

#define TRY_OR(EVAL, ...)                                                                                                                                                                              \
    ({                                                                                                                                                                                                 \
        auto RESULT = EVAL;                                                                                                                                                                            \
        if (!(RESULT).has_value()) {                                                                                                                                                                   \
            __VA_ARGS__;                                                                                                                                                                               \
            return std::unexpected(std::move(RESULT.error()));                                                                                                                                         \
        }                                                                                                                                                                                              \
        *std::move(RESULT);                                                                                                                                                                            \
    })

#define REQUIRE(TEST, MESSAGE, ...)                                                                                                                                                                    \
    {                                                                                                                                                                                                  \
        if (!(TEST)) {                                                                                                                                                                                 \
            std::print("Error: " MESSAGE "\n", ##__VA_ARGS__);                                                                                                                                         \
            std::print("    The error occurred in function " COLOR_YEL "{}" COLOR_RESET " at " COLOR_CYN "{}" COLOR_RESET ":" COLOR_RED "{}" COLOR_RESET "\n", __FUNCTION__, __FILE__, __LINE__);      \
            std::fflush(stdout);                                                                                                                                                                       \
            abort();                                                                                                                                                                                   \
        }                                                                                                                                                                                              \
    }

#define REQUIRE_EQUALS(TEST, VALUE, MESSAGE, ...)                                                                                                                                                      \
    {                                                                                                                                                                                                  \
        auto RESULT = TEST;                                                                                                                                                                            \
        if (RESULT != VALUE) {                                                                                                                                                                         \
            std::print("Error: " MESSAGE "\n", ##__VA_ARGS__);                                                                                                                                         \
            std::print("    The error occurred in function " COLOR_YEL "{}" COLOR_RESET " at " COLOR_CYN "{}" COLOR_RESET ":" COLOR_RED "{}" COLOR_RESET "\n", __FUNCTION__, __FILE__, __LINE__);      \
            std::fflush(stdout);                                                                                                                                                                       \
            abort();                                                                                                                                                                                   \
        }                                                                                                                                                                                              \
    }

#define REQUIRE_NOT_REACHED(MESSAGE, ...)                                                                                                                                                              \
    {                                                                                                                                                                                                  \
        std::print("Error: " MESSAGE "\n", ##__VA_ARGS__);                                                                                                                                             \
        std::print("    The error occurred in function " COLOR_YEL "{}" COLOR_RESET " at " COLOR_CYN "{}" COLOR_RESET ":" COLOR_RED "{}" COLOR_RESET "\n", __FUNCTION__, __FILE__, __LINE__);          \
        std::fflush(stdout);                                                                                                                                                                           \
        abort();                                                                                                                                                                                       \
    }

#define REQUIRE_TODO(MESSAGE, ...)                                                                                                                                                                     \
    {                                                                                                                                                                                                  \
        std::print("Error: This code path is not written yet: " MESSAGE "\n", ##__VA_ARGS__);                                                                                                          \
        std::print("    The error occurred in function " COLOR_YEL "{}" COLOR_RESET " at " COLOR_CYN "{}" COLOR_RESET ":" COLOR_RED "{}" COLOR_RESET "\n", __FUNCTION__, __FILE__, __LINE__);          \
        std::fflush(stdout);                                                                                                                                                                           \
        abort();                                                                                                                                                                                       \
    }
