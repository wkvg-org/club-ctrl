# SPDX-License-Identifier: Apache-2.0
target_sources(app PRIVATE
	device.c
	moving_head.c
	moving_head_shell.c
	rgbw_par.c
	rgbw_par_shell.c
	dmx_shell.c
)
target_include_directories(app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/..)
