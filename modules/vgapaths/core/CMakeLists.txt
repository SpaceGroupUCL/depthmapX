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

set(vgapathscore vgapathscore)
set(vgapathscore_SRCS
    vgavisualshortestpath.h
    vgavisualshortestpath.cpp
    vgametricshortestpathtomany.h
    vgametricshortestpathtomany.cpp
    vgametricshortestpath.h
    vgametricshortestpath.cpp
    vgametricdepthlinkcost.h
    vgametricdepthlinkcost.cpp
    vgaisovistzone.h
    vgaisovistzone.cpp
    vgaangularshortestpath.h
    vgaangularshortestpath.cpp
    extractlinkdata.h
    extractlinkdata.cpp)

set(modules_core "${modules_core}" "vgapathscore" CACHE INTERNAL "modules_core" FORCE)

add_compile_definitions(VGAPATHS_CORE_LIBRARY)

add_library(${vgapathscore} OBJECT ${vgapathscore_SRCS})
