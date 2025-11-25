
#include <iostream>
#include <fstream>
#include <string>
#include <cstring>
#include <iomanip>
#include <sstream>
#include <sqlite3.h>
#include <filesystem>
#include "openssl/sha.h"

using namespace std;

//Build Commands:
// g++ -o logappend logappend.cpp -lssl -lcrypto -lsqlite3
// Usage:
// ./logappend <query>

//  Query Format:
// ./logappend -T <timestamp> -K <token> (-E <employee-name> | -G <guest-name>) (-A | -L) [-R <room-id>] <log>
// ./logappend -B <file>

// Example Query:
// ./logappend -T 5 -K secret -L -E Fred -R 1 log1

// To-do:
// Check DB exists X
// Create DB X
// Process Query to DB X
// edit check token to look at first column
// Implement Log File Encryption - AES
// Implement Input Sanitization? - regex

void ProcessBatchFile(string); 
bool ParseQuery(int, char**);
void ProcessQuery(int, char**);
// Debugging Function
void PrintQuery(int, char**);

void CreateDB(string, string);
bool CheckDBExists(string);
bool CheckDBToken(string, string);
//static int callback(void*, int, char**, char**);
string sha256(const std::string);

// deprecated
bool CheckToken(string, string);
bool CheckLogExists(string);
void CreateLog(string, string);



int main(int argc, char* argv[]) 
{
    //check if query is batch file
    if(strcmp(argv[1], "-B") == 0)
    {
        ProcessBatchFile(argv[2]);
    }

    //check if normal query and valid
    else if(strcmp(argv[1], "-T") == 0)
    {
        if (ParseQuery(argc, argv) == false)
        {
            cout << "Error: Invalid Query" << endl;
        }
        else 
        {
            //valid query, check if log file exists
            if (CheckDBExists(argv[10]) == false)
            {
                // create db file with token
                // first entry in db is a blank entry that only contains the token
                CreateDB(argv[10], sha256(argv[4]));
                ProcessQuery(argc, argv);
            }else
            {
                if (CheckDBToken(argv[10], sha256(argv[4])) == false)
                {
                    cout << "Error: Invalid Token" << endl;
                }
            }
        }
    }

} 

/*
    ==========OLD MAIN==========

    //check if query is batch file
    if(strcmp(argv[1], "-B") == 0)
    {
        ProcessBatchFile(argv[2]);
    } 
    //check if normal query
    else if(strcmp(argv[1], "-T") == 0)
    {
        //check if query is valid
        if (ParseQuery(argc, argv) == false)
        {
            cout << "Error: Invalid Query" << endl;
        }
        else 
        {
            //valid query, check if log file exists
            if (CheckLogExists(argv[10]) == false)
            {
                //create log file with token
                //CreateLog(argv[10], sha256(argv[4]));
                CreateDB(argv[10], sha256(argv[4]));
                //write query to log file
                ofstream file;
                file.open(string(argv[10]) + ".txt", std::ios::app);
                for (int i = 1; i < argc; i++)
                {
                    file << argv[i] << " ";
                }
                file << endl;
                file.close();
            }
            else 
            {
                //log file exists, check token, then write query to log file
                // ===LATER=== if valid, check if the logic is consistent (going back in time, leaving a room never entered)
                //for now, if valid write to log file
                if (CheckToken(argv[10], sha256(argv[4])) == false)
                {
                    cout << "Error: Invalid Token" << endl;
                }
                else
                {
                    ofstream file;
                    file.open(string(argv[10]) + ".txt", std::ios::app);
                    for (int i = 1; i < argc; i++)
                    {
                        file << argv[i] << " ";
                    }
                    file << endl;
                    file.close();
                }
            }
        }
    } else
    {
        cout << "Error: Invalid Query" << endl;
    }

*/

