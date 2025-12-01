#include <iostream>
#include <cassert>
#include <string>
#include "../src/logread.hpp" // your main functions

using namespace std;

// Build Commands: 
// MUST BE IN PROJECT ROOT TO RUN TESTS!
// g++ -std=c++17 -g test_logread.cpp -o test_logread -lssl -lcrypto -lsqlite3
// g++ -std=c++17 -I/opt/homebrew/opt/openssl@3/include \   
    // -L/opt/homebrew/opt/openssl@3/lib \
    // -g src/logread.cpp tests/test_logread.cpp -o test_logread \
    // -lssl -lcrypto -lsqlite3


// Helper function for printing test results
void printResult(const string &testName, bool passed) {
    cout << testName << ": " << (passed ? "PASS" : "FAIL") << endl;
}

int main() {
    // ----------------------------
    // Test 1: Token validation
    // ----------------------------
    {
        bool passed = false;
        try {
    sqlite3* db;
    sqlite3_open(":memory:", &db);

    // build table and insert an hashed token 
    std::string sqlCreate =
        "CREATE TABLE LOGFILE (ID INTEGER PRIMARY KEY, TOKEN TEXT);"
        "INSERT INTO LOGFILE (ID, TOKEN) VALUES (0, '" + sha256("testtoken") + "');";

        char* errMsg = nullptr;
        sqlite3_exec(db, sqlCreate.c_str(), nullptr, nullptr, &errMsg);

        // should return true for correct token 
        passed = checkToken(db, "testtoken"); // correct token
            sqlite3_close(db);
        } catch (...) {
            passed = false;
        }
        printResult("Token validation (correct token)", passed);
    }

    // ----------------------------
    // Test 2: Invalid token
    // ----------------------------
    {
        bool passed = false;
        try {
            sqlite3* db;
            sqlite3_open(":memory:", &db);
            std::string sqlCreate =
                "CREATE TABLE LOGFILE (ID INTEGER PRIMARY KEY, TOKEN TEXT);"
                "INSERT INTO LOGFILE (ID, TOKEN) VALUES (0, '" + sha256("testtoken") + "');";
            char* errMsg = nullptr;
            sqlite3_exec(db, sqlCreate.c_str(), nullptr, nullptr, &errMsg);

            checkToken(db, "wrongtoken"); // should fail
        } catch (...) {
            passed = true; // expected to throw/exit, which means the test passed 
        }
        printResult("Token validation (invalid token)", passed);
    }

    // ----------------------------
    // Test 3: AES decryption malformed input
    // ----------------------------
    {
        bool passed = false;
        try {
            string badCipher = "NotBase64!";
            string token = "01234567890123456789012345678901" "0123456789012345"; // 48 bytes
            string result = AESDecryptDB(badCipher, token); // should not crash
            passed = true;
        } catch (...) {
            passed = false;
        }
        printResult("AES decryption malformed input", passed);
    }

    // ----------------------------
    // Test 4: Parse log with fake data
    // ----------------------------
    {
        bool passed = false;
        try {
            sqlite3* db;
            sqlite3_open(":memory:", &db);
            const char* sqlCreate =
            // creates a table with missing or broken fields (like the time isnt what should be expected, should be a number)
                "CREATE TABLE LOGFILE (ID INTEGER PRIMARY KEY, TIME TEXT, NAME TEXT, EG TEXT, AL TEXT, ROOMID TEXT, TOKEN TEXT);"
                "INSERT INTO LOGFILE (ID, TIME, NAME, EG, AL, ROOMID) VALUES (1, 'bad', 'Alice', '-E', '-A', '1');";
            char* errMsg = nullptr;
            sqlite3_exec(db, sqlCreate, nullptr, nullptr, &errMsg);

            parseLog(db, sha256("testtoken")); // should handle bad data gracefully
            sqlite3_close(db);
            passed = true;
        } catch (...) {
            passed = false;
        }
        printResult("Parse log with malformed data", passed);
    }

    // ----------------------------
    // Test 5: Buffer overflow / large data
    // ----------------------------
    {
        bool passed = false;
        try {
            string largeCipher(100000, 'A'); // 100k chars of 'A'
            string token = "01234567890123456789012345678901" "0123456789012345";
            string result = AESDecryptDB(largeCipher, token); // should not crash
            passed = true;
        } catch (...) {
            passed = false;
        }
        printResult("AES decryption large input", passed);
    }

    return 0;
}
