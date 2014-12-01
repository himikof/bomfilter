cmake_minimum_required(VERSION 2.8.12 FATAL_ERROR)
project(bomfilter CXX)

option(BOMFILTER_ENABLE_TESTS ON)

if (CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    set(CMAKE_CXX_FLAGS "--std=c++11 -static-libstdc++ -static-libgcc -fno-exceptions -fno-rtti -Wall -Wextra")
    add_definitions(-D__GXX_EXPERIMENTAL_CXX0X__)
endif()

add_executable(bomfilter
    src/bomfilter.cpp
)

set_target_properties(bomfilter
    PROPERTIES INTERPROCEDURAL_OPTIMIZATION ON
)

if (CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(bomfilter PRIVATE
        $<$<OR:$<CONFIG:RelWithDebInfo>,$<CONFIG:Release>>:-flto>
    )
    set_target_properties(bomfilter PROPERTIES 
        LINK_FLAGS_RELEASE -flto -fuse-linker-plugin
        LINK_FLAGS_RELWITHDEBINFO -flto -fuse-linker-plugin
    )
endif()

if (BOMFILTER_ENABLE_TESTS)
    enable_testing()
    find_package(GTest REQUIRED)
    include_directories(${GTEST_INCLUDE_DIR})

    #add_executable(cbor-tests
    #    src/cbor-tests.cxx
    #    ${CBORCXX_PUBLIC_HEADERS}
    #)

    #target_link_libraries(cbor-tests
    #    ${GTEST_LIBRARIES}
    #)
    #
    #add_test(NAME cbor-unit-tests COMMAND cbor-tests)
endif()
