// Copyright (C) 2011-2012, Tasos Varoudis

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

#include "AboutDlg.h"
#include "version.h"

CAboutDlg::CAboutDlg(QWidget *parent)
: QDialog(parent)
{
	setupUi(this);
	QString m_version_info;
    m_version_info = QString(tr("Version %1.%2.%3")).arg(DEPTHMAPX_MAJOR_VERSION).arg(DEPTHMAPX_MINOR_VERSION).arg(DEPTHMAPX_REVISION_VERSION);
	QString m_copyright;
    m_copyright = QString(tr("(C) 2000-2010 University College London, Alasdair Turner, Eva Friedrich\n(C) 2011-2014 Tasos Varoudis\n(C) 2017 Christian Sailer, Petros Koutsolampros"));
	QString m_agreement;
    m_agreement = QString(tr("This program is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.\x0D\x0D\x0A\x0D\x0D\x0AThis program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more details.\x0D\x0D\x0A\x0D\x0D\x0AYou should have received a copy of the GNU General Public License along with this program.  If not, see <http://www.gnu.org/licenses/>."));

    c_version_info->setText(m_version_info);
	c_copyright->setText(m_copyright);
	c_agreement->setText(m_agreement);
}

void CAboutDlg::OnOK()
{
	accept();
}