void ProcessBatchFile(string batchName)
{
    // -B <file>
    // batch file, contains list of commands without logappend beginning
    // -B cannot appear within the batch file
    //cout << batchName << endl;
    ifstream file(batchName);
    string line;
    while (getline(file, line))
    {
        cout << "Processing line: " << line << endl;
        //split line into arguments
        const int MAX_ARGS = 11;
        char* args[MAX_ARGS];
        args[0] = const_cast<char*>("");
        int argc = 1;
        char* token = strtok(const_cast<char*>(line.c_str()), " ");
        while (token != nullptr && argc < MAX_ARGS)
        {
            args[argc] = token;
            argc++;
            token = strtok(nullptr, " ");
        }
        //process each query
        if (ParseQuery(argc, args) == false)
        {
            cout << "Error: Invalid Query in Batch File" << endl;
        }
        else 
        {
            //valid query, check if log file exists
            if (CheckLogExists(args[10]) == false)
            {
                //create log file with token
                CreateLog(args[10], sha256(args[4]));
                //write query to log file
                cout << "Writing to new log file: " << args[10] << endl;
                ofstream logfile;
                logfile.open(string(args[10]) + ".txt", std::ios::app);
                for (int i = 0; i < argc; i++)
                {
                    logfile << args[i] << " ";
                }
                logfile << endl;
                logfile.close();
            }
            else 
            {
                //log file exists, check token, then write query to log file
                if (CheckToken(args[10], sha256(args[4])) == false)
                {
                    cout << "Error: Invalid Token in Batch File" << endl;
                }
                else
                {
                    ofstream logfile;
                    logfile.open(string(args[10]) + ".txt", std::ios::app);
                    for (int i = 0; i < argc; i++)
                    {
                        logfile << args[i] << " ";
                    }
                    logfile << endl;
                    logfile.close();
                }
            }
        }
    }
    
    file.close();
}

int callbackB(void *data, int count, char **argv, char **columnNames){
    //1st parameter of this function is received from 4th parameter of sqlite3_exec
    //count -> is the number of columns
    //columnNames ->  array of pointers to strings where each entry represents the name of corresponding result column as obtained
    //argv -> array of pointers to strings obtained as if from [sqlite3_column_text()]

   int i;
   for(i = 0; i<count; i++) {
      printf("Col: %s = %s\n", columnNames[i], argv[i]);
   }
   printf("\n");
   return 0;
}

static int callbackC(void *count, int argc, char **argv, char **azColName) {
    int *c = (int*)count;
    *c = atoi(argv[0]);
    return 0;
}

void ProcessQuery(int argc, char* query[])
{
    //SQL variables
    sqlite3 *db;
    char *errMsg = 0;
    int rc;
    int count = 0;

    //query variables
    string time = query[2];
    string token = sha256(query[4]);
    string AL = query[5];
    string name = query[7];
    string EG = query[6];
    string roomid = query[9];

    std::cout << "filename: " << query[10] << std::endl;

    string dbname = string(query[10]) + ".db";

    rc = sqlite3_open(dbname.c_str(), &db);
    rc = sqlite3_exec(db, "select count(*) from LOGFILE", callbackC, &count, &errMsg);

    string sql = "INSERT INTO LOGFILE (ID, TOKEN, TIME, NAME, EG, AL, ROOMID) "
                "VALUES("+std::to_string(count)+",'"+token+"','"+time+"','"+name+"','"+EG+"','"+AL+"','"+roomid+"')";
    
    rc = sqlite3_exec(db, sql.c_str(), callbackC, 0, &errMsg);

    
}

// Make sure that query is valid before writing to log file
bool ParseQuery(int argc, char* query[]){
    bool valid = true;

    //cout << "argc: " << argc << endl;

    // Check if query has correct number of arguments
    if (argc <= 10)
    {
        cout << "Error - Invalid Query: Missing Arguments" << endl;
        return false;
    }
    if (argc > 11)
    {
        cout << "Error - Invalid Query: Too Many Arguments" << endl;
        return false;
    }

    // -T <timestamp>
    // time is measured in seconds since the gallery opened
    if (strcmp(query[1], "-T") != 0) 
    {
        cout << "Error - Invalid Query: Missing Timestamp" << endl;
        return false;
    }

    // -K <token>
    if (strcmp(query[3], "-K") != 0)
    {
        cout << "Error - Invalid Query: Missing Token" << endl;
        return false;
    }

    // -A | -L arrival or left event
    if (strcmp(query[5], "-A") != 0 && strcmp(query[5], "-L") != 0)
    {
        cout << "Error - Invalid Query: Missing Event" << endl;
        return false;
    }

    // -E <employee> | -G <guest>
    if (strcmp(query[6], "-E") != 0 && strcmp(query[6], "-G") != 0)
    {
        cout << "Error - Invalid Query: Missing Guest/Employee" << endl;
        return false;
    }


    // -R <room-id> room-id is an int
    if (strcmp(query[8], "-R") != 0)
    {
        cout << "Error - Invalid Query: Missing Room" << endl;
        return false;
    }

    /*
    // Check that token matches log file token
    if (CheckToken(query[10], sha256(query[4])) == false)
    {
        cout << "Error - Invalid Query: Incorrect Token" << endl;
        return false;
    }
    */
    return true;
}

