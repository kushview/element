
#include "SQLiteCpp/SQLiteCpp.h"
#include "SQLiteCpp/Database.h"

#include "db/Database.h"

namespace Element {

Database::Database() {}
Database::~Database() {}

void Database::exec (const String& sql)
{
    try
    {
        File file;
        SQLite::Database db (file.getFullPathName().toUTF8(), 
            SQLite::OPEN_READWRITE | SQLite::OPEN_CREATE);
    }
    catch (std::exception& e)
    {
        DBG(e.what());
    }
}

}

#include "../../libs/SQLiteCpp/src/Backup.cpp"
#include "../../libs/SQLiteCpp/src/Column.cpp"
#include "../../libs/SQLiteCpp/src/Database.cpp"
#include "../../libs/SQLiteCpp/src/Exception.cpp"
#include "../../libs/SQLiteCpp/src/Statement.cpp"
#include "../../libs/SQLiteCpp/src/Transaction.cpp"
