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

#include "directorydialog.h"
#include "ui_directorydialog.h"

#include <QFileDialog>

DirectoryDialog::DirectoryDialog(QWidget *parent) : QDialog(parent), ui(new Ui::DirectoryDialog) {
    ui->setupUi(this);
    ui->buttonBox->buttons()[0]->setDisabled(true);
}

DirectoryDialog::~DirectoryDialog() {
    delete ui;
}

void DirectoryDialog::toggleOkButton() {
    ui->buttonBox->buttons()[0]->setEnabled(
        ui->mainLabel->text() != ui->sourceLabel->text()
        && ui->patchLabel->text() != ui->sourceLabel->text()
        && ui->mainLabel->text() != "..."
        && ui->sourceLabel->text() != "..."
        && ui->patchLabel->text() != "...");
}

void DirectoryDialog::on_mainPushButton_clicked() {
    QString path = QFileDialog::getExistingDirectory(this, "Select the main copy directory");
    if (!path.isEmpty()) {
        ui->mainLabel->setText(path);
        toggleOkButton();
    }
}

void DirectoryDialog::on_sourcePushButton_clicked() {
    QString path = QFileDialog::getExistingDirectory(this, "Select the source copy directory");
    if (!path.isEmpty()) {
        ui->sourceLabel->setText(path);
        toggleOkButton();
    }
}

void DirectoryDialog::on_patchPushButton_clicked() {
    QString path = QFileDialog::getExistingDirectory(this, "Select the patch copy directory");
    if (!path.isEmpty()) {
        ui->patchLabel->setText(path);
        toggleOkButton();
    }
}

QString DirectoryDialog::main() {
    return ui->mainLabel->text();
}

QString DirectoryDialog::source() {
    return ui->sourceLabel->text();
}

QString DirectoryDialog::patch() {
    return ui->patchLabel->text();
}
