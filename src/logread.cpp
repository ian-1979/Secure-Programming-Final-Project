#include <iostream>
#include <fstream>
#include <map>
#include <vector> 
#include <algorithm>
#include <set> 
#include <string.h>
#include <cstdlib>
#include <sstream>
using namespace std;

// Example Queries : 
// logread -K <token> -S <log>
// logread -K <token> -R (-E <name> | -G <name>) <log>

// Optional Queries : 
// logread -K <token> -T (-E <name> | -G <name>) <log>
// logread -K <token> -I (-E <name> | -G <name>) [(-E <name> | -G <name>) ...] <log>

struct PersonState { 
    bool inGallery = false; 
    int entryTime = 0;  
    int totalTimeSpent = 0; 
    map<int, bool> inRoom; // roomID -> true/false 
    vector<int> roomsVisited; 
}; 

string token;
string logFile; 
string command; // ex : -k, -t, -s, -i, etc 
vector<pair<string, bool>> targets; // pair<name, isEmployee>

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

bool checkToken(const string& fileName, const string& currentToken){
    ifstream f(fileName);
    if(!f.is_open()) printInvalid(); 

    string storedToken; 
    if(!getline(f, storedToken)) invalidTok(); 
    if(storedToken != currentToken) invalidTok(); 

    return true; 
}

// Parse Log 
void parseLog(const string& fileName){
    ifstream f(fileName); 
    if(!f.is_open()) printInvalid(); 

    string line; 
    getline(f, line); 

    while(getline(f,line)){
        if(line.empty()) continue; 

        vector<string> argument; 
        size_t start = 0, end; 
        istringstream iss(line);
        string word; 

        while(iss >> word){
            argument.push_back(word); 
        }
        
        argument.push_back(line.substr(start));
        if(argument.size() != 5) invalidTok(); 

        string action = argument[0];
        string role = argument[1]; 
        string name = argument[2]; 
        int roomID = stoi(argument[3]); 
        int time = stoi(argument[4]); 

        map<string, PersonState>& group = (role == "E" ? employees : guests); 
        PersonState& p = group[name]; 

        if(action == "A"){
            if(roomID == 0){
                p.inGallery = true; 
                p.entryTime = time; 
            } else {
                p.inRoom[roomID] = true; 
                p.roomsVisited.push_back(roomID);
            }
        } else if(action == "L"){
            if(roomID == 0){
                p.inGallery = false; 
                if(p.entryTime > 0){
                    p.totalTimeSpent += (time - p.entryTime);
                    p.entryTime = 0; 
                } else {
                    p.inRoom[roomID] = false; 
                }

            }
        }
        
    }
}

// Employee/Guest Search
void showState(){
    vector<string> employeeNames, guestNames; 
    for(auto& [name, p] : employees){
        if(p.inGallery){
            employeeNames.push_back(name);
        }
    } 

    for(auto& [name, p] : guests){
        if(p.inGallery){
            guestNames.push_back(name);
        }
    } 

    sort(employeeNames.begin(), employeeNames.end());
    sort(guestNames.begin(), guestNames.end()); 

    for(int i = 0; i < employeeNames.size(); ++i){
        if(i > 0){
            cout << ", ";
        }
        cout << employeeNames[i];
    }
    cout << "\n"; 

    for(int i = 0; i < guestNames.size(); ++i){
        if(i > 0){
            cout << ", ";
        }
        cout << guestNames[i];
    }
    cout << "\n"; 

    map<int, vector<string>> rooms; 
    for(auto& [name, p] : employees){
        for(auto& [r, inside] : p.inRoom){
            if(inside){
                rooms[r].push_back(name); 
            }
        }
    }

    for(auto& [name, p] : guests){
        for(auto& [r, inside] : p.inRoom){
            if(inside){
                rooms[r].push_back(name); 
            }
        }
    }

    for(auto& [room, names] : rooms){
        sort(names.begin(), names.end());
        cout << room << ": ";
        for(int i = 0; i < names.size(); ++i){
            if(i > 0){
                cout << ", ";
                cout << names[i];
            }
        }
        cout << "\n";
    }
}

// Room Status
void showRooms(const string& name, bool isEmployee){
    auto& group = (isEmployee ? employees : guests); 
    auto it = group.find(name);
    if(it == group.end()){
        return; 
    }

    auto& rooms = it->second.roomsVisited; 
    for(int i = 0; i < rooms.size(); ++i){
        if(i > 0){
            cout << ", ";
        }
        cout << rooms[i]; 
    }
    cout << "\n"; 
}

// Parse Query
int main(int argc, char* argv[]){
   if(argc < 4){
        printInvalid(); 
   }

   bool showS = false, showR = false; 
   bool isEmployee = false; 
   string name; 

   for(int i = 0; i < argc; ++i){
        string arg = argv[i]; 
        if(arg == "-K"){
            if(i + 1 >= argc) printInvalid(); 
            token = argv[++i];
        } else if(arg == "-S"){
            showS = true; 
        } else if(arg == "-R"){
            showR = true; 
        } else if(arg == "-E" || arg == "-G"){
            isEmployee = (arg == "-E");
            if(i + 1 >= argc){
                printInvalid();
            }
            name = argv[++i];
        } else if(arg[0] != '-'){
            logFile = arg; 
        }
   }

    if(token.empty() || logFile.empty()) printInvalid(); 
    if(!showS && !showR) printInvalid(); 

    checkToken(logFile, token);
    parseLog(logFile);

    if(showS) showState(); 
    if(showR) showRooms(name, isEmployee); 
}
// Employee/Guest Path