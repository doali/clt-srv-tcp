cmake_minimum_required(VERSION 3.23)
project(netlib VERSION 0.1.0 LANGUAGES CXX)

# Génère compile_commands.json dans le répertoire de build
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

option(NETLIB_BUILD_EXAMPLES "Build examples" ON)
option(NETLIB_WARNINGS_AS_ERRORS "Treat warnings as errors" ON)

add_library(netlib
  src/framing.cpp
  src/session.cpp
  src/server.cpp
  src/client.cpp
)

target_include_directories(netlib PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:include>
)

target_compile_features(netlib PUBLIC cxx_std_20)

find_package(Boost REQUIRED COMPONENTS system)
target_link_libraries(netlib PUBLIC Boost::system)

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)
target_link_libraries(netlib PUBLIC Threads::Threads)

if (NETLIB_WARNINGS_AS_ERRORS)
  if (MSVC)
    target_compile_options(netlib PRIVATE /W4 /WX)
  else()
    target_compile_options(netlib PRIVATE -Wall -Wextra -Wpedantic -Werror)
  endif()
else()
  if (MSVC)
    target_compile_options(netlib PRIVATE /W4)
  else()
    target_compile_options(netlib PRIVATE -Wall -Wextra -Wpedantic)
  endif()
endif()

if (NETLIB_BUILD_EXAMPLES)
  add_executable(netlib_echo_server examples/echo_server.cpp)
  target_link_libraries(netlib_echo_server PRIVATE netlib)

  add_executable(netlib_echo_client examples/echo_client.cpp)
  target_link_libraries(netlib_echo_client PRIVATE netlib)
endif()

# Copie compile_commands.json dans la racine du projet (pour clangd, Emacs, etc.)
add_custom_target(compile_commands
  COMMAND ${CMAKE_COMMAND} -E copy_if_different
          ${CMAKE_BINARY_DIR}/compile_commands.json
          ${CMAKE_SOURCE_DIR}/compile_commands.json
  DEPENDS ${CMAKE_BINARY_DIR}/compile_commands.json
  COMMENT "Copy compile_commands.json to project root"
)

find_program(CLANG_FORMAT clang-format)

if (CLANG_FORMAT)
  file(GLOB_RECURSE NETLIB_SOURCES
    ${CMAKE_SOURCE_DIR}/src/*.cpp
    ${CMAKE_SOURCE_DIR}/include/*.hpp
  )

  add_custom_target(format
    COMMAND ${CLANG_FORMAT} -i ${NETLIB_SOURCES}
    COMMENT "Running clang-format on sources"
  )
endif()

find_program(CLANG_TIDY clang-tidy)

if (CLANG_TIDY)
  add_custom_target(tidy
    COMMAND ${CLANG_TIDY}
            -p ${CMAKE_BINARY_DIR}
            ${NETLIB_SOURCES}
    COMMENT "Running clang-tidy on sources"
  )
endif()
