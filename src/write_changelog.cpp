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
                patch_date = QDate::fromString(i.last(i.length() - 6), "dd/MMM/yyyy");
            }
        }
        if (!patch_date.isValid()) {
            QMessageBox::warning(nullptr, "Warning", "No valid date found in the changelog! It will have to be merged manually.");
            return;
        }
        f.close();
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
    }

    // find the right date and insert the patch log

}
