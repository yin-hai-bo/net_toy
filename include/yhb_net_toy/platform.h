#ifndef YHB_NET_TOY_PLATFORM_H
#define YHB_NET_TOY_PLATFORM_H

#if !defined(LIKELY) && !defined(UNLIKELY)
#   if defined __GNUC__
#       define LIKELY(x) __builtin_expect (!!(x), 1)
#       define UNLIKELY(x) __builtin_expect (!!(x), 0)
#   else
#       define LIKELY(x) (x)
#       define UNLIKELY(x) (x)
#   endif
#endif

#ifdef GTEST
#   define STATIC_IF_NO_GTEST
#   define FRIEND_GTEST(test_case_name, test_name) friend class test_case_name##_##test_name##_Test
#else
#   define STATIC_IF_NO_GTEST static
#   define FRIEND_GTEST(test_case_name, test_name)
#endif

#ifdef _WIN32
#include <WinSock2.h>
#include <ws2tcpip.h>
#else
#include <arpa/inet.h>
#include <netinet/in.h>
#endif

#endif
