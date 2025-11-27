#include <iostream>
#include <fstream>
#include <map>
#include <vector>
#include <algorithm>
#include <set>
#include <string.h>
#include <cstdlib>
#include <sstream>
#include <iomanip>
#include <openssl/sha.h>
#include <sqlite3.h>

using namespace std;

// Build Commands:
// g++ -o logread logread.cpp -lssl -lcrypto -lsqlite3
// Usage:
// ./logread <query>

// Example Queries :
// logread -K <token> -S <log>
// logread -K <token> -R (-E <name> | -G <name>) <log>

// Optional Queries :
// logread -K <token> -T (-E <name> | -G <name>) <log>
// logread -K <token> -I (-E <name> | -G <name>) [(-E <name> | -G <name>) ...] <log>

struct PersonState{
    bool inGallery = false;
    int entryTime = 0;
    int totalTimeSpent = 0;
    map<int, bool> inRoom; // roomID -> true/false
    vector<int> roomsVisited;
};

string token;
string logFile;
int logTime = 0; 
bool showS = false, showR = false, showT = false; 
bool isEmployee = false;


map<string, PersonState> employees;
map<string, PersonState> guests;

void printInvalid(){
    cout << "invalid" << endl;
    exit(255);
}

void invalidTok(){
    cout << "integrity violation" << endl;
    exit(255);
}

string sha256(const string inputStr){
    unsigned char hash[SHA256_DIGEST_LENGTH];
    const unsigned char *data = (const unsigned char *)inputStr.c_str();
    SHA256(data, inputStr.size(), hash);
    stringstream ss;
    for (int i = 0; i < SHA256_DIGEST_LENGTH; i++){
        ss << hex << setw(2) << setfill('0') << (int)hash[i];
    }
    return ss.str();
}

bool checkToken(sqlite3 *db, const string &currentToken){
    const char *sql = "SELECT TOKEN FROM LOGFILE WHERE ID = 0";
    sqlite3_stmt *stmt;

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK){
        printInvalid();
    }

    string storedHash;
    if (sqlite3_step(stmt) == SQLITE_ROW){
        const unsigned char* text = sqlite3_column_text(stmt, 0);
        if(text){
            storedHash = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 0));
        } else { // just in case theres no token here 
            storedHash = ""; 
        }
    }
    else{
        sqlite3_finalize(stmt);
        invalidTok(); // no row found
    }

    sqlite3_finalize(stmt);

    string givenHash = sha256(currentToken);

    if (storedHash != givenHash){
        invalidTok();
    }

    return true;
}

// Parse Log
void parseLog(sqlite3 *db){
    const char *sql =
        "SELECT TIME, NAME, EG, AL, ROOMID "
        "FROM LOGFILE "
        "WHERE ID != 0 "
        "ORDER BY TIME ASC";
    
    sqlite3_stmt* stmt; 
    if(sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK){
        printInvalid(); 
    }

    while(sqlite3_step(stmt) == SQLITE_ROW){
        int time = sqlite3_column_int(stmt, 0);
        if(time > logTime) logTime = time;
        string name = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
        string role = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2)); // employee or guest
        string action = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3)); // arrived or left 
        int roomID = sqlite3_column_int(stmt, 4);

        // for the -g, -e, -a, -l
        if(!role.empty() && role[0] == '-') role = role.substr(1);
        if(!action.empty() && action[0] == '-') action = action.substr(1);

        map<string, PersonState>& group = (role == "E" ? employees : guests);
        PersonState& p = group[name];

        if(action == "A"){
            if(roomID == 0){
                if(!p.inGallery){
                    p.inGallery = true; 
                    p.entryTime = time; 
                }
            } else {
                p.inRoom[roomID] = true; 
            }
            p.roomsVisited.push_back(roomID); 
        } else if(action == "L"){
            if(roomID == 0){
                if(p.inGallery && p.entryTime > 0){
                    p.totalTimeSpent += (time - p.entryTime);
                    p.entryTime = 0; 
                }
                p.inGallery = false; 
                for (auto& rr : p.inRoom) rr.second = false;
            } else {
                p.inRoom[roomID] = false; 
            }
        }
    }
    sqlite3_finalize(stmt); 
}

