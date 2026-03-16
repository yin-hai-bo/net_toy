cmake_minimum_required(VERSION 3.16)

project(net_toy VERSION 0.1.0 LANGUAGES CXX)

include(CTest)

add_library(hbutils STATIC
    src/checksum.cpp
    src/route_table.cpp
    src/tb_rate_limiter.cpp
)

target_include_directories(hbutils
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_compile_features(hbutils PUBLIC cxx_std_11)

if (MSVC)
    target_compile_options(hbutils PRIVATE /W4 /WX /utf-8)
else ()
    target_compile_options(hbutils PRIVATE -Wall -Wextra -Werror -Wfatal-errors -O1)
endif ()

if (WIN32)
    target_link_libraries(hbutils PUBLIC ws2_32)
endif ()

if (BUILD_TESTING)
    find_package(GTest REQUIRED)

    add_executable(net_toy_test
        test/test_checksum.cpp
        test/test_route_table.cpp
        test/test_tb_rate_limiter.cpp
    )

    target_compile_features(net_toy_test PRIVATE cxx_std_11)
    target_compile_definitions(net_toy_test PRIVATE GTEST)

    if (MSVC)
        target_compile_options(net_toy_test PRIVATE /W4 /WX /utf-8)
    else ()
        target_compile_options(net_toy_test PRIVATE -Wall -Wextra -Werror -Wfatal-errors -O1)
    endif ()

    target_link_libraries(net_toy_test
        PRIVATE
            hbutils
            GTest::gtest
            GTest::gtest_main
    )

    include(GoogleTest)
    gtest_discover_tests(net_toy_test)
endif ()
