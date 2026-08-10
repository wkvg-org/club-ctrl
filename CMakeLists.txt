# Copyright (C) 2025 - 2026 Wolfenbütteler Kultur- und Veranstaltungsgesellschaft gemeinnützige UG (haftungsbeschränkt)
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)

# Circumvent Windows commandline length exceeded
SET(CMAKE_C_USE_RESPONSE_FILE_FOR_OBJECTS 1)
SET(CMAKE_CXX_USE_RESPONSE_FILE_FOR_OBJECTS 1)

SET(CMAKE_C_RESPONSE_FILE_LINK_FLAG "@")
SET(CMAKE_CXX_RESPONSE_FILE_LINK_FLAG "@")

SET(CMAKE_NINJA_FORCE_RESPONSE_FILE 1 CACHE INTERNAL "")

# Zephyr cmake commands

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(blinky)

target_sources(app PRIVATE src/main.c)
target_sources(app PRIVATE src/modules/config/config.c)
target_sources(app PRIVATE src/modules/net/mqtt_subscriber.c)
target_sources(app PRIVATE src/modules/net/udp_server.c)

add_subdirectory(src/modules/output/dmx)
add_subdirectory(src/modules/output/sound)
add_subdirectory(src/modules/show)
