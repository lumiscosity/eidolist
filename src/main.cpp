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

#include <QApplication>
#include <QRegularExpression>
#include <QMessageBox>
#include <qfile.h>
#include <QDirIterator>
#include <qsettings.h>

#include <lcf/rpg/map.h>

int readlog(QString path, QList<Asset> &assets, QList<DBAsset> &dbassets, const bool validate = false) {
    // 0 - success
    // 1 - file load failure
    // 2 - changelog/file list mismatch not ignored
    QRegularExpression ex("^[^ ]* [^ ]*\\[.*$");
    QRegularExpression fileex("^(?:\\S+\\s+){2}(.*?)(?:\\s*\\(|$)");

    QMap<QString, short> diffs;
    diffs["-"] = 0;
    diffs["*"] = 1;
    diffs["+"] = 2;

    QFile f(path + "/changelog.txt");
    if (f.open(QFile::ReadOnly | QFile::Text)){
        QTextStream in(&f);
        QString i;
        while (in.readLineInto(&i)) {
            if (i.length() > 5) {
                if (QStringList{"+", "*"}.contains(i.first(1))) {
                    if (ex.match(i).hasMatch() && i.mid(2, 3) == "MAP" && i.contains("]")) {
                        // map (stored in db items for convenience)
                        DBAsset file(diffs[i.first(1)], "Map", i.split("[")[1].split("]")[0].toInt());
                        if (validate && !QFile::exists(path + QString("/%1/Map%2.lmu").arg(file.folder).arg(paddedint(file.id, 4)))) {
                            if (QMessageBox::warning(
                                    nullptr,
                                    "Warning",
                                    QString("Map[%1] was mentioned in the changelog, but not included in the patch!").arg(paddedint(file.id, 4)),
                                    QMessageBox::StandardButton::Abort | QMessageBox::StandardButton::Ignore
                                    )) {
                                return 2;
                            }
                        }
                        dbassets.push_back(file);
                    } else if (!ex.match(i).hasMatch() && i.split(" ").size() >= 2) {
                        // file
                        Asset file(diffs[i.first(1)], i.split(" ")[1], fileex.match(i).captured(1));
                        if (validate && !QFile::exists(path + QString("/%1/%2").arg(file.folder).arg(file.name))) {
                            if (QMessageBox::warning(
                                nullptr,
                                "Warning",
                                QString("%1/%2 was mentioned in the changelog, but not included in the patch!").arg(file.folder).arg(file.name),
                                QMessageBox::StandardButton::Abort | QMessageBox::StandardButton::Ignore
                                    )) {
                                return 2;
                            }
                        }
                        assets.push_back(file);
                    } else {
                        // database item
                        dbassets.push_back(DBAsset(diffs[i.first(1)], i.split(" ")[1].split("[")[0], i.split("[")[1].split("]")[0].toInt()));
                    }
                }
            }
        }
        // ensure that all files in the patch have been included in the changelog
        if (validate) {
            QDirIterator iter(path, QDirIterator::Subdirectories);
            while (iter.hasNext()) {
                iter.next();
                QString name = iter.fileName();
                QString folder = iter.filePath().first(iter.filePath().length() - iter.fileName().length() - 1);
                bool found = false;
                for (Asset i : assets) {
                    if (i.diff && folder == i.folder && name == i.name) {
                        found = true;
                    }
                }
                for (DBAsset i : dbassets) {
                    if (i.diff && i.folder == "Map" && name.mid(3, 4).toInt() == i.id) {
                        found = true;
                    }
                }
                if (!found) {
                    if (QMessageBox::warning(
                            nullptr,
                            "Warning",
                            QString("%1/%2 was included in the patch, but not in the changelog!").arg(folder).arg(name),
                            QMessageBox::StandardButton::Abort | QMessageBox::StandardButton::Ignore
                            )) {
                        return 2;
                    }
                }
            }
        }
        return 0;
    } else {
        QMessageBox::critical(
            nullptr,
            "Error",
            QString("The changelog in copy %1 could not be located! Make sure that it is placed in the copy folder as changelog.txt.").arg(path),
            QMessageBox::StandardButton::Abort | QMessageBox::StandardButton::Ignore
        );
        return 1;
    }
}

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

