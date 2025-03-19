cmake_minimum_required(VERSION 3.28 FATAL_ERROR)

#CMake
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
add_compile_options(
    -Werror
    -Wall
)
project(wordle-server-game)

# Protobuf
find_package(Protobuf REQUIRED)
if (NOT Protobuf_FOUND)
    message(FATAL_ERROR "Protobuf lib not found!")
endif()
if(Protobuf_VERBOSE)
  message(STATUS "Using Protocol Buffers ${protobuf_VERSION}")
endif()

set(PROTOS_NAMES_
    uuid.proto
    enum_the_char_color.proto
    enum_row_result.proto
    request_row_body.proto
    request_new_game_body.proto
    response_row_body.proto
    response_new_game_body.proto
)

set(PROTO_FILES_PATH "${CMAKE_CURRENT_SOURCE_DIR}/wordle-data/proto/")
set(PROTOS_PATHS_ ${PROTOS_NAMES_})
list(TRANSFORM PROTOS_PATHS_ PREPEND ${PROTO_FILES_PATH})

set(PROTO_GENERATED_PATH "${CMAKE_CURRENT_BINARY_DIR}/src/generated/")
file(MAKE_DIRECTORY ${PROTO_GENERATED_PATH})

add_library(${CMAKE_PROJECT_NAME}-generated STATIC ${PROTOS_PATHS_})
target_link_libraries(${CMAKE_PROJECT_NAME}-generated PUBLIC protobuf::libprotobuf)
protobuf_generate (
    TARGET ${CMAKE_PROJECT_NAME}-generated
    LANGUAGE cpp
    PROTOC_OUT_DIR  ${PROTO_GENERATED_PATH}
    OUT_VAR GENERATED_H_CPP_FILES
    PROTOS ${PROTOS_PATHS_}
    IMPORT_DIRS ${PROTO_FILES_PATH}
)
# Protobuf

find_package(userver REQUIRED COMPONENTS core postgresql)
# Common sources
set(SOURCE_FILES__ 
    http_handler_new_game.cpp http_handler_new_game.hpp
    http_handler_row.cpp http_handler_row.hpp
    )
list(TRANSFORM SOURCE_FILES__ PREPEND "src/")
add_library(${CMAKE_PROJECT_NAME}_objs OBJECT ${SOURCE_FILES__})
target_link_libraries(${CMAKE_PROJECT_NAME}_objs PUBLIC
                      userver::core
                      userver::postgresql
                      ${CMAKE_PROJECT_NAME}-generated
                      )
target_include_directories(${CMAKE_PROJECT_NAME}_objs PUBLIC
                           ${CMAKE_CURRENT_SOURCE_DIR}/src 
                           ${PROTO_GENERATED_PATH})
# Common sources

# The Service
add_executable(${CMAKE_PROJECT_NAME} src/main.cpp)
target_compile_features(${CMAKE_PROJECT_NAME} PUBLIC cxx_std_20)
target_include_directories(${CMAKE_PROJECT_NAME} PUBLIC 
                           ${CMAKE_CURRENT_SOURCE_DIR}/src 
                           ${PROTO_GENERATED_PATH})
target_link_libraries(${PROJECT_NAME} ${PROJECT_NAME}_objs)

# Unit Tests
#add_executable(${CMAKE_PROJECT_NAME}_unittest
#    src/game_api_test.cpp
#)
#target_link_libraries(${CMAKE_PROJECT_NAME}_unittest PRIVATE ${CMAKE_PROJECT_NAME}_objs userver::utest)
#add_google_tests(${CMAKE_PROJECT_NAME}_unittest)

# Benchmarks
#add_executable(${CMAKE_PROJECT_NAME}_benchmark
#    src/hello_benchmark.cpp
#)
#target_link_libraries(${CMAKE_PROJECT_NAME}_benchmark PRIVATE ${CMAKE_PROJECT_NAME}_objs userver::ubench)
#add_google_benchmark_tests(${CMAKE_PROJECT_NAME}_benchmark)


#CPack
set(CMAKE_INSTALL_PREFIX /opt/${CMAKE_PROJECT_NAME})

install(PROGRAMS ${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_PROJECT_NAME}
        DESTINATION bin
        PERMISSIONS WORLD_EXECUTE OWNER_WRITE OWNER_READ)

install(FILES configs/config_vars.yaml DESTINATION /etc/${CMAKE_PROJECT_NAME}/)
install(FILES configs/config_vars.testing.yaml DESTINATION /etc/${CMAKE_PROJECT_NAME}/)
install(FILES configs/static_config.yaml DESTINATION /etc/${CMAKE_PROJECT_NAME}/)

execute_process(COMMAND mkdir ${CMAKE_CURRENT_BINARY_DIR}/symlink)
execute_process(COMMAND ln -s /opt/${CMAKE_PROJECT_NAME}/bin/${CMAKE_PROJECT_NAME} ${CMAKE_CURRENT_BINARY_DIR}/symlink/${CMAKE_PROJECT_NAME})
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/symlink/wordle-server-game
        DESTINATION /usr/bin/
        PERMISSIONS WORLD_EXECUTE WORLD_READ)

set(CPACK_GENERATOR "DEB")
set(CPACK_DEBIAN_PACKAGE_NAME ${CMAKE_PROJECT_NAME})
set(CPACK_PACKAGING_INSTALL_PREFIX ${CMAKE_INSTALL_PREFIX})
set(CPACK_PACKAGE_CONTACT "chicherin.alexey@ya.ru")
set(CPACK_DEBIAN_PACKAGE_HOMEPAGE "https://github.com/alec-chicherini/wordle-server-game")
set(CPACK_DEBIAN_PACKAGE_ARCHITECTURE amd64)
set(CPACK_DEBIAN_PACKAGE_VERSION 25.03)
set(CPACK_DEBIAN_FILE_NAME ${CPACK_DEBIAN_PACKAGE_NAME}_${CPACK_DEBIAN_PACKAGE_VERSION}_${CPACK_DEBIAN_PACKAGE_ARCHITECTURE}.deb)

#https://raw.githubusercontent.com/userver-framework/userver/refs/heads/develop/scripts/docs/en/deps/ubuntu-24.04.md
#add deps

include(CPack)