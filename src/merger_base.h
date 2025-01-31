#pragma once

#include "ce_diff.h"
#include "databaseholder.h"
#include "diffs.h"
#include "directorydialog.h"
#include "dialogs/filemerge_dialog.h"
#include "lcf/lmu/reader.h"
#include "map_diff.h"
#include "write_changelog.h"
#include "changelog_parse.h"

#include <QApplication>
#include <QRegularExpression>
#include <QMessageBox>
#include <qfile.h>
#include <QDirIterator>
#include <qsettings.h>

#include <lcf/rpg/map.h>

template <class T> void db_insert(std::vector<T> &source, std::vector<T> &dest, size_t id) {
    // if there is not enough space in the work copy database, extend it
    if (source.size() > dest.size()) {
        unsigned int i = dest.size();
        while (i < source.size()) {
            T empty;
            empty.ID = i + 1;
            dest.push_back(empty);
            i++;
        }
    }
    dest[id] = source[id];
};

void remove(QString source) {
    if (QFile::exists(source)) {
        QFile::remove(source);
    }
}

void merge(QString source, QString dest) {
    remove(dest);
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

int dbmerge(QString main, QString source, QString patch, DBAsset dbasset, DatabaseHolder &h) {
    if (dbasset.folder != "Map" && h.p_db == nullptr) {
        QMessageBox::critical(nullptr, "Error", QString("Database item %1 with ID %2 is in the changelog, but no LDB is provided! Aborting.").arg(dbasset.folder).arg(dbasset.id));
        return 2;
    }
    if (dbasset.folder == "Map") {
        // populate the tilediff
        // for more info about this check the map_diff.cpp file
        std::unique_ptr<lcf::rpg::Map> s_map = lcf::LMU_Reader::Load((source + QString("/Map%1.lmu").arg(paddedint(dbasset.id, 4))).toStdString(), h.encoding);
        std::unique_ptr<lcf::rpg::Map> p_map = lcf::LMU_Reader::Load((patch + QString("/Map%1.lmu").arg(paddedint(dbasset.id, 4))).toStdString(), h.encoding);
        if (p_map == nullptr) {
            QMessageBox::warning(nullptr, "Warning", QString("Map%1 was not found in the patch copy, despite being mentioned in the changelog!").arg(paddedint(dbasset.id, 4)));
            return 1;
        }
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
        if (h.p_tree != nullptr){
            h.m_tree->maps[dbasset.id] = h.p_tree->maps[dbasset.id];
        }
        merge(patch + QString("/Map%1.lmu").arg(paddedint(dbasset.id, 4)), main + QString("/Map%1.lmu").arg(paddedint(dbasset.id, 4)));
    } else if (dbasset.folder == "CE") {
        db_insert(h.p_db->commonevents, h.m_db->commonevents, dbasset.id - 1);
    } else if (dbasset.folder == "Switch" || dbasset.folder == "S") {
        db_insert(h.p_db->switches, h.m_db->switches, dbasset.id - 1);
    } else if (dbasset.folder == "Variable" || dbasset.folder == "V") {
        db_insert(h.p_db->variables, h.m_db->variables, dbasset.id - 1);
    } else if (dbasset.folder == "Actor") {
        db_insert(h.p_db->actors, h.m_db->actors, dbasset.id - 1);
    } else if (dbasset.folder == "Animation") {
        db_insert(h.p_db->animations, h.m_db->animations, dbasset.id - 1);
    } else if (dbasset.folder == "BattlerAnim") {
        db_insert(h.p_db->battleranimations, h.m_db->battleranimations, dbasset.id - 1);
    } else if (dbasset.folder == "Class") {
        db_insert(h.p_db->classes, h.m_db->classes, dbasset.id - 1);
    } else if (dbasset.folder == "Element") {
        db_insert(h.p_db->attributes, h.m_db->attributes, dbasset.id - 1);
    } else if (dbasset.folder == "Enemy") {
        db_insert(h.p_db->enemies, h.m_db->enemies, dbasset.id - 1);
    } else if (dbasset.folder == "Item") {
        db_insert(h.p_db->items, h.m_db->items, dbasset.id - 1);
    } else if (dbasset.folder == "Skill") {
        db_insert(h.p_db->skills, h.m_db->skills, dbasset.id - 1);
    } else if (dbasset.folder == "State") {
        db_insert(h.p_db->states, h.m_db->states, dbasset.id - 1);
    } else if (dbasset.folder == "Terrain") {
        db_insert(h.p_db->terrains, h.m_db->terrains, dbasset.id - 1);
    } else if (dbasset.folder == "Tileset") {
        db_insert(h.p_db->chipsets, h.m_db->chipsets, dbasset.id - 1);
    } else if (dbasset.folder == "Troop") {
        db_insert(h.p_db->troops, h.m_db->troops, dbasset.id - 1);
    }
    return 0;
}
