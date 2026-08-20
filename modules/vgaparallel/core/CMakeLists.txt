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

set(vgaparallelcore vgaparallelcore)
set(vgaparallelcore_SRCS
    vgavisuallocalopenmp.cpp
    vgavisuallocaladjmatrix.cpp
    vgavisualglobalopenmp.cpp
    vgametricopenmp.cpp
    vgaangularopenmp.cpp)

include(CMakeOpenMP.txt)

set(modules_core "${modules_core}" "vgaparallelcore" CACHE INTERNAL "modules_core" FORCE)

add_compile_definitions(VGAPARALLEL_CORE_LIBRARY)

add_library(${vgaparallelcore} OBJECT ${vgaparallelcore_SRCS})

find_package(OpenMP REQUIRED)

target_link_libraries(${vgaparallelcore} OpenMP::OpenMP_CXX)
