#include "map_diff.h"

#include "dialogs/mapinfodiff_dialog.h"

#include <QSettings>
#include <qmessagebox.h>

#include <lcf/rpg/map.h>
#include <lcf/lmu/reader.h>

int first_free_id(std::unique_ptr<lcf::rpg::Map> &map) {
    // adapted from EasyRPG Editor
    std::vector<lcf::rpg::Event>::iterator ev;
    int id = 1;
    for (;;++id) {
        bool valid = true;
        for (ev = map->events.begin(); ev != map->events.end(); ++ev) {
            if (ev->ID == id) {
                valid = false;
                break;
            }
        }
        if (valid) {
            break;
        }
    }

    return id;
}

int map_diff(DatabaseHolder &d, DBAsset asset, QString main, QString source, QString patch) {
    // 0: success
    // 1: automerge failed, no major errors
    // 2: major error

    // check the map metadata
    if (d.p_tree != nullptr) {
        bool oob_map_main = d.m_tree->maps.size() < asset.id;
        bool oob_map_patch = d.p_tree->maps.size() < asset.id;
        if (oob_map_main) {
            // TODO: add dummy maps to the main copy if necessary
            QMessageBox::critical(nullptr, "Error", QString("Not enough map slots in the main copy to merge Map[%1]!").arg(paddedint(asset.id, 4)));
            return 2;
        }
        if (oob_map_patch) {
            QMessageBox::critical(
                nullptr,
                "Error",
                QString("Attempted to merge Map[%1], which doesn't exist in the patch!").arg(paddedint(asset.id, 4)));
            return 2;
        } else if (d.p_tree->maps[asset.id] != d.m_tree->maps[asset.id]) {
            MapInfoDiffDialog dialog;
            dialog.populateLabels(d.p_tree->maps[asset.id], d.m_tree->maps[asset.id]);
            if (dialog.exec()) {
                d.m_tree->maps[asset.id] = d.p_tree->maps[asset.id];
            }
        }
    }

    // merge map tiles
    // this actually uses an external file (and gross misuse of qsettings)
    // to track which tiles have been changed in the main copy so far
    // if none of the tiles overlap, we can automerge; else a manual diff is required
    std::unique_ptr<lcf::rpg::Map> m_map = lcf::LMU_Reader::Load((main + QString("/Map%1.lmu").arg(paddedint(asset.id, 4))).toStdString(), d.encoding);
    std::unique_ptr<lcf::rpg::Map> s_map = lcf::LMU_Reader::Load((source + QString("/Map%1.lmu").arg(paddedint(asset.id, 4))).toStdString(), d.encoding);
    std::unique_ptr<lcf::rpg::Map> p_map = lcf::LMU_Reader::Load((patch + QString("/Map%1.lmu").arg(paddedint(asset.id, 4))).toStdString(), d.encoding);
    QSettings tilediff(main + "/eidolist_tilediff");

    if (!m_map || !s_map || !p_map) {
        return 1;
    }

    if (m_map->height != s_map->height || s_map->height != p_map->height || m_map->width != s_map->width || s_map->width != p_map->width) {
        QMessageBox::warning(
            nullptr,
            "Automerge failure",
            QString("The size of Map[%1] differs between the main, source and patch copies. As such, the map must be merged manually.").arg(paddedint(asset.id, 4)));
        return 1;
    }

    // theoretically i could make a custom type instead of chaining qlists like this
    // but that'd require fiddling with qt metatype macros and i just want to be done with this
    // there are always 2 inner lists: one for the lower layer, and one for the upper layer
    QList<QSet<int>> m_changed = tilediff.value(QString::number(asset.id)).value<QList<QSet<int>>>();
    if (m_changed.isEmpty()) {
        m_changed = QList<QSet<int>>{QSet<int>(), QSet<int>()};
    }
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

    // compare the lists
    bool can_automerge_tiles = true;
    for (int i : p_changed[0]) {
        if (m_changed[0].contains(i)) {
            can_automerge_tiles = false;
        }
    }
    for (int i : p_changed[0]) {
        if (m_changed[0].contains(i)) {
            can_automerge_tiles = false;
        }
    }

    if (can_automerge_tiles) {
        for (int i : p_changed[0]) {
            m_map->lower_layer[i] = p_map->lower_layer[i];
        }
        for (int i : p_changed[1]) {
            m_map->upper_layer[i] = p_map->upper_layer[i];
        }
        m_changed[0].unite(p_changed[0]);
        m_changed[1].unite(p_changed[1]);
        tilediff.setValue(QString::number(asset.id), QVariant::fromValue(m_changed));
    } else {
        QMessageBox::warning(
            nullptr,
            "Automerge failure",
            QString("The same tiles in Map[%1] have already been modified in this build cycle. As such, the map must be merged manually.").arg(paddedint(asset.id, 4)));
        return 1;
    }

    // merge events
    // TODO: add a warning if events overlap

    // get every new and changed event in the original map
    for (lcf::rpg::Event i : p_map->events) {
        bool found = false;
        for (lcf::rpg::Event j : s_map->events) {
            if (i.ID == j.ID && i != j) {
                // event at this id was changed
                bool deleted = true;
                for (int k = 0; k < m_map->events.size(); k++) {
                    // replace the one in the new map, if it still exists
                    if (m_map->events[k].ID == i.ID) {
                        m_map->events[k] = i;
                    }
                    deleted = false;
                    break;
                }
                if (deleted) {
                    QMessageBox::warning(
                        nullptr,
                        "Warning",
                        QString("Event %1 in Map[%2] was modified in the patch, but it has been removed in the main copy! Skipping.").arg(i.ID).arg(paddedint(asset.id, 4)));
                }
                found = true;
                break;
            }
        }
        if (!found) {
            // new event; add it to the map
            i.ID = first_free_id(m_map);
            m_map->events.push_back(i);
        }
    }

    // get every removed event
    int removed_count = 0;
    for (lcf::rpg::Event i : s_map->events) {
        bool found = false;
        for (lcf::rpg::Event j : p_map->events) {
            if (i.ID == j.ID) {
                found = true;
                break;
            }
        }
        if (!found) {
            // removed event; remove it from the map
            for (int k = 0; k < m_map->events.size(); k++) {
                // replace the one in the new map, if it still exists
                if (m_map->events[k].ID == i.ID) {
                    m_map->events.erase(m_map->events.begin() + k);
                }
                removed_count++;
                break;
            }
        }
    }
    if (removed_count) {
        QMessageBox::warning(
            nullptr,
            "Warning",
            QString("%1 events have been removed from Map[%2]. Make sure everything still works fine!").arg(removed_count).arg(paddedint(asset.id, 4)));
    }

    // save the map
    lcf::LMU_Reader::Save(
        (main + QString("/Map%1.lmu").arg(paddedint(asset.id, 4))).toStdString(),
        *m_map,
        lcf::GetEngineVersion(*d.m_db),
        d.encoding,
        lcf::SaveOpt::eNone
    );

    return 0;
}
