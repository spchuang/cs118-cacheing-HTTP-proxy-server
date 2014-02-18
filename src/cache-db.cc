#include "cache-db.h"
#include <iostream>

Database::Database(std::string filename)
{
	database = NULL;
	if(!open(filename)){
   	throw SqliteException("Cannot open database", sqlite3_errmsg(database));
	}
}

Database::~Database()
{
}

bool Database::open(std::string filename)
{
	if(sqlite3_open(filename.c_str(), &database) == SQLITE_OK)
		return true;
		
	return false;   
}

vector<vector<string> > Database::query(std::string query)
{
	sqlite3_stmt *statement;
	vector<vector<string> > results;

	if(sqlite3_prepare_v2(database, query.c_str(), -1, &statement, 0) == SQLITE_OK)
	{
		int cols = sqlite3_column_count(statement);
		int result = 0;
		while(true)
		{
			result = sqlite3_step(statement);
			
			if(result == SQLITE_ROW)
			{
				vector<string> values;
				for(int col = 0; col < cols; col++)
				{
					values.push_back((char*)sqlite3_column_text(statement, col));
				}
				results.push_back(values);
			}
			else
			{
				break;   
			}
		}
	   
		sqlite3_finalize(statement);
	}
	
	string error = sqlite3_errmsg(database);
	
	if(error != "not an error"){
	   throw SqliteException("Sqlite Cache", error.c_str());
	}
	
	return results;  
}

void Database::close()
{
	sqlite3_close(database);   
}
