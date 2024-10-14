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

#include "filemerge_dialog.h"
#include "ui_filemerge_dialog.h"

#include <qfile.h>

FileMergeDialog::FileMergeDialog(QWidget *parent) : QDialog(parent), ui(new Ui::FileMergeDialog) {
    ui->setupUi(this);
}

FileMergeDialog::~FileMergeDialog() {
    delete ui;
}

void FileMergeDialog::populateLabel(QLabel *label, QString path) {
    if (QFile::exists(path)) {
        if (path.endsWith(".png") || path.endsWith(".bmp")) {
            ui->mainLabel->setPixmap(QPixmap(path));
        } else {
            // TODO: add sound handling
            ui->mainLabel->setText(path);
        }
    } else {
        ui->mainLabel->setText("(not present)");
    }
}

void FileMergeDialog::populateLabels(QString main, QString patch) {
    populateLabel(ui->mainLabel, main);
    populateLabel(ui->patchLabel, patch);
}
