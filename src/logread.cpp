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
#include <sqlite3.h>
#include <openssl/sha.h>
#include <openssl/aes.h>
#include <openssl/rand.h>
#include <openssl/evp.h>
#include <openssl/bio.h>
#include <openssl/buffer.h>
#include "logread.hpp"

using namespace std;

// global variables for command-line arguments
string token; 
string logFile; // database file name 
int logTime = 0;  // tracker for time 
bool showS = false, showR = false, showT = false; // flags for -S, -R, -T
bool isEmployee = false; // flag for if person in gallery is employee or guests


// state of employees and guests in the gallery
map<string, PersonState> employees;
map<string, PersonState> guests;

// ERROR HANDLING 
void printInvalid(){
    throw runtime_error("INVALID! The information provided is an invalid query or input not found in database."); // invalid input or query
}

void invalidTok(){
    throw runtime_error("INTEGRITY VIOLATION! Token provided is NOT in database, exiting now."); // invalid token, throws an integrity violation
}

string sha256(const string inputStr){
    unsigned char hash[SHA256_DIGEST_LENGTH]; // 32-byte buffer for hash
    const unsigned char *data = (const unsigned char *)inputStr.c_str(); // input as bytes
    SHA256(data, inputStr.size(), hash);

    // convert binary hash to a hex string
    stringstream ss;
    for (int i = 0; i < SHA256_DIGEST_LENGTH; i++){
        ss << hex << setw(2) << setfill('0') << (int)hash[i];
    }
    return ss.str(); // return hash as string
}

string AESDecryptDB(string ctxt, string token){
    string ptxt = "";
    
    // key and iv initialization
    unsigned char key[32];
    unsigned char iv[AES_BLOCK_SIZE];
    memcpy(key, token.substr(0, 32).c_str(), 32);
    memcpy(iv, token.substr(32, 16).c_str(), AES_BLOCK_SIZE);

    // Base64 decode the ciphertext
    BIO *bio, *b64;
    unsigned char decoded_ctxt[128];
    int decoded_len;

    b64 = BIO_new(BIO_f_base64());
    bio = BIO_new_mem_buf(ctxt.c_str(), ctxt.length());
    bio = BIO_push(b64, bio);
    decoded_len = BIO_read(bio, decoded_ctxt, sizeof(decoded_ctxt));
    BIO_free_all(bio);

    // AES decryption using EVP_DecryptInit_ex
    EVP_CIPHER_CTX *ctx = EVP_CIPHER_CTX_new();
    EVP_DecryptInit_ex(ctx, EVP_aes_256_cbc(), NULL, key, iv);
    
    unsigned char plaintext[128];
    int len;
    EVP_DecryptUpdate(ctx, plaintext, &len, decoded_ctxt, decoded_len);
    int plaintext_len = len;

    EVP_DecryptFinal_ex(ctx, plaintext + len, &len);
    plaintext_len += len;

    ptxt = string((char*)plaintext, plaintext_len);

    EVP_CIPHER_CTX_free(ctx);
    return ptxt;    
}

// verify if the user token is stored in DB
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
        invalidTok(); // no token found
    }

    sqlite3_finalize(stmt);

    // hash user input and compare
    string givenHash = sha256(currentToken);
    if (strcmp(storedHash.c_str(), givenHash.c_str()) != 0){
        invalidTok(); // token does not match!
    }
    return true;
}

