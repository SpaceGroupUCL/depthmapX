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

set(vgacore vgacore)
set(vgacore_SRCS
    vgaangular.cpp
    vgaangulardepth.cpp
    vgaisovist.cpp
    vgametric.cpp
    vgametricdepth.cpp
    vgathroughvision.cpp
    vgavisualglobal.cpp
    vgavisualglobaldepth.cpp
    vgavisuallocal.cpp)

set(modules_core "${modules_core}" "vgacore" CACHE INTERNAL "modules_core" FORCE)

add_compile_definitions(VGA_CORE_LIBRARY)

add_library(${vgacore} OBJECT ${vgacore_SRCS})
