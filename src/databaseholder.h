#include <qstring.h>
#include <lcf/lmt/reader.h>
#include <lcf/ldb/reader.h>

#pragma once

class DatabaseHolder {
public:
    DatabaseHolder(QString main, QString source, QString patch);
    std::unique_ptr<lcf::rpg::TreeMap> p_tree = nullptr;
    std::unique_ptr<lcf::rpg::TreeMap> m_tree = nullptr;
    std::unique_ptr<lcf::rpg::Database> p_db = nullptr;
    std::unique_ptr<lcf::rpg::Database> s_db = nullptr;
    std::unique_ptr<lcf::rpg::Database> m_db = nullptr;
};

