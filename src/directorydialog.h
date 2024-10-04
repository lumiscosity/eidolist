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

#pragma once

#include <QDialog>

namespace Ui {
class DirectoryDialog;
}

class DirectoryDialog : public QDialog {
    Q_OBJECT

public:
    explicit DirectoryDialog(QWidget *parent = nullptr);
    ~DirectoryDialog();

    QString orig();
    QString source();
    QString patch();
    void toggleOkButton();
private slots:
    void on_mainPushButton_clicked();
    void on_sourcePushButton_clicked();
    void on_patchPushButton_clicked();
private:
    Ui::DirectoryDialog *ui;
};
