#include "cache-db.h"
#include <stdlib.h>     /* atoi */
#include <iostream>

Database::Database(std::string filename)
{
	database = NULL;
	if(!open(filename)){
   	throw SqliteException("Cannot open database", sqlite3_errmsg(database));
	}
	if(!sqlite3_threadsafe()){
   	cout <<"[WARNING] This is not thread safe"<<endl;
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


bool Database::containsHostPathCache(string host_path)
{
   string sql ="SELECT count(HOST_PATH) FROM CACHE WHERE HOST_PATH='"+host_path+"';";

   vector<vector<string> > result = query(sql.c_str());
   
   string count_string = result.at(0).at(0);
   int count = atoi(count_string.c_str());
   
   return count>0;
}

void Database::insertCache(string host_path, string expire, string resp)
{
   string sql = "INSERT INTO CACHE VALUES('"+host_path+"', '"+expire+"', '"+resp+"');" ;
   query(sql.c_str());
}


//   return vector<string>
//   [0] = EXPIRE
//   [1] = FORMATED_RESP

vector<string> Database::getCache(string host_path){
   string sql = "SELECT EXPIRE, FORMATED_RESP FROM CACHE  WHERE HOST_PATH='"+host_path+"';";
   vector<vector<string> > result = query(sql.c_str());
   return result.at(0);
}

void Database::updateCache(string host_path, string expire, string resp)
{
   string sql = "UPDATE CACHE SET EXPIRE='"+expire+"',FORMATED_RESP='"+resp+"' WHERE HOST_PATH='"+host_path+"';" ;
   query(sql.c_str());
}