// Parse Log and Update Gallery States
void parseLog(sqlite3 *db, string token){
    const char *sql =
        "SELECT TIME, NAME, EG, AL, ROOMID "
        "FROM LOGFILE "
        "WHERE ID != 0 "
        "ORDER BY CAST(TIME AS INTEGER) ASC";
    
    sqlite3_stmt* stmt; 
    if(sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK){
        printInvalid(); 
    }

while(sqlite3_step(stmt) == SQLITE_ROW){
    try {
        // decrypt database 
        const unsigned char* tCol = sqlite3_column_text(stmt, 0);
        const unsigned char* nCol = sqlite3_column_text(stmt, 1);
        const unsigned char* rCol = sqlite3_column_text(stmt, 2);
        const unsigned char* aCol = sqlite3_column_text(stmt, 3);
        const unsigned char* rmCol = sqlite3_column_text(stmt, 4);

        // Skip if any column is NULL or empty
        if (!tCol || !nCol || !rCol || !aCol || !rmCol) {
            continue; // malformed row
        }

        string timeStr = AESDecryptDB((const char*)tCol, token);
        string name    = AESDecryptDB((const char*)nCol, token);
        string role    = AESDecryptDB((const char*)rCol, token);
        string action  = AESDecryptDB((const char*)aCol, token);
        string roomStr = AESDecryptDB((const char*)rmCol, token);

        // stoi may throw → this try/catch will capture it
        int time = stoi(timeStr);
        int roomID = stoi(roomStr);

        if(time > logTime) logTime = time;

        // remove leading '-' if present
        if(!role.empty() && role[0] == '-') role = role.substr(1);
        if(!action.empty() && action[0] == '-') action = action.substr(1);

        // determine guest or employee
        map<string, PersonState>& group =
            (strcmp(role.c_str(), "E") == 0) ? employees : guests;
        PersonState& p = group[name];

        // ARRIVED
        if(action == "A") {
            p.inRoom[roomID] = true;

            if(!p.inGallery){
                p.inGallery = true;
                p.entryTime = time;
            }

            if (find(p.roomsVisited.begin(), p.roomsVisited.end(), roomID) == p.roomsVisited.end()){
                p.roomsVisited.push_back(roomID);
            }
        }
        // LEFT
        else if(action == "L") {
            // cout << "DEBUG - " << name << " leaving room " << roomID << endl;
            // cout << "DEBUG - Before erase: ";
            // for (auto& [r, inside] : p.inRoom) {
            //     cout << "Room " << r << ": " << inside << ", ";
            // }
            // cout << endl;
            
            p.inRoom.erase(roomID);
            
            // cout << "DEBUG - After erase: ";
            // for (auto& [r, inside] : p.inRoom) {
            //     cout << "Room " << r << ": " << inside << ", ";
            // }
            bool inAnyRoom = false;
            for (const auto &roomStatus : p.inRoom) {
                if (roomStatus.second) { // if value is 'true'
                    inAnyRoom = true;
                    break;
                }
            }

            if (!inAnyRoom) {
                if (p.inGallery && p.entryTime > 0) {
                    p.totalTimeSpent += (time - p.entryTime);
                    p.entryTime = 0;
                }

                p.inGallery = false;
                // p.inRoom.clear();            

            }
        }
                // Inside the try block, after parsing:
        // cout << "DEBUG - Processing: time=" << time << ", name=" << name 
        //     << ", action=" << action << ", roomID=" << roomID << endl;

    } catch (...) {
        // If ANYTHING goes wrong, skip the row
        continue;
    }
}


    sqlite3_finalize(stmt); 
}

// shows current state of Gallery
void showState(){
    vector<string> employeeNames, guestNames;
    // gather employees
    for (auto &[name, p] : employees){
        if (!p.inGallery) continue;
        if (p.inGallery){
            employeeNames.push_back(name);
        }
    }
    // gather guests
    for (auto &[name, p] : guests){
        if (!p.inGallery) continue;
        if (p.inGallery){
            guestNames.push_back(name);
        }
    }

    // sorting names alphabetically
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

    
    // room employees are in
    map<int, vector<string>> rooms;
    for (auto &[name, p] : employees){
        if(p.inGallery){
            for (auto &[r, inside] : p.inRoom){
                if (inside){
                    rooms[r].push_back(name);
                }
            }
        }
    }
    // room guests are in
    for (auto &[name, p] : guests){
        if(p.inGallery){
        for (auto &roomEntry : p.inRoom){
                    if (roomEntry.second){ // if inside is true
                        rooms[roomEntry.first].push_back(name);
                    }
                }
        }
    }

    // what exact room people are in, printing
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

// all rooms visited by someone
void showRooms(const string &name, bool isEmployee){
    auto &group = (isEmployee ? employees : guests);
    auto it = group.find(name);

    if (it == group.end()){
        cout << "PERSON NOT FOUND, TRY AGAIN." << endl; 
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

// total time spent in gallery by a person, 
// person has to leave room for total time to be calculated
void showTime(const string &name, bool isEmployee){
    auto &group = (isEmployee ? employees : guests);
    auto it = group.find(name);

    if(it == group.end()){
        cout << "PERSON NOT FOUND, TRY AGAIN." << endl; 
        return;
    }

    PersonState &p = it -> second; 
    int timeSpent = p.totalTimeSpent;

    // if currently in gallery, add ongoing time 
    if(p.inGallery && p.entryTime > 0){
        timeSpent += (logTime - p.entryTime);
    }

    cout << timeSpent << "\n"; 
}


