// depthmapX - spatial network analysis platform
// Copyright (C) 2017 Christian Sailer

// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.

// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.

#ifndef VERSION_H
#define VERSION_H

#include "version_defs.h"

// use these to define the depthmap versions
#define DEPTHMAPX_MAJOR_VERSION 0
#define DEPTHMAPX_MINOR_VERSION 6
#define DEPTHMAPX_REVISION_VERSION 0

#define DEPTHMAP_MODULE_VERSION 10.04

// leave these alone - C Preprocessor magic to get stuff to the right format
#define STRINGIFY(v) #v
#define TITLE_BASE_FORMAT(version, minor, revision) "depthmapX " STRINGIFY(version) "." STRINGIFY(minor) "." STRINGIFY(revision)
#define TITLE_BASE TITLE_BASE_FORMAT(DEPTHMAPX_MAJOR_VERSION, DEPTHMAPX_MINOR_VERSION, DEPTHMAPX_REVISION_VERSION)

#endif // VERSION_H