void dbconflict(QString main, QString source, QString patch, DBAsset dbasset, DatabaseHolder &holder) {
    if (dbasset.folder == "Map") {
        // map merging
        if (dbasset.diff == 0) {
            FileMergeDialog d;
            d.populateLabels(main + QString("/Map%1.lmu").arg(paddedint(dbasset.id, 4)), patch + QString("/Map%1.lmu").arg(paddedint(dbasset.id, 4)));
        } else {
            map_diff(holder, dbasset, main, source, patch);
        }
    } else if (dbasset.folder == "CE") {
        // CE merging
        ce_diff(holder, dbasset);
    } else {
        // all other merges
        // some of these could use a custom merge workflow down the line, but i'm happy enough with these two for now
        QMessageBox::warning(nullptr, "Warning", QString("Database item %1 with ID %2 must be merged manually!").arg(dbasset.folder).arg(dbasset.id));
    }
}

void dbmerge(QString main, QString source, QString patch, DBAsset dbasset, DatabaseHolder &h) {
    if (dbasset.folder == "Map") {
        // populate the tilediff
        // for more info about this check the map_diff.cpp file
        std::unique_ptr<lcf::rpg::Map> s_map = lcf::LMU_Reader::Load((source + QString("/Map%1.lmu").arg(paddedint(dbasset.id, 4))).toStdString(), "UTF-8");
        std::unique_ptr<lcf::rpg::Map> p_map = lcf::LMU_Reader::Load((patch + QString("/Map%1.lmu").arg(paddedint(dbasset.id, 4))).toStdString(), "UTF-8");
        QSettings tilediff(main + "/eidolist_tilediff");
        QList<QSet<int>> p_changed = QList<QSet<int>>{QSet<int>(), QSet<int>()};
        // generate a list of changed tiles in the patch
        for (int i = 0; i > p_map->lower_layer.size(); i++) {
            if (p_map->lower_layer[i] !=  s_map->lower_layer[i]) {
                p_changed[0].insert(i);
            }
        }
        for (int i = 0; i > p_map->upper_layer.size(); i++) {
            if (p_map->upper_layer[i] !=  s_map->upper_layer[i]) {
                p_changed[1].insert(i);
            }
        }
        tilediff.setValue(QString::number(dbasset.id), QVariant::fromValue(p_changed));
        // merge the map
        h.m_tree->maps[dbasset.id] = h.p_tree->maps[dbasset.id];
        merge(patch + QString("/Map%1.lmu").arg(paddedint(dbasset.id, 4)), main + QString("/Map%1.lmu").arg(paddedint(dbasset.id, 4)));
    } else if (dbasset.folder == "CE") {
        h.m_db->commonevents[dbasset.id - 1] = h.p_db->commonevents[dbasset.id - 1];
    } else if (dbasset.folder == "Switch" || dbasset.folder == "S") {
        h.m_db->switches[dbasset.id - 1] = h.p_db->switches[dbasset.id - 1];
    } else if (dbasset.folder == "Variable" || dbasset.folder == "V") {
        h.m_db->variables[dbasset.id - 1] = h.p_db->variables[dbasset.id - 1];
    } else if (dbasset.folder == "Actor") {
        h.m_db->actors[dbasset.id - 1] = h.p_db->actors[dbasset.id - 1];
    } else if (dbasset.folder == "Animation") {
        h.m_db->animations[dbasset.id - 1] = h.p_db->animations[dbasset.id - 1];
    } else if (dbasset.folder == "BattlerAnim") {
        h.m_db->battleranimations[dbasset.id - 1] = h.p_db->battleranimations[dbasset.id - 1];
    } else if (dbasset.folder == "Class") {
        h.m_db->classes[dbasset.id - 1] = h.p_db->classes[dbasset.id - 1];
    } else if (dbasset.folder == "Element") {
        h.m_db->attributes[dbasset.id - 1] = h.p_db->attributes[dbasset.id - 1];
    } else if (dbasset.folder == "Enemy") {
        h.m_db->enemies[dbasset.id - 1] = h.p_db->enemies[dbasset.id - 1];
    } else if (dbasset.folder == "Item") {
        h.m_db->items[dbasset.id - 1] = h.p_db->items[dbasset.id - 1];
    } else if (dbasset.folder == "Skill") {
        h.m_db->skills[dbasset.id - 1] = h.p_db->skills[dbasset.id - 1];
    } else if (dbasset.folder == "State") {
        h.m_db->states[dbasset.id - 1] = h.p_db->states[dbasset.id - 1];
    } else if (dbasset.folder == "Terrain") {
        h.m_db->terrains[dbasset.id - 1] = h.p_db->terrains[dbasset.id - 1];
    } else if (dbasset.folder == "Tileset") {
        h.m_db->chipsets[dbasset.id - 1] = h.p_db->chipsets[dbasset.id - 1];
    } else if (dbasset.folder == "Troop") {
        h.m_db->troops[dbasset.id - 1] = h.p_db->troops[dbasset.id - 1];
    }
}

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
