// Copyright (C) 2017 Petros Koutsolampros

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

#include "generalpage.h"
#include <QtWidgets>
#include "depthmapX/settings.h"

GeneralPage::GeneralPage(Settings &settings, QWidget *parent)
    : SettingsPage(settings, parent)
{
    readSettings(settings);
    QGroupBox *configGroup = new QGroupBox(tr("General configuration"));
    QCheckBox *simpleModeCheckBox = new QCheckBox(tr("Simple mode"));
    simpleModeCheckBox->setToolTip(tr("If enabled, only Integration [HH] will be calulcated (or Visual Integration [HH] for VGA)"));
    simpleModeCheckBox->setChecked(m_simpleVersion);
    connect(simpleModeCheckBox, &QCheckBox::stateChanged, [=] () {m_simpleVersion = !m_simpleVersion;});

    QVBoxLayout *configLayout = new QVBoxLayout;
    configLayout->addWidget(simpleModeCheckBox);
    configGroup->setLayout(configLayout);

    QVBoxLayout *mainLayout = new QVBoxLayout;
    mainLayout->addWidget(configGroup);
    mainLayout->addStretch(1);
    setLayout(mainLayout);
}