// Employee/Guest Search
void showState(){
    vector<string> employeeNames, guestNames;
    for (auto &[name, p] : employees){
        if (p.inGallery){
            employeeNames.push_back(name);
        }
    }

    for (auto &[name, p] : guests){
        if (p.inGallery){
            guestNames.push_back(name);
        }
    }

    sort(employeeNames.begin(), employeeNames.end());
    sort(guestNames.begin(), guestNames.end());

    cout << "Employees in Gallery : ";
    for (int i = 0; i < employeeNames.size(); ++i){
        if (i > 0){
            cout << ", ";
        }
        cout << employeeNames[i];
    }
    cout << "\n";

    cout << "Guests in gallery : ";  
    for (int i = 0; i < guestNames.size(); ++i){
        if (i > 0){
            cout << ", ";
        }
        cout << guestNames[i];
    }
    cout << "\n";

    map<int, vector<string>> rooms;
    for (auto &[name, p] : employees){
        for (auto &[r, inside] : p.inRoom){
            if (inside){
                rooms[r].push_back(name);
            }
        }
    }

    for (auto &[name, p] : guests){
        for (auto &[r, inside] : p.inRoom){
            if (inside){
                rooms[r].push_back(name);
            }
        }
    }

    for (auto &[room, names] : rooms){
        sort(names.begin(), names.end());
        cout << "Room " << room << ": ";
        for (int i = 0; i < names.size(); ++i){
            if (i > 0){
                cout << ", ";
            }
                cout << names[i];
        }
        cout << "\n";
    }
}

// Room Status
void showRooms(const string &name, bool isEmployee){
    auto &group = (isEmployee ? employees : guests);
    auto it = group.find(name);
    if (it == group.end()){
        return;
    }

    auto &rooms = it->second.roomsVisited;
    for (int i = 0; i < rooms.size(); ++i){
        if (i > 0){
            cout << ", ";
        }
        cout << rooms[i];
    }
    cout << "\n";
}

void showTime(const string &name, bool isEmployee){
    auto &group = (isEmployee ? employees : guests);
    auto it = group.find(name);
    if(it == group.end()){
        return;
    }

    PersonState &p = it -> second; 
    int timeSpent = p.totalTimeSpent;

    if(p.inGallery && p.entryTime > 0){
        timeSpent += (logTime - p.entryTime);
    }

    cout << timeSpent << "\n"; 
}

// Parse Query
int main(int argc, char *argv[]){
    if (argc < 4){
        std::cout << "not enough args" << std::endl;
        printInvalid();
    }

    bool showS = false, showR = false;
    bool isEmployee = false;
    string name;

    for (int i = 0; i < argc; ++i){
        string arg = argv[i];
        if (arg == "-K"){
            if (i + 1 >= argc){
                std::cout << "token missing" << std::endl;
                printInvalid();
            }
            token = argv[++i];
        }
        else if (arg == "-S"){
            showS = true;
        }
        else if (arg == "-R"){
            showR = true;
        }
        else if(arg == "-T"){
            showT = true; 
        }
        else if (arg == "-E" || arg == "-G"){
            isEmployee = (arg == "-E");
            if (i + 1 >= argc){
                printInvalid();
                std::cout << "name missing" << std::endl;
            }
            name = argv[++i];
        }
        else if (arg[0] != '-'){
            logFile = arg;
        }
    }

    if (token.empty() || logFile.empty()){
        printInvalid();
        std::cout << "log or token empty" << std::endl;
    }

    if (!showS && !showR && !showT){
        printInvalid();
        std::cout << "no action specified" << std::endl;
    }

    sqlite3* db;

    if (sqlite3_open(logFile.c_str(), &db) != SQLITE_OK){
        printInvalid();
    }

    checkToken(db, token);
    parseLog(db); 
    sqlite3_close(db); 
    
    if (showS)
        showState();
    if (showR)
        showRooms(name, isEmployee);
    if(showT)
        showTime(name, isEmployee); 
}
