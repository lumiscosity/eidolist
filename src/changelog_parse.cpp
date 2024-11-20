#include "changelog_parse.h"

int readlog(QString path, QList<Asset> &assets, QList<DBAsset> &dbassets, const bool validate) {
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
