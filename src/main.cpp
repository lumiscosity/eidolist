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

        DatabaseHolder h(main, source, patch);

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
        merge(main + "/RPG_RT.ldb", main + "/RPG_RT.ldb.bak");
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
                    dbmerge(main, source, patch, i, h);
                }
            }
        }
        // save modified map tree and database
        lcf::LMT_Reader::Save(
            (main + QString("/RPG_RT.lmt")).toStdString(),
            *h.m_tree,
            lcf::EngineVersion::e2k3,
            "UTF-8",
            lcf::SaveOpt::eNone);
        lcf::LDB_Reader::Save(
            (main + QString("/RPG_RT.ldb")).toStdString(),
            *h.m_db,
            "UTF-8",
            lcf::SaveOpt::eNone);

        // add the patch changelog to the main changelog
        write_changelog(main, patch);

        QMessageBox::information(nullptr, "Eidolist", "Merge complete! Don't forget to double-check if the patch was applied correctly and playtest the added content.");
    }

    return 0;
}
