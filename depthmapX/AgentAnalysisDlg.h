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

#include "ui_AgentAnalysisDlg.h"
#include <salalib/mgraph.h>
#include <salalib/attributes.h>
#include <salalib/shapemap.h>
#include <salalib/axialmap.h>

class CAgentAnalysisDlg : public QDialog, public Ui::CAgentAnalysisDlg
{
	Q_OBJECT
public:
	CAgentAnalysisDlg(QWidget *parent = 0);
	int		m_release_location;
	int		m_fov;
	int		m_frames;
	double	m_release_rate;
	int		m_steps;
	int		m_timesteps;
	int		m_occlusion;
	bool	m_record_trails;
	int		m_trail_count;
	//}}AFX_DATA

	int m_gatelayer;
    std::vector<std::string> m_names;
	void UpdateData(bool value);
	void showEvent(QShowEvent * event);

private slots:
		void OnOK();
		void OnCancel();
};
