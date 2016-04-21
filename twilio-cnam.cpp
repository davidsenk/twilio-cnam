#include <iostream>
#include <curl/curl.h>
#include <sqlite3.h>
#include <ctime>
#include <string>


#include "json.hpp"

#define SQLiteDB "/var/spool/asterisk/cnam/cnam.db"

//1 week
#define CacheTime 604800

#include "tokens.hpp"

#define DEBUG 0

using namespace std;
using json = nlohmann::json;


size_t CurlCallback(void *contents, size_t size, size_t nmemb, string *s)
{
    size_t newLength = size*nmemb;
    size_t oldLength = s->size();
    try
    {
        s->resize(oldLength + newLength);
    }
    catch(bad_alloc &e)
    {
        //handle memory problem
        return 0;
    }

    copy((char*)contents,(char*)contents+newLength,s->begin()+oldLength);
    return size*nmemb;
}

void sqlite_errordisp(sqlite3 *db)
{
        cerr << "sqlite error " << sqlite3_errmsg(db) << endl;
	sqlite3_close(db);
}

void createtables(sqlite3 *db)
{
	int rc;
	string sql = "CREATE TABLE IF NOT EXISTS cache";
		sql +=	"(phone TEXT(10),";
		sql +=	"name TEXT(20),";
		sql +=	"time INT,";
		sql +=	"id INTEGER NOT NULL PRIMARY KEY AUTOINCREMENT);";

	rc = sqlite3_exec(db, sql.c_str(), 0, 0, NULL);

	if(rc)
	{
		sqlite_errordisp(db);
		return;
	}
}

string timestr(time_t t) {
   stringstream strm;
   strm << t;
   return strm.str();
}

void insertcache(string phone, string name, sqlite3 *db)
{
	int rc;
	string sql = "INSERT INTO cache VALUES ('";
		sql += phone + "', '";
		sql += name + "', ";
		sql += timestr(time(NULL)) + ", NULL);";

	rc = sqlite3_exec(db, sql.c_str(), 0, 0, NULL);

        if(rc)
        {
                sqlite_errordisp(db);
                return;
        }
}
bool cachehit;
int callback(void *data, int argc, char **argv, char **azColName)
{
	if(argc > 0)
	{
		cachehit = true;
		cout << argv[0];
	}
	return 0;
}
void checkcache(string phone, sqlite3 *db)
{
	int rc;
	cachehit = false;
	string sql = "SELECT name FROM cache WHERE phone = '" + phone + "'";
		sql += " and time > strftime('%s', 'now') - " + to_string(CacheTime);
		sql += " order by id desc limit 1;";
	rc = sqlite3_exec(db, sql.c_str(), callback, 0, NULL);

	if(rc)
        {
                sqlite_errordisp(db);
                return;
        }
}

//create database if not exists or open existing
bool opendb(sqlite3 *&db, string path)
{
//passing a pointer by reference? can't get any fuckier than that!
	int rc;
	rc = sqlite3_open(path.c_str(), &db);
	if(rc)
	{
		sqlite_errordisp(db);
		return false;
	}
	createtables(db);
}

int main(int argc, char **argv)
{
	if(!(argc > 1))
		return 1; //not enough args
	CURL *curl;
	CURLcode res;
	
	curl_global_init(CURL_GLOBAL_DEFAULT);
	curl = curl_easy_init();

	string url = "https://lookups.twilio.com/v1/PhoneNumbers/";
	url += argv[1];
	url += "?Type=caller-name";

	#if DEBUG > 0
		cout << url << endl << endl;
	#endif
	string s;

	sqlite3 *db;


	bool dbopen = false;
	cachehit = false;
	dbopen = opendb(db, SQLiteDB);
	checkcache(argv[1], db);
	
	if(curl && !cachehit)
	{
		#if DEBUG > 0
			curl_easy_setopt(curl, CURLOPT_VERBOSE, 1);
		#endif
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, CurlCallback);
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, &s);
		curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
		curl_easy_setopt(curl, CURLOPT_USERNAME, AccountSID);
		curl_easy_setopt(curl, CURLOPT_PASSWORD, AuthToken);
		curl_easy_setopt(curl, CURLOPT_HTTPGET, 1);
		res = curl_easy_perform(curl);
		if(res != CURLE_OK)
		{
			cerr << "curl failed " << curl_easy_strerror(res) << endl;
		}
		else
		{
			try
			{
				json resp = json::parse(s);
				string name = resp["caller_name"]["caller_name"];
				if(name == "null")
				{
					throw "name not found";
				}
				//name.erase(1);
				//name.erase(name.length()-1);
				cout << name;
				insertcache(argv[1], name, db);
			}
			catch(exception &e)
			{
				cerr << "unable to parse json response" << endl;
				cerr << e.what();
			}
		}
		curl_easy_cleanup(curl);
	}
	curl_global_cleanup();
	sqlite3_close(db);
	return 0;
}
