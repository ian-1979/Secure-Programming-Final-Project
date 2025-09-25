
#include <iostream>
#include <fstream>
#include <string.h>
using namespace std;

// Example Queries:
// logappend -T <timestamp> -K <token> (-E <employee-name> | -G <guest-name>) (-A | -L) [-R <room-id>] <log>
// logappend -B <file>

void ProcessBatchFile(string);
void PrintQuery(int, char**);

// List of functions that need implemented (not in order):
// Parse Query
// Sanitize Input
// Read Batch File
// Write to Temp File
// Convert Temp to Final
// Get Timestamp
// Encrypt Log File
// Check Token Against Log File Name 



int main(int argc, char* argv[]) {

    PrintQuery(argc, argv);
    if(strcmp(argv[1],"-B") == 0)
    {
        ProcessBatchFile(argv[2]);
    } else if(strcmp(argv[1],"-T") == 0)
    {
        
    } else
    {
        //error: invalid input
    }

} 

void ProcessBatchFile(string batchName)
{
    cout << batchName << endl;
}

void ParseQuery(int argc, char* query[]){
    // -B <file>
    // batch file, contains list of commands without logappend beginning
    // -B cannot appear within the batch file
    
    // -T <timestamp>
    // time is measured in seconds since the gallery opened


    // -K <token>

    // -E <employee> | -G <guest>

    // -A | -L arrival or left event

    // -R <room-id> room-id is an int, if not room-id specified, then event is for whole gallery

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
