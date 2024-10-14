#include "databaseholder.h"

DatabaseHolder::DatabaseHolder(QString main, QString patch) {
    p_tree = lcf::LMT_Reader::Load((patch + "RPG_RT.lmt").toStdString(), "UTF-8");
    m_tree = lcf::LMT_Reader::Load((main + "RPG_RT.lmt").toStdString(), "UTF-8");
    p_db = lcf::LDB_Reader::Load((patch + "RPG_RT.ldb").toStdString(), "UTF-8");
    m_db = lcf::LDB_Reader::Load((main + "RPG_RT.ldb").toStdString(), "UTF-8");
}
