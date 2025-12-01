#ifndef LOGREAD_HPP
#define LOGREAD_HPP

#include <string>
#include <map>
#include <vector>
#include <sqlite3.h>

// ---------------------------
// Data Structures
// ---------------------------
struct PersonState {
    bool inGallery = false;
    int entryTime = 0;
    int totalTimeSpent = 0;
    std::map<int, bool> inRoom;      // roomID -> true/false
    std::vector<int> roomsVisited;   // list of rooms visited
};

// ---------------------------
// Global Variables
// ---------------------------
extern std::string token;
extern std::string logFile;
extern int logTime;
extern bool showS;
extern bool showR;
extern bool showT;
extern bool isEmployee;

extern std::map<std::string, PersonState> employees;
extern std::map<std::string, PersonState> guests;

// ---------------------------
// Function Declarations
// ---------------------------

// security / cryptography
std::string sha256(const std::string inputStr);
std::string AESDecryptDB(const std::string ctxt, const std::string token);

// authentication / authorization
bool checkToken(sqlite3 *db, const std::string &currentToken);

// log parsing and analyzing rooms
void parseLog(sqlite3 *db, std::string token);
void showState();
void showRooms(const std::string &name, bool isEmployee);
void showTime(const std::string &name, bool isEmployee);

// error handling
void printInvalid();
void invalidTok();

#endif // LOGREAD_HPP
