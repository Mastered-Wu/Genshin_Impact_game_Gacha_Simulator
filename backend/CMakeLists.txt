cmake_minimum_required(VERSION 3.16)
project(GachaSimulator LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_library(gacha_core
    src/WishEngine.cpp
)
target_include_directories(gacha_core PUBLIC include)

if (MINGW)
    target_link_options(gacha_core INTERFACE -static -static-libgcc -static-libstdc++)
endif()

add_executable(gacha_core_tests tests/WishEngineTests.cpp)
target_link_libraries(gacha_core_tests PRIVATE gacha_core)

add_executable(api_smoke_tests tests/ApiSmokeTests.cpp src/BannerRepository.cpp src/Json.cpp src/main.cpp)
target_compile_definitions(api_smoke_tests PRIVATE GACHA_TESTING=1)
target_link_libraries(api_smoke_tests PRIVATE gacha_core)

add_executable(gacha_server src/BannerRepository.cpp src/Json.cpp src/main.cpp)
target_link_libraries(gacha_server PRIVATE gacha_core)

if (WIN32)
    target_link_libraries(api_smoke_tests PRIVATE ws2_32)
    target_link_libraries(gacha_server PRIVATE ws2_32)
endif()
