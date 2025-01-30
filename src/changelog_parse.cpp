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
                        if (validate && !QFile::exists(path + QString("/Map%1.lmu").arg(paddedint(file.id, 4)))) {
                            if (QMessageBox::warning(
                                    nullptr,
                                    "Warning",
                                    QString("Map[%1] was mentioned in the changelog, but not included in the patch!").arg(paddedint(file.id, 4)),
                                    QMessageBox::StandardButton::Abort | QMessageBox::StandardButton::Ignore
                                    ) == QMessageBox::StandardButton::Abort) {
                                return 2;
                            }
                        }
                        dbassets.push_back(file);
                    } else if (!ex.match(i).hasMatch() && i.split(" ").size() >= 2) {
                        // file
                        Asset file(diffs[i.first(1)], i.split(" ")[1], fileex.match(i).captured(1));
                        if (validate && !any_exists(path + QString("/%1/%2").arg(file.folder).arg(file.name))) {
                            if (QMessageBox::warning(
                                    nullptr,
                                    "Warning",
                                    QString("%1/%2 was mentioned in the changelog, but not included in the patch!").arg(file.folder).arg(file.name),
                                    QMessageBox::StandardButton::Abort | QMessageBox::StandardButton::Ignore
                                    ) == QMessageBox::StandardButton::Abort) {
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

        return 0;
    } else {
        QMessageBox::critical(
            nullptr,
            "Error",
            QString("The changelog in copy %1 could not be located! Make sure that it is placed in the copy folder as changelog.txt.").arg(path),
            QMessageBox::StandardButton::Abort
            );
        return 1;
    }
}

bool any_exists(QString path) {
    return
        QFile::exists(path)
        || QFile::exists(path + ".png")
        || QFile::exists(path + ".bmp")
        || QFile::exists(path + ".xyz")
        || QFile::exists(path + ".mp3")
        || QFile::exists(path + ".wav")
        || QFile::exists(path + ".ogg");
}
