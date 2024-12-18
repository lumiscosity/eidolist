#include "databaseholder.h"

DatabaseHolder::DatabaseHolder(QString main, QString source, QString patch, std::string in_encoding) {
    p_tree = lcf::LMT_Reader::Load((patch + "/RPG_RT.lmt").toStdString(), in_encoding);
    m_tree = lcf::LMT_Reader::Load((main + "/RPG_RT.lmt").toStdString(), in_encoding);
    p_db = lcf::LDB_Reader::Load((patch + "/RPG_RT.ldb").toStdString(), in_encoding);
    s_db = lcf::LDB_Reader::Load((source + "/RPG_RT.ldb").toStdString(), in_encoding);
    m_db = lcf::LDB_Reader::Load((main + "/RPG_RT.ldb").toStdString(), in_encoding);
    encoding = in_encoding;
}
