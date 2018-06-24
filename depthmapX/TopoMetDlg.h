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

#include "ui_TopoMetDlg.h"
#include "salalib/topomet.h"

class CTopoMetDlg : public QDialog, public Ui::CTopoMetDlg
{
	Q_OBJECT
public:
	CTopoMetDlg(QWidget *parent = 0);
	int m_topological;
	QString m_radius;
	double m_dradius;
	bool m_selected_only;
	void UpdateData(bool value);
	void showEvent(QShowEvent * event);

    bool isAnalysisTopological() {
        return m_topological == TOPOMET_METHOD_TOPOLOGICAL;
    }

	private slots:
		void OnOK();
};
