#include "write_changelog.h"

#include <qdatetime.h>
#include <qfile.h>
#include <qmessagebox.h>
#include <qtextstream.h>

void write_changelog(QString main, QString patch) {
    // get the date
    QFile f(main + "/changelog.txt");
    QDate patch_date;
    if (f.open(QFile::ReadOnly | QFile::Text)) {
        QString i;
        QTextStream in(&i);
        while (in.readLineInto(&i)) {
            if (i.first(5) == "Date:") {
                patch_date = QDate::fromString(i.trimmed().last(i.length() - 6), "dd/MMM/yyyy");
            }
        }
        if (!patch_date.isValid()) {
            QMessageBox::warning(nullptr, "Warning", "No valid date found in the changelog! It will have to be merged manually.");
            return;
        }
        f.close();
    } else {
        QMessageBox::warning(nullptr, "Warning", "The patch changelog cound not be opened!");
        return;
    }

    // read the main log into a stringlist
    QStringList main_log;
    QFile f_main(main + "/changelog.txt");
    if (f_main.open(QFile::ReadOnly | QFile::Text)) {
        QTextStream in(&f_main);
        QString i;
        while (in.readLineInto(&i)) {
            main_log.append(i.trimmed());
        }
        f_main.close();
    } else {
        QMessageBox::warning(nullptr, "Warning", "The main changelog cound not be opened!");
    }

    // find the right date and insert the patch log
    int insert_at = 0;
    for (QString i : main_log) {
        if (i.first(5) == "Date:") {
            if (patch_date < QDate::fromString(i.trimmed().last(i.length() - 6), "dd/MMM/yyyy")) {
                break;
            }
        }
        insert_at++;
    }
    // backtrack to the separator before this date
    QString sep_check = main_log[insert_at];
    while (insert_at > 0 && sep_check != "|=|=|=|=|=|=|=|=|=|=|=|=|=|=|=|=|=|=|=|=|=|=|=|=|=|") {
        insert_at--;
        sep_check = main_log[insert_at];
    }
    // write the patch changelog here, ignoring the first line unless no separator is found
    // (the line is the first separator, in a valid changelog, but a doubled separator is better than none for fallback reasons)
    if (f.open(QFile::ReadOnly | QFile::Text)) {
        QString i;
        QTextStream in(&i);
        if (insert_at > 0) {
            in.readLineInto(nullptr);
        }
        while (in.readLineInto(&i)) {
            main_log.insert(insert_at, i);
            insert_at++;
        }
        f.close();
    } else {
        QMessageBox::warning(nullptr, "Warning", "The patch changelog cound not be opened!");
        return;
    }
}
