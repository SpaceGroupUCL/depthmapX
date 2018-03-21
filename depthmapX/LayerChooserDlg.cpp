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

#include "LayerChooserDlg.h"

CLayerChooserDlg::CLayerChooserDlg(const std::vector<std::string>& names, QWidget *parent)
: QDialog(parent),  m_names(names)
{
	setupUi(this);
	m_text = tr("");
	m_layer = 0;
}

void CLayerChooserDlg::OnOK()
{
	UpdateData(true);
	accept();
}

void CLayerChooserDlg::OnCancel()
{
	reject();
}

void CLayerChooserDlg::UpdateData(bool value)
{
	if (value)
	{
		m_text = c_text->text();
		m_layer = c_layer_selector->currentIndex();
	}
	else
	{
		c_text->setText(m_text);
		c_layer_selector->setCurrentIndex(m_layer);
	}
}

void CLayerChooserDlg::showEvent(QShowEvent * event)
{
	for (size_t i = 0; i < m_names.size(); i++) {
		c_layer_selector->addItem(QString(m_names[i].c_str()));
	}
	c_layer_selector->setCurrentIndex(0);

	UpdateData(false);
}
