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

#include "ui_InsertColumnDlg.h"
#include <salalib/mgraph.h>
#include <salalib/attributes.h>
#include <salalib/shapemap.h>
#include <salalib/axialmap.h>
#include <string>
#include <vector>

class CInsertColumnDlg : public QDialog, public Ui::CInsertColumnDlg
{
	Q_OBJECT
public:
	CInsertColumnDlg(AttributeTable *table = NULL, int col = -1, QWidget *parent = 0);
	bool	m_selection_only;
	int m_col;
    std::vector<std::string> m_col_names;
    std::string m_formula_text;
	void UpdateData(bool value);
	void showEvent(QShowEvent * event);

private slots:
		void OnUseAttribute();
		void OnSelChangeColumnNames();
		void OnDblclkColumnNames(QListWidgetItem * item);
		void OnOK();
		void OnCancel();
};
