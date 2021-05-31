// Integration of the OWN3D service into OBS Studio
// Copyright (C) 2021 own3d media GmbH <support@own3d.tv>
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <https://www.gnu.org/licenses/>.

#include "ui-gdpr.hpp"

own3d::ui::gdpr::gdpr(QWidget* parent) : QDialog(parent), Ui::GDPR()
{
	// Initialize UI.
	setupUi(this);

	// Set up default state.
	_decline->setEnabled(true);
	_accept->setEnabled(false);
	_agreement1->setChecked(false);

	// Connect Signals with Slots.
	connect(_decline, &QPushButton::pressed, this, &gdpr::on_decline);
	connect(_accept, &QPushButton::pressed, this, &gdpr::on_accept);
	connect(_agreement1, &QCheckBox::stateChanged, this, &gdpr::on_agreement1_checked);
}

own3d::ui::gdpr::~gdpr() {}

void own3d::ui::gdpr::on_agreement1_checked(int checked)
{
	blockSignals(true);
	if (checked == Qt::CheckState::Checked) {
		_accept->setEnabled(true);
	} else {
		_accept->setEnabled(false);
	}
	blockSignals(false);
}

void own3d::ui::gdpr::on_accept()
{
	blockSignals(true);
	hide();
	blockSignals(false);
	emit accepted();
}

void own3d::ui::gdpr::on_decline()
{
	blockSignals(true);
	hide();
	blockSignals(false);
	emit declined();
}

void own3d::ui::gdpr::showEvent(QShowEvent* event)
{
	setModal(true);
}

void own3d::ui::gdpr::closeEvent(QCloseEvent* event)
{
	setModal(false);
}
