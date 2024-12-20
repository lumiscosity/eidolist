#pragma once

#include <QString>
#include <QList>
#include "diffs.h"
#include <QFile>
#include <QMessageBox>
#include <QRegularExpression>
#include <QDirIterator>

int readlog(QString path, QList<Asset> &assets, QList<DBAsset> &dbassets, const bool validate = false);
