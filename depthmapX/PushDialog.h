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

#include "ui_PushDialog.h"
#include <salalib/mgraph.h>
#include <salalib/attributes.h>
#include <salalib/shapemap.h>
#include <salalib/axialmap.h>

class CPushDialog : public QDialog, public Ui::CPushDialog
{
	Q_OBJECT
public:
    CPushDialog(std::map<IntPair,std::string>& names, QWidget *parent = 0);
	int		m_layer_selection;
	QString	m_origin_attribute;
	QString	m_origin_layer;
	bool	m_count_intersections;
	int		m_function;
	void UpdateData(bool value);
	void showEvent(QShowEvent * event);

    std::vector<std::string> m_names;

	private slots:
		void OnOK();
		void OnCancel();
};
