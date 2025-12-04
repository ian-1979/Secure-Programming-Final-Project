# Secure Programming Final Project - Phase 3
## Group #5: Adrion Thomas and Ian Scheetz

## Libraries Used 

- OpenSSL
- SQLite3

## Build Instructions 

### Logappend

Build Commands:\
`g++ -o logappend logappend.cpp -lssl -lcrypto -lsqlite3`\
`./logappend <query>`

Query Format:\
`./logappend -T <timestamp> -K <token> (-E <employee-name> | -G <guest-name>) (-A | -L) [-R <room-id>] <log>`\
`./logappend -B <file>`

Example Queries:\
`./logappend -T 5 -K secret -A -E Fred -R 1 log1`\
`./logappend -T 10 -K secret -L -G Alice -R 1 log1`

### Logread

Build Commands:\
`g++ -o logread logread_main.cpp -lssl -lcrypto -lsqlite3`\
`./logread <query>`

Query Format:\
`./logread -K <token> -S <log>`\
`./logread -K <token> -R (-E <name> | -G <name>) <log>`

Example Queries:\
`./logread -K secret -S log1`\
`./logread -K secret -R -E Fred log1`

## Security Considerations

1. Input Validation and Sanitization - Both the logappend.cpp and logread.cpp include functions to ensure the user input matches the expected structure of input and does not contain any harmful strings. The ParseQuery function in logread validates and sanitizes all queries. If invalid characters or structure is detected an error is thrown.
2. Output Encoding - Before writing data to the database file, all data is encrypted using AES and encoded using bas64 to avoid corruption of the data.  
3. Parameterized Queries - All SQL queries are carefully constructed and do not use any user input.
4. Error Handling - Throughout both scripts there are serveral points in which input is checked for errors.
5. Memory Management - Sensitive data is not hardcoded and all arrays have proper bounds.
6. Avoid Hardcoding - Neither the key nor the IV (initialization vector) used during AES encryption are hardcoded. Both are determined by the hashed security token.
7. Cryptography - Hashing and symmetric key encryption were used to ensure the confidentiality of the data. The algorithms used were SHA256 and AES encryption. The OpenSSL library was used to implement these algorithms because it is a well established and reliable encryption library. SHA256 is a hash function used to conver the security tokens into hashes. AES encryption is used to encrypt the data before writing to the database. The user must input the same token used to encrypt the data in order for logread to decrypt the data.
8. Authentication - When a database file is created, an empty entry is created in the table only containing the hashed security token. In order to write or read from this database file, the user must input a security token. This token is ran through the SHA256 algorithm and if the hash matches the hash stored in the database the user query is accepted and data is written or read from the database.

## Logappend Documentation

### Query Functions:
- void ProcessBatchFileDB(string): Loops through a text file of queries and processes them into the database.
- bool ParseQuery(int, char**): Ensures that the provided query matches the expected structure of a query and does not contain any forbidden characters. Returns false if the query violates any rules.
- void ProcessQuery(int, char**): Encrypts the data give in the query and writes to the database file.

### Database Functions:
- void CreateDB(string, string): Creates the .db file and appends an entry containing the hashed security token.
- bool CheckDBExists(string): Returns true if the specified .db file already exists.
- bool CheckDBToken(string, string): Returns true if the hashed user token matches the token in the specified .db file.
- string sha256(const std::string): Hash function used to encrypt the token.
- string AESEncryptDB(string, string): AES implementation that is used to encrypt any string using the specified token. It uses the hash of the token as the key.


## Logread Documentation

## Key Functions:
bool checkToken(sqlite3* db, const string& currentToken): Checks if the SHA256 hash of the user-provided token matches the stored hash in the database. 
void parseLog(sqlite3* db, string token): Reads all non-token entries from the database, decrypts the data, and updates the state of the employees and guests. Tracks arrival and leaving events, updates rooms each person is in or visited, and calculates total time in Gallery.
void showState() : Displays the current Gallery status which includes the current employees and guests inside and what rooms each person is in.
void showRooms(const string& name, bool isEmployee): Displays all rooms visited by a specific employee or guest. 
void showTime(const string& name, bool isEmployee): Displays the total time spent in the Gallery by a specified employee or guest (the person has to leave for the time to be calculated)

## AES Decryption
string AESDecryptDB(string ctxt, string token): Decrypts a Base64-encoded AES-256 ciphertext by using the key and IV derived from the hashed token. Is able to handle malformed or large inputs without crashing. 

## SHA256 Hashing 
string sha256(const string inputStr) : Takes an input string and will return it as a hexadecimal string instead. Will be used to ensure that the user-supplied token matches one stored in the database. 

## Utlity Functions: 
void printInvalid(): Throws an exception if user provides the wrong commands or if information user wants to provide is not already in database. 
void invalidToken() : Throws an exception if the user provides the wrong token. 


## Test Suite Instructions

### Logappend.cpp

Within the tests folder there are two batch files. These each contain various queries. Batch1.txt contains only valid queries. Batch2.txt contains two valid queries and seven invalid queries. Running batch2.txt will result in the creation of two .db files, log1.db and log2.db. Each db file will only contain one entry.

Command to run the batch files:\
`./logappend -B ../tests/batch1.txt`\
`./logappend -B ../tests/batch2.txt`\
\
Alternatively, each individual line of the batch files can be run by prepending `./logappend`\
Example:\
`./logappend -T 5 -K secret -A -G Alice -R 1 log1`

### Logread.cpp
Testing for Logread is within the tests folder under test_logread.cpp. 
1. Token Validation :
- There are two tests involving the token.. For the first one, if it is the correct token the test will pass. For the latter, if it is the invalid token, it throws an exception which passes the test as it delves with whether or not an exception was thrown. 
2. AES Decryption:
- There are two tests about how the AES decryption works on larger and malformed inputs. The malformed Base64 input should not crash the program along with an large input, if it doesn't crash the program, it passes. 
3. Log Parsing :
- Malformed log entries should be handled by the program without crashing.

# Command to run tests: 
`g++ -std=c++17 -g ../src/logread.cpp test_logread.cpp -o test_logread -lssl -lcrypto -lsqlite3`

# Run Tests: 
`./test_logread`
