# Copyright (C) 2020 Petros Koutsolampros

# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.

# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.

# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.

set(vgaparallelgui vgaparallelgui)

set(CMAKE_AUTOMOC ON)
# AUTOUIC is required to trigger the equivalent function in depthmapX
# the .ui files in this target are however generated using qt5_wrap_ui
set(CMAKE_AUTOUIC_SEARCH_PATHS UI ../../../depthmapX/UI)
set(CMAKE_AUTOUIC ON)
find_package(Qt5 COMPONENTS Core Widgets Gui OpenGL REQUIRED)

set(vgaparallelgui_SRCS
    uictrigger.cpp
    vgaparallelmainwindow.cpp
)

set(modules_gui "${modules_gui}" "vgaparallelgui" CACHE INTERNAL "modules_gui" FORCE)


include_directories(${CMAKE_CURRENT_SOURCE_DIR})
include_directories(${CMAKE_CURRENT_BINARY_DIR})
include_directories(${Qt5Core_INCLUDE_DIRS})
include_directories(${Qt5Widgets_INCLUDE_DIRS})
include_directories(${Qt5Gui_INCLUDE_DIRS})

add_definitions(${Qt5Core_DEFINITIONS})
add_definitions(${Qt5Widgets_DEFINITIONS})
add_definitions(${Qt5Gui_DEFINITIONS})

add_compile_definitions(VGAPARALLEL_GUI_LIBRARY)

add_library(${vgaparallelgui} OBJECT ${vgaparallelgui_SRCS} ${vgaparallelgui_UIS})
