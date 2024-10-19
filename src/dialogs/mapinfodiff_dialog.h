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

#include <lcf/rpg/mapinfo.h>

namespace Ui {
class MapInfoDiffDialog;
}

class MapInfoDiffDialog : public QDialog
{
    Q_OBJECT

public:
    explicit MapInfoDiffDialog(QWidget *parent = nullptr);
    ~MapInfoDiffDialog();

    void populateLabels(lcf::rpg::MapInfo main, lcf::rpg::MapInfo patch);
private slots:
    void on_mainCopyButton_clicked();
    void on_patchButton_clicked();
private:
    void populateLabel(QLabel *label, lcf::rpg::MapInfo info);
    Ui::MapInfoDiffDialog *ui;
    QString m_main;
    QString m_patch;
};
