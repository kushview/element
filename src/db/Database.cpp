
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