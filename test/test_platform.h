#ifndef TEST_TEST_PLATFORM_H
#define TEST_TEST_PLATFORM_H

#ifdef _WIN32
#include <WinSock2.h>
#include <ws2tcpip.h>
#else
#include <arpa/inet.h>
#endif

#endif
