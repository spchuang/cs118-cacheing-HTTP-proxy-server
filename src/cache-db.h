//taken from http://www.dreamincode.net/forums/topic/122300-sqlite-in-c/
#ifndef __DATABASE_H__
#define __DATABASE_H__

#include <string>
#include <vector>
#include <sqlite3.h>

using namespace std;

class Database
{
public:
	Database(string filename);
	~Database();
	
	bool open(string filename);
	vector<vector<string> > query(string query);
	void close();
	
private:
	sqlite3 *database;
	//Database *db;

};

#endif

