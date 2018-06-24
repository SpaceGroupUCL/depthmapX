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

#include "TopoMetDlg.h"
#include <QMessageBox>

CTopoMetDlg::CTopoMetDlg(QWidget *parent)
: QDialog(parent)
{
	setupUi(this);
    m_topological = TOPOMET_METHOD_TOPOLOGICAL;
	m_selected_only = false;
	m_radius = tr("n");

	UpdateData(false);
}

void CTopoMetDlg::OnOK()
{
	UpdateData(true);

	/*m_radius.TrimLeft(' ');
	m_radius.TrimRight(' ');*/

	// my own validate on the radius (note: on fail to convert, atoi returns 0)
	if (m_radius.isEmpty() || (m_radius.indexOf("n") == -1 && m_radius.indexOf("N") == -1 &&
		m_radius.indexOf("1") == -1 && m_radius.indexOf("2") == -1 && m_radius.indexOf("3") == -1 && 
		m_radius.indexOf("4") == -1 &&  m_radius.indexOf("5") == -1 &&  m_radius.indexOf("6") == -1 &&
		m_radius.indexOf("7") == -1 &&  m_radius.indexOf("8") == -1 &&  m_radius.indexOf("9") == -1)) {
		QMessageBox::warning(this, tr("Warning"), tr("The radius must either be numeric or 'n'"), QMessageBox::Ok, QMessageBox::Ok);
		m_radius = tr("n");
		UpdateData(false);
		return;
	}

	if (m_radius == "n" || m_radius == "N") {
		m_dradius = -1.0;
	}
	else {
		m_dradius = m_radius.toDouble();
		if (m_dradius <= 0.0) {
			QMessageBox::warning(this, tr("Warning"), tr("The radius must either be 'n' or a number in the range 0.0 to infinity"), QMessageBox::Ok, QMessageBox::Ok);
			return;
		}
	}

	accept();
}

void CTopoMetDlg::UpdateData(bool value)
{
	if (value)
	{
		if (c_topological->isChecked())
            m_topological = TOPOMET_METHOD_TOPOLOGICAL;
		else if (radioButton->isChecked())
            m_topological = TOPOMET_METHOD_METRIC;
		else
			m_topological = -1;
		m_radius = c_radius->text();
		if (checkBox->checkState())
			m_selected_only = true;
		else
			m_selected_only = false;
	}
	else
	{
		switch(m_topological)
		{
        case TOPOMET_METHOD_TOPOLOGICAL:
			c_topological->setChecked(true);
			break;
        case TOPOMET_METHOD_METRIC:
			radioButton->setChecked(true);
			break;
		default:
			break;
		}
		c_radius->setText(m_radius);
		if (m_selected_only)
			checkBox->setCheckState(Qt::Checked);
		else
			checkBox->setCheckState(Qt::Unchecked);
	}
}

void CTopoMetDlg::showEvent(QShowEvent * event)
{
	UpdateData(false);
}
