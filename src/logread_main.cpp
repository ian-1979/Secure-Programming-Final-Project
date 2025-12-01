#include "logread.hpp"
#include <iostream>
#include <sqlite3.h>
using namespace std;

// Build Commands:
// g++ -std=c++17 -g logread_main.cpp logread.cpp -o logread -lssl -lcrypto -lsqlite3
// Parse Query

// Usage:
// ./logread <query>

// Example Queries :
// logread -K <token> -S <log>
// logread -K <token> -R (-E <name> | -G <name>) <log>

// Optional Queries :
// logread -K <token> -T (-E <name> | -G <name>) <log>
// logread -K <token> -I (-E <name> | -G <name>) [(-E <name> | -G <name>) ...] <log>

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
                std::cout << "THERE IS NO TOKEN!" << std::endl;
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
                std::cout << "NO NAME PROVIDED." << std::endl;
                printInvalid();
            }
            name = argv[++i];
        }
        else if (arg[0] != '-'){
            logFile = arg + ".db";
        }
    }

    if (token.empty() || logFile.empty()){
        std::cout << "LOG OR TOKEN IS EMPTY, NOT ALLOWED." << std::endl;
        printInvalid();
    }

    if (!showS && !showR && !showT){
        printInvalid();
        std::cout << "NO ACTION SPECIFIED." << std::endl;
    }

    sqlite3* db;

    if (sqlite3_open(logFile.c_str(), &db) != SQLITE_OK){
        printInvalid();
    }

    checkToken(db, token);
    parseLog(db, sha256(token)); 
    sqlite3_close(db); 
    
    if (showS)
        showState();
    if (showR)
        showRooms(name, isEmployee);
    if(showT)
        showTime(name, isEmployee); 
}