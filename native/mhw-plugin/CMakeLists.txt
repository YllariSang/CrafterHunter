cmake_minimum_required(VERSION 3.21)
project(crafterhunter_mhw VERSION 0.1.0 LANGUAGES CXX)

if(NOT WIN32)
    message(FATAL_ERROR "The MHW endpoint targets 64-bit Windows only")
endif()

add_library(crafterhunter_mhw SHARED
    src/plugin.cpp
)

target_compile_features(crafterhunter_mhw PRIVATE cxx_std_20)
target_compile_definitions(crafterhunter_mhw PRIVATE
    WIN32_LEAN_AND_MEAN
    NOMINMAX
    UNICODE
    _UNICODE
)
target_link_libraries(crafterhunter_mhw PRIVATE ws2_32)

if(MSVC)
    target_compile_options(crafterhunter_mhw PRIVATE /W4 /permissive- /EHsc)
else()
    target_compile_options(crafterhunter_mhw PRIVATE -Wall -Wextra -Wpedantic)
endif()

if(MINGW)
    target_link_options(crafterhunter_mhw PRIVATE -static-libgcc -static-libstdc++)
endif()

set_target_properties(crafterhunter_mhw PROPERTIES
    OUTPUT_NAME "CrafterHunter.MHW"
    PREFIX ""
)
