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

#include "PushDialog.h"

CPushDialog::CPushDialog(std::map<IntPair, string> &names, QWidget *parent)
: QDialog(parent)
{
	setupUi(this);
	m_layer_selection = -1;
	m_origin_attribute = tr("");
	m_origin_layer = tr("");
	m_count_intersections = false;
	m_function = -1;

	m_function = 0;

    for (auto name: names) {
        m_names.push_back(name.second);
	}
}

void CPushDialog::OnOK()
{
	UpdateData(true);
	accept();
}

void CPushDialog::OnCancel()
{
	reject();
}

void CPushDialog::UpdateData(bool value)
{
	if (value)
	{
		m_layer_selection = c_layer_selector->currentIndex();
		m_origin_attribute = c_origin_attribute->text();
		m_origin_layer = c_origin_layer->text();
		if (c_count_intersections->checkState())
			m_count_intersections = true;
		else
			m_count_intersections = false;
		m_function = c_function->currentIndex();
	}
	else
	{
		c_layer_selector->setCurrentIndex(m_layer_selection);
		c_origin_attribute->setText(m_origin_attribute);
		c_origin_layer->setText(m_origin_layer);
		if (m_count_intersections)
			c_count_intersections->setCheckState(Qt::Checked);
		else
			c_count_intersections->setCheckState(Qt::Unchecked);
		c_function->setCurrentIndex(m_function);
	}
}

void CPushDialog::showEvent(QShowEvent * event)
{
	for (size_t i = 0; i < m_names.size(); i++) {
		c_layer_selector->addItem(QString(m_names[i].c_str()));
	}
	c_layer_selector->setCurrentIndex(0);

	UpdateData(false);
}
