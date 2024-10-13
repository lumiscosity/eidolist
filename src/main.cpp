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

#include "diffs.h"
#include "directorydialog.h"

#include <QApplication>
#include <QRegularExpression>
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
                    }
                }
            }
        }
        return 0;
    } else {
        return 1;
    }
}

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    DirectoryDialog w;
    if (w.exec()) {
        QList<Asset> main_assets;
        QList<DBAsset> main_dbassets;
        QList<Asset> patch_assets;
        QList<DBAsset> patch_dbassets;

        readlog(w.main(), main_assets, main_dbassets);
        readlog(w.patch(), patch_assets, patch_dbassets);

    }

    return a.exec();
}
