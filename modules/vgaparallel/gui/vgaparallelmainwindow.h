// Copyright (C) 2020 Petros Koutsolampros

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

#pragma once

#include "depthmapX/imainwindowmodule.h"

class VGAParallelMainWindow : public IMainWindowModule {
  private:
    enum class AnalysisType {
        NONE,
        VISUAL_LOCAL_OPENMP,
        VISUAL_LOCAL_ADJMATRIX,
        VISUAL_GLOBAL_OPENMP,
        METRIC_OPENMP,
        ANGULAR_OPENMP
    };
    double ConvertForVisibility(const std::string &radius) const;
    double ConvertForMetric(const std::string &radius) const;

  private slots:
    void OnVGAParallel(MainWindow *mainWindow, AnalysisType analysisType);

  public:
    VGAParallelMainWindow() : IMainWindowModule() {}
    bool createMenus(MainWindow *mainWindow);
};
