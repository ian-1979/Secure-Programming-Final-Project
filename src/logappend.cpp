
#include <iostream>
#include <fstream>
#include <string>
#include <cstring>
#include <iomanip>
#include <sstream>
#include <sqlite3.h>
#include <filesystem>
#include <openssl/sha.h>
#include <openssl/aes.h>
#include <openssl/rand.h>
#include <openssl/evp.h>
#include <openssl/bio.h>
#include <openssl/buffer.h>


using namespace std;

//Build Commands:
// g++ -o logappend logappend.cpp -lssl -lcrypto -lsqlite3
// Usage:
// ./logappend <query>

//  Query Format:
// ./logappend -T <timestamp> -K <token> (-E <employee-name> | -G <guest-name>) (-A | -L) [-R <room-id>] <log>
// ./logappend -B <file>

// Example Query:
// ./logappend -T 5 -K secret -A -E Fred -R 1 log1


//Function Prototypes

// Query Functions
void ProcessBatchFileDB(string); 
bool ParseQuery(int, char**);
void ProcessQuery(int, char**);

// DB Functions
void CreateDB(string, string);
bool CheckDBExists(string);
bool CheckDBToken(string, string);
string sha256(const std::string);
string AESEncryptDB(string, string);

// Debugging Function
void PrintQuery(int, char**);



int main(int argc, char* argv[]) 
{   
    cout << "Processing query..." << endl;

    //check if query is batch file
    if(strcmp(argv[1], "-B") == 0)
    {
        ProcessBatchFileDB(argv[2]);
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
            }
            else
            // log files exists, check if token is valid
            {
                if (CheckDBToken(argv[10], sha256(argv[4])) == false)
                {
                    cout << "Error: Invalid Token" << endl;
                }else
                {
                    ProcessQuery(argc, argv);
                    cout << "Query Processed Successfully" << endl;
                    cout << "Database "+string(argv[10])+ ".db Updated" << endl;
                }
            }
        }
    } else 
    {
        cout << "Error: Invalid Query" << endl;
    }
} 

void ProcessBatchFileDB(string batchName)
{
    // -B <file>
    // batch file, contains list of commands without logappend beginning
    // -B cannot appear within the batch file
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
            if (CheckDBExists(args[10]) == false)
            {
                CreateDB(args[10], sha256(args[4]));
                ProcessQuery(argc, args);
            }
            else
            {
                // log files exists, check if token is valid
                if (CheckDBToken(args[10], sha256(args[4])) == false)
                {
                    cout << "Error: Invalid Token" << endl;
                }else
                {
                    ProcessQuery(argc, args);
                }
            }
        }
    }
}

// Callback to print query results - for debugging
int callbackB(void *data, int count, char **argv, char **columnNames){
   int i;
   for(i = 0; i<count; i++) {
      printf("Col: %s = %s\n", columnNames[i], argv[i]);
   }
   printf("\n");
   return 0;
}

// Callback to get count of entries in db
static int callbackC(void *count, int argc, char **argv, char **azColName) {
    int *c = (int*)count;
    *c = atoi(argv[0]);
    return 0;
}

// Process query and add it to db
void ProcessQuery(int argc, char* query[])
{
    //SQL variables
    sqlite3 *db;
    char *errMsg = 0;
    int rc;
    int count = 0;

    //query variables
    string time = AESEncryptDB(query[2], sha256(query[4]));
    string token = sha256(query[4]);
    string AL = AESEncryptDB(query[5], sha256(query[4]));
    string name = AESEncryptDB(query[7], sha256(query[4])); 
    string EG = AESEncryptDB(query[6], sha256(query[4])); 
    string roomid = AESEncryptDB(query[9], sha256(query[4]));

    string dbname = string(query[10]) + ".db";

    rc = sqlite3_open(dbname.c_str(), &db);
    rc = sqlite3_exec(db, "select count(*) from LOGFILE", callbackC, &count, &errMsg);

    string sql = "INSERT INTO LOGFILE (ID, TOKEN, TIME, NAME, EG, AL, ROOMID) "
                "VALUES("+std::to_string(count)+",'"+token+"','"+time+"','"+name+"','"+EG+"','"+AL+"','"+roomid+"')";
    
    rc = sqlite3_exec(db, sql.c_str(), callbackC, 0, &errMsg);
}

