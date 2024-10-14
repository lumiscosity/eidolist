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

#include <qstring.h>

#pragma once

// diff: 0 for -, 1 for *, 2 for +

struct Asset {
    Asset(short diff, QString folder, QString name) : diff(diff), folder(folder), name(name) {}
    short diff;
    QString folder;
    QString name;
};

struct DBAsset {
    DBAsset(short diff, QString folder, int id) : diff(diff), folder(folder), id(id) {}
    short diff;
    QString folder;
    int id;
};
