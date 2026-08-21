# SPDX-License-Identifier: Apache-2.0
target_sources(app PRIVATE
	fixture.c
	fixture_shell.c
	dmx_shell.c
)
target_include_directories(app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/..)
