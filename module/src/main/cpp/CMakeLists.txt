cmake_minimum_required(VERSION 3.22.1)
project(sample)

# Ensure MODULE_NAME is defined
if(NOT DEFINED MODULE_NAME)
    set(MODULE_NAME ${PROJECT_NAME})
endif()

# Define compile flags
set(CXX_FLAGS
    -fno-exceptions
    -fno-rtti
    -fvisibility=hidden
    -fvisibility-inlines-hidden
    -flto=auto
    -pipe
    -O3
    -std=c++20
    -D_LIBCPP_HAS_NO_LOCALIZATION
    -D_LIBCPP_NO_EXCEPTIONS
    -D_LIBCPP_NO_RTTI
    -D_LIBCPP_BUILDING_LIBRARY
)

# Define linker flags
set(LINKER_FLAGS
    -ffixed-x18
    -Wl,--hash-style=both
)

# Combine flags
string(REPLACE ";" " " CXX_FLAGS "${CXX_FLAGS}")
string(REPLACE ";" " " LINKER_FLAGS "${LINKER_FLAGS}")

# Set flags
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${CXX_FLAGS}")
set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} ${LINKER_FLAGS}")
set(CMAKE_MODULE_LINKER_FLAGS "${CMAKE_MODULE_LINKER_FLAGS} ${LINKER_FLAGS}")

# Add external subdirectory first to ensure dependencies are built
add_subdirectory(external)

# Create the shared library
add_library(${MODULE_NAME} SHARED main.cpp)

# Set include directories
target_include_directories(${MODULE_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/external/rapidjson/include
    ${CMAKE_CURRENT_SOURCE_DIR}/external/libcxx/include
)

# Link against required libraries
target_link_libraries(${MODULE_NAME} PRIVATE
    cxx
    log
)

# Set additional compile definitions
target_compile_definitions(${MODULE_NAME} PRIVATE
    ANDROID
    _FORTIFY_SOURCE=2
)
