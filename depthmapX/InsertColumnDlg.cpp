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

#include "InsertColumnDlg.h"
#include "mainwindow.h"

CInsertColumnDlg::CInsertColumnDlg(AttributeTable *table, int col, QWidget *parent)
: QDialog(parent)
{
	setupUi(this);
	m_selection_only = false;
	m_col = col;

	m_col_names.push_back("Ref Number");
	for (int i = 0; i < table->getColumnCount(); i++) {
		m_col_names.push_back(table->getColumnName(i));
	}
	if (m_col == -1) {
		foreach (QWidget *widget, QApplication::topLevelWidgets()) {
			MainWindow *mainWin = qobject_cast<MainWindow *>(widget);
			if (mainWin)
			{
                m_formula_text = mainWin->m_formula_cache.toStdString();
				break;
			}
		}
	}
	else {
		m_formula_text = table->getColumnFormula(m_col);
	}
	c_use_column->setEnabled(true);
}

void CInsertColumnDlg::OnUseAttribute()
{
	// take the currently selected list item and use in formula	

	QString string;
	string = c_column_names->item(c_column_names->currentRow())->text();
	string = tr("value(\"") + string + tr("\")");
	c_formula->insertPlainText(string);
	c_formula->setFocus();
}

void CInsertColumnDlg::OnSelChangeColumnNames()
{
   c_use_column->setEnabled(true);
}

void CInsertColumnDlg::OnDblclkColumnNames(QListWidgetItem * item)
{
	QString string;
	string = c_column_names->item(c_column_names->currentRow())->text();
	string = tr("value(\"") + string + tr("\")");
	c_formula->insertPlainText(string);
	c_formula->setFocus();
}

void CInsertColumnDlg::OnOK()
{
	UpdateData(true);
	QString text;
	text = c_formula->toPlainText();
	foreach (QWidget *widget, QApplication::topLevelWidgets()) {
		MainWindow *mainWin = qobject_cast<MainWindow *>(widget);
		if (mainWin)
		{
			mainWin->m_formula_cache = text;
			break;
		}
	}

    m_formula_text = text.toStdString();

	// note formula text for column will have to be updated by calling function
	accept();
}

void CInsertColumnDlg::OnCancel()
{
	reject();
}

void CInsertColumnDlg::UpdateData(bool value)
{
	if (value)
	{
		if (c_selection_desc->checkState())
			m_selection_only = true;
		else
			m_selection_only = false;
	}
	else
	{
		if (m_selection_only)
			c_selection_desc->setCheckState(Qt::Checked);
		else
			c_selection_desc->setCheckState(Qt::Unchecked);
	}
}

void CInsertColumnDlg::showEvent(QShowEvent * event)
{
	for (size_t i = 0; i < m_col_names.size(); i++) {
		c_column_names->addItem(QString(m_col_names[i].c_str()));
	}
	c_formula->setPlainText(QString(m_formula_text.c_str()));
	c_column_names->setCurrentRow(-1);
	//c_use_column->setEnabled(false);

	if (m_col == -1) {
		// use for selection query
		// override title and names:
		setWindowTitle(tr("Make selection"));

		c_formula_desc->setText(tr("Query"));
		c_selection_desc->setText(tr("Apply query to selected objects only"));
	}
	else {
		// it's important for the user to know the column name:
		// (note our column names lookup has "Ref Number" in the zero position, so add one:
		setWindowTitle(QString("Replace values for ") + QString(m_col_names[m_col+1].c_str()));

		c_formula_desc->setText(tr("Formula"));
		c_selection_desc->setText(tr("Apply formula to selected objects only"));
	}
	UpdateData(false);
}
