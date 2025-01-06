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

#include "ce_diff.h"
#include "databaseholder.h"
#include "diffs.h"
#include "directorydialog.h"
#include "dialogs/filemerge_dialog.h"
#include "lcf/lmu/reader.h"
#include "map_diff.h"
#include "write_changelog.h"
#include "changelog_parse.h"
#include "merger_base.h"

#include <QApplication>
#include <QRegularExpression>
#include <QMessageBox>
#include <qfile.h>
#include <QDirIterator>
#include <qsettings.h>

#include <lcf/rpg/map.h>


int main(int argc, char *argv[]) {
    QApplication a(argc, argv);
    DirectoryDialog w;
    if (w.exec()) {
        // read the changelog
        QList<Asset> main_assets;
        QList<DBAsset> main_dbassets;
        QList<Asset> patch_assets;
        QList<DBAsset> patch_dbassets;

        QString main = w.main();
        QString patch = w.patch();
        QString source = w.source();

        DatabaseHolder h(main, source, patch, w.encoding());

        // if there is no changelog for the main copy yet, create one
        if (!QFile::exists(main + "/changelog.txt")) {
            QFile f(main + "/changelog.txt");
            f.open(QFile::OpenModeFlag::WriteOnly);
            f.write("");
            f.close();
        } else if (readlog(main, main_assets, main_dbassets)) {
            return 1;
        }
        if (readlog(patch, patch_assets, patch_dbassets)) {
            return 1;
        }
        // ensure that all files in the patch have been included in the changelog
        QDirIterator iter(patch, QDirIterator::Subdirectories);
        while (iter.hasNext()) {
            QString i = iter.next();
            if (i.endsWith(".") || i.endsWith(".lmt") || i.endsWith(".ldb") || i.endsWith(".txt") || i.endsWith(".exe") || i.endsWith(".ini") || !i.contains(".")) {
                continue;
            }
            QString name = iter.fileName();
            QString folder = iter.filePath().first(iter.filePath().length() - iter.fileName().length() - 1);
            bool found = false;
            for (Asset i : patch_assets) {
                if (i.diff && folder == i.folder && name == i.name) {
                    found = true;
                }
            }
            for (DBAsset i : patch_dbassets) {
                if (i.diff && i.folder == "Map" && name.mid(3, 4).toInt() == i.id) {
                    found = true;
                }
            }
            if (!found) {
                if (QMessageBox::warning(
                        nullptr,
                        "Warning",
                        QString("%1/%2 was included in the patch, but not in the changelog!").arg(folder.split("/").last()).arg(name),
                        QMessageBox::StandardButton::Abort | QMessageBox::StandardButton::Ignore
                        ) == QMessageBox::StandardButton::Abort) {
                    return 2;
                }
                for (int i = 0; i <= patch_assets.size(); i++) {
                    if (patch_assets[i].diff && folder == patch_assets[i].folder && name == patch_assets[i].name) {
                        patch_assets.remove(i);
                        break;
                    }
                }
                // TODO: this does nothing. try adding a map to a changelog and not including it in the patch. if you don't abort it breaks.
                for (int i = 0; i <= patch_dbassets.size(); i++) {
                    if (patch_dbassets[i].diff && patch_dbassets[i].folder == "Map" && name.mid(3, 4).toInt() == patch_dbassets[i].id) {
                        patch_dbassets.remove(i);
                        break;
                    }
                }
            }
        }
        // run merges for files
        // assets which are being added, removed or modified for the first time this build cycle (not in main_assets) get automerged
        // otherwise, a conflict is raised
        for (Asset i : patch_assets) {
            Asset *a = nullptr;
            for (Asset j : main_assets) {
                if (j.folder == i.folder && j.name == i.name) {
                    a = &j;
                    break;
                }
            }
            if (a != nullptr) {
                switch (a->diff) {
                    case 0: {
                        // if the item has been removed this cycle, ignore incoming removals and ask about additions/modifications
                        if (i.diff != 0) {
                            conflict(main, patch, i);
                        }
                    }
                    default: {
                        // if the item has been modified or added this cycle, always ask
                        conflict(main, patch, i);
                    }
                }
            } else {
                // item not modified this cycle; automerge
                // (except for removals, always ask about those for safety reasons)
                if (i.diff == 0) {
                    conflict(main, patch, i);
                } else {
                    QString path = QString("/%1/%2").arg(i.folder).arg(i.name);
                    merge(patch + path, main + path);
                }
            }
        }
        // make a backup of the database. eidolist is beta grade software!
        QFile::copy(main + "/RPG_RT.ldb", main + "/RPG_RT.ldb.bak");
        // run merges for database items and maps
        // they're grouped together simply because it's convenient
        for (DBAsset i : patch_dbassets) {
            DBAsset *a = nullptr;
            for (DBAsset j : main_dbassets) {
                if (j.folder == i.folder && j.id == i.id) {
                    a = &j;
                    break;
                }
            }
            if (a != nullptr) {
                switch (a->diff) {
                    case 0: {
                        // if the item has been removed this cycle, ignore incoming removals and ask about additions/modifications
                        if (i.diff != 0) {
                            dbconflict(main, source, patch, i, h);
                        }
                    }
                    default: {
                        // if the item has been modified or added this cycle, always ask
                        dbconflict(main, source, patch, i, h);
                    }
                }
            } else {
                // item not modified this cycle; automerge
                // (except for removals, always ask about those for safety reasons)
                if (i.diff == 0) {
                    dbconflict(main, source, patch, i, h);
                } else {
                    if (dbmerge(main, source, patch, i, h) == 2) {
                        return 1;
                    }
                }
            }
        }
        // save modified map tree and database
        if (h.m_tree != nullptr) {
            lcf::LMT_Reader::Save(
                (main + QString("/RPG_RT.lmt")).toStdString(),
                *h.m_tree,
                lcf::GetEngineVersion(*h.m_db),
                h.encoding,
                lcf::SaveOpt::eNone);
        }
        if (h.m_db != nullptr) {
            lcf::LDB_Reader::Save(
                (main + QString("/RPG_RT.ldb")).toStdString(),
                *h.m_db,
                h.encoding,
                lcf::SaveOpt::eNone);
        }

        // add the patch changelog to the main changelog
        write_changelog(main, patch);

        QMessageBox::information(nullptr, "Eidolist", "Merge complete! Don't forget to double-check if the patch was applied correctly and playtest the added content.");
    }

    return 0;
}
