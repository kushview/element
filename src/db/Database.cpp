/*
    This file is part of Element
    Copyright (C) 2019  Kushview, LLC.  All rights reserved.

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

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
