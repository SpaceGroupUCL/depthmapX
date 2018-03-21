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

#include "ui_OptionsDlg.h"
#include <salalib/mgraph.h>
#include <salalib/attributes.h>
#include <salalib/shapemap.h>
#include <salalib/axialmap.h>

class COptionsDlg : public QDialog, public Ui::COptionsDlg
{
	Q_OBJECT
public:
	COptionsDlg(QWidget *parent = 0);
	bool	m_global;
	bool	m_local;
	QString	m_radius;
	bool	m_gates_only;
	int		m_output_type;
	QString	m_radius2;
	void UpdateData(bool value);
	void showEvent(QShowEvent * event);

    std::vector<std::string> m_layer_names;

	private slots:
		void OnOutputType(bool);
		void OnUpdateRadius(QString);
		void OnUpdateRadius2(QString);
		void OnOK();
		void OnCancel();
};
