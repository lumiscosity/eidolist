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

#include "databaseholder.h"
#include "diffs.h"
#include "directorydialog.h"
#include "filemerge_dialog.h"

#include <QApplication>
#include <QRegularExpression>
#include <QMessageBox>
#include <qfile.h>

int readlog(QString path, QList<Asset> &assets, QList<DBAsset> &dbassets) {
    QRegularExpression ex("^[^ ]* [^ ]*\\[.*$");
    QRegularExpression fileex("^(?:\\S+\\s+){2}(.*?)(?:\\s*\\(|$)");

    QMap<QString, short> diffs;
    diffs["-"] = 0;
    diffs["*"] = 1;
    diffs["+"] = 2;

    QFile f(path + "changelog.txt");
    if (f.open(QFile::ReadOnly | QFile::Text)){
        QTextStream in(&f);
        QString i;
        while (in.readLineInto(&i)) {
            if (i.length() > 5) {
                if (QStringList{"+", "*"}.contains(i.first(1))) {
                    if (ex.match(i).hasMatch() && i.mid(2, 3) == "MAP" && i.contains("]")) {
                        // map (stored in db items for convenience)
                        dbassets.push_back(DBAsset(diffs[i.first(1)], "Map", i.split("[")[1].split("]")[0].toInt()));
                    } else if (!ex.match(i).hasMatch() && i.split(" ").size() >= 2) {
                        // file
                        assets.push_back(Asset(diffs[i.first(1)], i.split(" ")[1], fileex.match(i).captured(1)));
                    } else {
                        // database item
                        dbassets.push_back(DBAsset(diffs[i.first(1)], i.split(" ")[1].split("[")[0], i.split("[")[1].split("]")[0].toInt()));
                    }
                }
            }
        }
        return 0;
    } else {
        return 1;
    }
}

QString paddedint(int number, int count) { return QString::number(number).rightJustified(count, char(48)); }

void remove(QString source) {
    if (QFile::exists(source)) {
        QFile::remove(source);
    }
}

void merge(QString source, QString dest) {
    remove(source);
    QFile::copy(source, dest);
}

void conflict(QString main, QString patch, Asset asset) {
    FileMergeDialog d;
    QString path = QString("/%1/%2").arg(asset.folder).arg(asset.name);
    d.populateLabels(main + path, patch + path);
    if (d.exec()) {
        if (asset.diff == 0) {
            remove(main + path);
        } else {
            merge(patch + path, main + path);
        }
    }
}

void dbconflict(QString main, QString patch, DBAsset dbasset) {
    if (dbasset.folder == "Map") {
        // map merging
        if (dbasset.diff == 0) {
            FileMergeDialog d;
            d.populateLabels(main + QString("Map%1.lmu").arg(paddedint(dbasset.id, 4)), patch + QString("Map%1.lmu").arg(paddedint(dbasset.id, 4)));
        } else {
            // TODO: add actual map merging workflow
            QMessageBox::warning(nullptr, "Warning", QString("Map %1 must be merged manually!").arg(dbasset.id));
        }
    } else if (dbasset.folder == "CE") {
        // CE merging
        // TODO: add actual CE merging workflow
        QMessageBox::warning(nullptr, "Warning", QString("CE %1 must be merged manually!").arg(dbasset.id));
    } else {
        // all other merges
        // some of these could use a custom merge workflow down the line, but i'm happy enough with these two for now
        QMessageBox::warning(nullptr, "Warning", QString("Database item %1 with ID %2 must be merged manually!").arg(dbasset.folder).arg(dbasset.id));
    }
}

void dbmerge(QString main, QString patch, DBAsset dbasset, DatabaseHolder &h) {
    if (dbasset.folder == "Map") {
        h.m_tree->maps[dbasset.id] = h.p_tree->maps[dbasset.id];
        merge(patch + QString("Map%1.lmu").arg(paddedint(dbasset.id, 4)), main + QString("Map%1.lmu").arg(paddedint(dbasset.id, 4)));
    } else if (dbasset.folder == "CE") {
        h.m_db->commonevents[dbasset.id - 1] = h.p_db->commonevents[dbasset.id - 1];
    } else if (dbasset.folder == "Switch") {
        h.m_db->switches[dbasset.id - 1] = h.p_db->switches[dbasset.id - 1];
    } else if (dbasset.folder == "Variable") {
        h.m_db->variables[dbasset.id - 1] = h.p_db->variables[dbasset.id - 1];
    } else if (dbasset.folder == "Actor") {
        h.m_db->actors[dbasset.id - 1] = h.p_db->actors[dbasset.id - 1];
    } else if (dbasset.folder == "Animation") {
        h.m_db->animations[dbasset.id - 1] = h.p_db->animations[dbasset.id - 1];
    } else if (dbasset.folder == "Item") {
        h.m_db->items[dbasset.id - 1] = h.p_db->items[dbasset.id - 1];
    } else if (dbasset.folder == "Terrain") {
        h.m_db->terrains[dbasset.id - 1] = h.p_db->terrains[dbasset.id - 1];
    } else if (dbasset.folder == "Tileset") {
        h.m_db->chipsets[dbasset.id - 1] = h.p_db->chipsets[dbasset.id - 1];
    }
}

int main(int argc, char *argv[])
{
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

        DatabaseHolder h(main, patch);

        readlog(w.main(), main_assets, main_dbassets);
        readlog(w.patch(), patch_assets, patch_dbassets);
        // run merges for files
        // assets which are being added, removed or modified for the first time this build cycle (not in main_assets) get automerged
        // otherwise, a conflict is raised
        for (Asset i : patch_assets) {
            Asset *a = nullptr;
            for (Asset j : main_assets) {
                if (j.folder == i.folder && j.name == i.name) {
                    *a = j;
                    break;
                }
            }
            if (a) {
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
        merge(main + "RPG_RT.ldb", main + "RPG_RT.ldb.bak");
        // run merges for database items and maps
        // they're grouped together simply because it's convenient
        for (DBAsset i : patch_dbassets) {
            DBAsset *a = nullptr;
            for (DBAsset j : main_dbassets) {
                if (j.folder == i.folder && j.id == i.id) {
                    *a = j;
                    break;
                }
            }
            if (a) {
                switch (a->diff) {
                    case 0: {
                        // if the item has been removed this cycle, ignore incoming removals and ask about additions/modifications
                        if (i.diff != 0) {
                            dbconflict(main, patch, i);
                        }
                    }
                    default: {
                        // if the item has been modified or added this cycle, always ask
                        dbconflict(main, patch, i);
                    }
                }
            } else {
                // item not modified this cycle; automerge
                // (except for removals, always ask about those for safety reasons)
                if (i.diff == 0) {
                    dbconflict(main, patch, i);
                } else {
                    dbmerge(main, patch, i, h);
                }
            }
        }
        // save modified map tree and database
        lcf::LMT_Reader::Save(
            (main + QString("RPG_RT.lmt")).toStdString(),
            *h.m_tree,
            lcf::EngineVersion::e2k3,
            "UTF-8",
            lcf::SaveOpt::eNone);
        lcf::LDB_Reader::Save(
            (main + QString("RPG_RT.ldb")).toStdString(),
            *h.m_db,
            "UTF-8",
            lcf::SaveOpt::eNone);
    }

    return a.exec();
}
