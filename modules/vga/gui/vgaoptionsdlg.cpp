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

#include "vgaoptionsdlg.h"
#include <QMessageBox>

VGAOptionsDlg::VGAOptionsDlg(QWidget *parent, Options existingOptions) : QDialog(parent) {
    setupUi(this);
    m_global = false;
    m_local = false;
    m_radius = tr("");
    m_gates_only = false;
    m_output_type = -1;
    m_radius2 = tr("");

    m_output_type = existingOptions.output_type;

    m_local = existingOptions.local;
    m_global = existingOptions.global;
    m_gates_only = existingOptions.gates_only;
    m_gatelayer = existingOptions.gatelayer;

    if ((int)existingOptions.radius == -1) {
        m_radius = QString("n");
        m_radius2 = QString("n");
    } else if (m_output_type == Options::OUTPUT_VISUAL) {
        char number[2];
        sprintf(number, "%d", (int)existingOptions.radius);
        m_radius = QString(number);
        m_radius2 = tr("n");
    } else {
        char number[32];
        sprintf(number, "%g", existingOptions.radius);
        m_radius = tr("n");
        m_radius2 = QString(number);
    }
}

void VGAOptionsDlg::OnOutputType(bool value) {
    UpdateData(true);

    if (m_output_type == Options::OUTPUT_VISUAL) {
        c_local->setEnabled(true);
        c_global->setEnabled(true);
        c_radius->setEnabled(true);
    } else {
        c_local->setEnabled(false);
        c_global->setEnabled(false);
        c_radius->setEnabled(false);
        c_radius->setText(tr("n")); // <- essentially, undo changes
    }

    if (m_output_type == Options::OUTPUT_METRIC) {
        c_radius2->setEnabled(true);
    } else {
        c_radius2->setText(tr("n")); // <- essentially, undo changes
        c_radius2->setEnabled(false);
    }
}

void VGAOptionsDlg::OnUpdateRadius(QString text) {
    if (text.length()) {
        if (!text.toInt() && text != tr("n")) {
            QMessageBox::warning(this, tr("Warning"), tr("The radius must either be n or number in range 1-99"),
                                 QMessageBox::Ok, QMessageBox::Ok);
            c_radius->setText(tr("n"));
        }
    }
}

void VGAOptionsDlg::OnUpdateRadius2(QString text) {
    if (text.length()) {
        if (text.toDouble() == 0.0 && text != tr("n")) {
            QMessageBox::warning(this, tr("Warning"), tr("The radius must either be n or a positive number"),
                                 QMessageBox::Ok, QMessageBox::Ok);
            c_radius2->setText(tr("n"));
        }
    }
}

void VGAOptionsDlg::OnOK() {
    UpdateData(true);

    m_gatelayer = c_layer_selector->currentIndex() - 1;

    if (m_output_type == Options::OUTPUT_VISUAL) {
        if (m_radius.compare(tr("n")) == 0) { // 0 means identical
            m_numericRadius = -1.0;
        } else {
            m_numericRadius = (double)m_radius.toInt();
            if (m_numericRadius <= 0.0) {
                QMessageBox::warning(this, tr("Warning"),
                                     tr("The radius must either be n or a number in the range 1-99"), QMessageBox::Ok,
                                     QMessageBox::Ok);
                return;
            }
        }
    } else {
        if (m_radius2.compare(tr("n")) == 0) { // 0 means identical
            m_numericRadius = -1.0;
        } else {
            m_numericRadius = m_radius2.toDouble();
            if (m_numericRadius <= 0.0) {
                QMessageBox::warning(this, tr("Warning"), tr("The radius must either be n or a positive number"),
                                     QMessageBox::Ok, QMessageBox::Ok);
                return;
            }
        }
    }
    accept();
}

void VGAOptionsDlg::OnCancel() { reject(); }

void VGAOptionsDlg::UpdateData(bool value) {
    if (value) {
        if (c_global->checkState())
            m_global = true;
        else
            m_global = false;

        if (c_local->checkState())
            m_local = true;
        else
            m_local = false;

        m_radius = c_radius->text();

        if (c_output_type->isChecked())
            m_output_type = 0;
        else if (c_radio1->isChecked())
            m_output_type = 1;
        else if (c_radio2->isChecked())
            m_output_type = 2;
        else if (c_radio3->isChecked())
            m_output_type = 3;
        else if (c_radio4->isChecked())
            m_output_type = 4;
        else
            m_output_type = -1;
        m_radius2 = c_radius2->text();
    } else {
        if (m_global)
            c_global->setCheckState(Qt::Checked);
        else
            c_global->setCheckState(Qt::Unchecked);

        if (m_local)
            c_local->setCheckState(Qt::Checked);
        else
            c_local->setCheckState(Qt::Unchecked);

        c_radius->setText(m_radius);

        switch (m_output_type) {
        case 0:
            c_output_type->setChecked(true);
            break;
        case 1:
            c_radio1->setChecked(true);
            break;
        case 2:
            c_radio2->setChecked(true);
            break;
        case 3:
            c_radio3->setChecked(true);
            break;
        case 4:
            c_radio4->setChecked(true);
            break;
        default:
            break;
        }
        c_radius2->setText(m_radius2);
    }
}

void VGAOptionsDlg::showEvent(QShowEvent *event) {
    for (size_t i = 0; i < m_layer_names.size(); i++) {
        c_layer_selector->addItem(QString(m_layer_names[i].c_str()));
    }
    c_layer_selector->setCurrentIndex(m_gatelayer + 1);

    OnOutputType(false);

    UpdateData(false);
}