// check if query token matches log file token
// hash token, compare against first line of log file
bool CheckToken(string logName, string k) 
{
    ifstream file;
    file.open(logName + ".txt");
    string token;
    if(getline(file, token))
    {
        if(strcmp(token.c_str(), k.c_str()) != 0)
        {
            cout << "Error: Token does not match log file token" << endl;
            cout << "Log Token: " << token << endl;
            cout << "Provided Token: " << k << endl;
            file.close();
            return false;
        }
    }
    return true;
}

bool CheckDBToken(string dbname, string k)
{
    bool valid = false;
    std::cout << "check token" << std::endl;
    sqlite3 *db;
    sqlite3_stmt *stmt;
    char *errMsg = 0;
    int rc;
    string tk;

    rc = sqlite3_open((dbname + ".db").c_str(), &db);
    const char *sql = "SELECT TOKEN FROM LOGFILE WHERE ID = 0;";
    
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        std::cerr << "Failed to prepare statement: " << sqlite3_errmsg(db) << std::endl;
        sqlite3_close(db);
        return 1;
    }

    if (sqlite3_step(stmt) == SQLITE_ROW) {
        const char *token = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
        std::cout << "token: " << token << std::endl;
        tk = token;
    } else {
        std::cerr << "No rows found or error executing query" << std::endl;
    }
    

    if(strcmp(tk.c_str(), k.c_str()) != 0)
    {
        cout << "Error: Token does not match log file token" << endl;
        cout << "Log Token: " << tk << endl;
        cout << "Provided Token: " << k << endl;
        valid = false;
    }else
    {
        std::cout << "they same :)" << std::endl;
        cout << "Log Token: " << tk << endl;
        cout << "Provided Token: " << k << endl;
        valid = true;
    }
    
    sqlite3_finalize(stmt);
    sqlite3_close(db);
    return valid;
}

// check if log file specified exists, if not, create log file and assign token
bool CheckLogExists(string fname) 
{
    ifstream file;
    file.open(fname + ".txt");
    if (!file)
    {
        cout << "Log file does not exist, creating new log file..." << fname << endl;
        return false;
    }
    else 
    {
        cout << "Log file exists: " << fname << endl;
        file.close();
        return true;
    }
}

// check if database exists
bool CheckDBExists(string dbname)
{
    return std::filesystem::exists(dbname + ".db");
}

// create log file with specified token
// set hash as first line of log
void CreateLog(string fname, string token) 
{
    ofstream file;
    file.open(fname + ".txt");
    if (!file)
    {
        cout << "Error: Could not create log file " << fname << endl;
    }
    else 
    {
        file << token << endl;
        cout << "Log file created: " << fname << endl;
        file.close();
    }
}


void CreateDB(string dbname, string token)
{
    sqlite3 *db;
    char *errMsg = 0;
    int rc;

    rc = sqlite3_open((dbname + ".db").c_str(), &db);

    if( rc ) {
        fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));
    } else {
        fprintf(stderr, "Opened database successfully\n");
    }

    string sql = "CREATE TABLE LOGFILE("  \
      "ID INT PRIMARY KEY," \
      "TOKEN CHAR(128)," \
      "TIME           INT," \
      "NAME            CHAR(50)," \
      "EG        CHAR(10)," \
      "AL       CHAR(10)," \
      "ROOMID          INT);";

    rc = sqlite3_exec(db, sql.c_str(), callbackC, 0, &errMsg);
   
    sql = "INSERT INTO LOGFILE (ID, TOKEN, TIME, NAME, EG, AL, ROOMID) "
                "VALUES(0,'"+token+"','','','','','')";

    rc = sqlite3_exec(db, sql.c_str(), callbackC, 0, &errMsg);

    if( rc != SQLITE_OK ){
        fprintf(stderr, "SQL error: %s\n", errMsg);
        sqlite3_free(errMsg);
    } else {
        fprintf(stdout, "Table created successfully\n");
    }

    sqlite3_close(db);
}

string sha256(const string inputStr)
{
    unsigned char hash[SHA256_DIGEST_LENGTH];
    const unsigned char* data = (const unsigned char*)inputStr.c_str();
    SHA256(data, inputStr.size(), hash);
    stringstream ss;
    for (int i = 0; i < SHA256_DIGEST_LENGTH; i++)
    {
        ss << hex << setw(2) << setfill('0') << (int)hash[i];
    }
    return ss.str();
}

void PrintQuery(int argc, char* query[])
{
    //print query to console
    int i = 0;
    while (i < argc) {
        cout << "Argument " << i  
             << ": " << query[i]
             << endl;
        i++;
    }
}
