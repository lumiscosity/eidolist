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
#include <qlabel.h>

namespace Ui {
class FileMergeDialog;
}

class FileMergeDialog : public QDialog
{
    Q_OBJECT

public:
    explicit FileMergeDialog(QWidget *parent = nullptr);
    ~FileMergeDialog();

    void populateLabels(QString main, QString patch);
private slots:
    void on_mainCopyButton_clicked();
    void on_patchButton_clicked();

private:
    void populateLabel(QLabel *label, QString path);
    Ui::FileMergeDialog *ui;
    QString m_main;
    QString m_patch;
};
