cmake_minimum_required(VERSION 3.16)
project(tgclic)

set(CMAKE_CXX_STANDARD 14)

find_package(Td 1.6.4 REQUIRED)

add_executable(tgclic main.cpp application.h utils.h respond_processor.h tg.h info_pool.h request_processor.h)
target_link_libraries(tgclic PRIVATE Td::TdStatic)
set_property(TARGET tgclic PROPERTY CXX_STANDARD 14)