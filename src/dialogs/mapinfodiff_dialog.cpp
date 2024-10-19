/*
 * This file is part of Eidolist.
 *
 * Eidolist is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Eidolist is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Eidolist. If not, see <http://www.gnu.org/licenses/>.
 */

#include "mapinfodiff_dialog.h"
#include "ui_mapinfodiff_dialog.h"

#include <qfile.h>

MapInfoDiffDialog::MapInfoDiffDialog(QWidget *parent) : QDialog(parent), ui(new Ui::MapInfoDiffDialog) {
    ui->setupUi(this);
}

MapInfoDiffDialog::~MapInfoDiffDialog() {
    delete ui;
}

void MapInfoDiffDialog::populateLabel(QLabel *label, lcf::rpg::MapInfo info) {
    label->setText(QString(
            "(placeholder, please check changelog for now)"
        ));
}

void MapInfoDiffDialog::populateLabels(lcf::rpg::MapInfo main, lcf::rpg::MapInfo patch) {
    populateLabel(ui->mainLabel, main);
    populateLabel(ui->patchLabel, patch);
}

void MapInfoDiffDialog::on_mainCopyButton_clicked() {
    this->reject();
}

void MapInfoDiffDialog::on_patchButton_clicked() {
    this->accept();
}

