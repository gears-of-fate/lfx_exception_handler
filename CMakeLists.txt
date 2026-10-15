cmake_minimum_required(VERSION 3.24)
project(lfx_exception_handler)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(LFX_EH_BUILD_TESTS "Build the unit tests" ON)
option(LFX_EH_BUILD_SANDBOX "Build the sandbox example" ON)

set(CMAKE_POLICY_VERSION_MINIMUM 3.10)

# --- LIBRERIA PRINCIPALE (STATIC) ---
add_library(lfx_exception_handler STATIC
        include/lfx_exception_handler/core/eh_config.h
        include/lfx_exception_handler/payloads/eh_payloads.h
        include/lfx_exception_handler/core/eh_handler.h
        include/lfx_exception_handler/LfxExceptionHandler.h
        src/core/eh_clock.h
        src/core/eh_dispatcher.h
        src/core/eh_action_excecutor.h
        src/core/eh_handler.cpp
)

target_sources(lfx_exception_handler PRIVATE
        src/core/eh_handler.cpp
)

target_include_directories(lfx_exception_handler
        PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
        PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

if (ANDROID)
    find_library(log-lib log)
    target_link_libraries(lfx_exception_handler PRIVATE ${log-lib})
endif()

# --- UNIT TESTS (GoogleTest) ---
# --- UNIT TESTS (GoogleTest) ---
if(LFX_EH_BUILD_TESTS)
    include(FetchContent)
    FetchContent_Declare(
            googletest
            URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.zip
    )
    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(googletest)

    enable_testing()

    add_executable(lfx_exception_handler_tests
            src/tests/test_handler.cpp
    )

    target_include_directories(lfx_exception_handler_tests PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}/src
            ${googletest_SOURCE_DIR}/googletest/include
    )

    target_link_libraries(lfx_exception_handler_tests
            PRIVATE
            lfx_exception_handler
            gtest_main
    )

    include(GoogleTest)
    gtest_discover_tests(lfx_exception_handler_tests)
endif()

# --- SANDBOX ---
if(LFX_EH_BUILD_SANDBOX)
    add_executable(lfx_exception_handler_sandbox example/main.cpp)
    target_include_directories(lfx_exception_handler_sandbox PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)
    target_link_libraries(lfx_exception_handler_sandbox PRIVATE lfx_exception_handler)
endif()

# --- INSTALLAZIONE & PACKAGING (CPack) ---
include(GNUInstallDirs)

install(TARGETS lfx_exception_handler
        EXPORT lfx_exception_handler-targets
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

install(DIRECTORY include/lfx_exception_handler
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

install(EXPORT lfx_exception_handler-targets
        FILE lfx_exception_handler-targets.cmake
        NAMESPACE lfx_exception_handler::
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/lfx_exception_handler
)

include(CMakePackageConfigHelpers)
configure_package_config_file(
        "cmake/lfx_exception_handler-config.cmake.in"
        "${CMAKE_CURRENT_BINARY_DIR}/lfx_exception_handler-config.cmake"
        INSTALL_DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/lfx_exception_handler"
)

install(FILES
        "${CMAKE_CURRENT_BINARY_DIR}/lfx_exception_handler-config.cmake"
        DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/lfx_exception_handler"
)

set(CPACK_GENERATOR "ZIP")

if (ANDROID)
    set(CPACK_PACKAGE_FILE_NAME "${PROJECT_NAME}-android-arm64")
elseif (WIN32)
    set(CPACK_PACKAGE_FILE_NAME "${PROJECT_NAME}-windows-x64")
endif()

include(CPack)