// Encrypt ptext with AES using token as key
string AESEncryptDB(string ptxt, string token)
{
    string ctxt = "";
    
    // key and iv initialization
    unsigned char key[32];
    unsigned char iv[AES_BLOCK_SIZE];
    memcpy(key, token.substr(0, 32).c_str(), 32);
    memcpy(iv, token.substr(32, 16).c_str(), AES_BLOCK_SIZE);

    // AES encryption using EVP_EncryptInit_ex
    EVP_CIPHER_CTX *ctx = EVP_CIPHER_CTX_new();
    EVP_EncryptInit_ex(ctx, EVP_aes_256_cbc(), NULL, key, iv);
    
    unsigned char ciphertext[128];
    int len;
    EVP_EncryptUpdate(ctx, ciphertext, &len, (unsigned char*)ptxt.c_str(), ptxt.length());
    int ciphertext_len = len;

    EVP_EncryptFinal_ex(ctx, ciphertext + len, &len);
    ciphertext_len += len;

    // Base64 encode the ciphertext
    BIO *bio, *b64;
    BUF_MEM *bptr;
    b64 = BIO_new(BIO_f_base64());
    bio = BIO_new(BIO_s_mem());
    bio = BIO_push(b64, bio);
    BIO_write(bio, ciphertext, ciphertext_len);
    BIO_flush(bio);
    BIO_get_mem_ptr(bio, &bptr);
    ctxt = string(bptr->data, bptr->length);
    BIO_free_all(bio);

    EVP_CIPHER_CTX_free(ctx);
    return ctxt;    
}

// Make sure that query is valid before writing to log file
bool ParseQuery(int argc, char* query[]){
    bool valid = true;

    // Input Sanitization
    //check for invalid characters such as ' " ; ( ) * ^ ! [ ]
    string bannedChar = "\"\'();*&^![]=";

    for(int a = 0; a < bannedChar.length(); a++)
    {
        for(int b = 0; b < argc; b++)
        {
            if(string(query[b]).find(bannedChar[a]) != std::string::npos)
            {
                cout << "Error - Invalid Query: Invalid Character In Query" << endl;
            }
        }
    }

    // Validate Query Structure
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
    try 
    {
        int roomID = stoi(string(query[9]));
        if (roomID < 0)
        {
            cout << "Error - Invalid Query: Room ID Cannot Be Negative" << endl;
            return false;
        }
    }
    catch (invalid_argument&)
    {
        cout << "Error - Invalid Query: Room ID Must Be An Integer" << endl;
        return false;
    }

    return true;
}

// Checks user token against token in db
bool CheckDBToken(string dbname, string k)
{
    bool valid = false;
    sqlite3 *db;
    sqlite3_stmt *stmt;
    char *errMsg = 0;
    int rc;
    string tk;

    // Open database and prepare statement
    rc = sqlite3_open((dbname + ".db").c_str(), &db);
    const char *sql = "SELECT TOKEN FROM LOGFILE WHERE ID = 0;";
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        std::cerr << "Failed to prepare statement: " << sqlite3_errmsg(db) << std::endl;
        sqlite3_close(db);
        return 1;
    }

    // Execute statement and retrieve token
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        const char *token = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
        tk = token;
    } else {
        std::cerr << "No rows found or error executing query" << std::endl;
    }
    
    // Compare tokens
    if(strcmp(tk.c_str(), k.c_str()) != 0)
    {
        cout << "Error: Token does not match log file token" << endl;
        valid = false;
    }else
    {
        valid = true;
    }
    
    sqlite3_finalize(stmt);
    sqlite3_close(db);
    return valid;
}

// check if database exists
bool CheckDBExists(string dbname)
{
    return std::filesystem::exists(dbname + ".db");
}

// Create database
void CreateDB(string dbname, string token)
{
    sqlite3 *db;
    char *errMsg = 0;
    int rc;

    // Open database
    rc = sqlite3_open((dbname + ".db").c_str(), &db);

    if( rc ) {
        fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));
    } else {
        fprintf(stderr, "Opened database successfully\n");
    }

    // Create SQL table
    string sql = "CREATE TABLE LOGFILE("  \
      "ID CHAR(64) PRIMARY KEY," \
      "TOKEN CHAR(128)," \
      "TIME           CHAR(64)," \
      "NAME            CHAR(64)," \
      "EG        CHAR(64)," \
      "AL       CHAR(64)," \
      "ROOMID          CHAR(64));";

    // Execute SQL statement
    rc = sqlite3_exec(db, sql.c_str(), callbackC, 0, &errMsg);
   
    // Insert hashed token as first entry
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

// SHA256 Hashing function
// Used to store the token as a hash in the database
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
