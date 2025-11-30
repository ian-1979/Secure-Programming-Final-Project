# Secure Programming Final Project - Phase 3

## Group Infromation 

Group #5: Adrion Thomas and Ian Scheetz

## Libraries Used 

- OpenSSL
- SQLite3

## Build Instructions 

- Logappend:

Build Commands:
g++ -o logappend logappend.cpp -lssl -lcrypto -lsqlite3
./logappend <query>

Query Format:
./logappend -T <timestamp> -K <token> (-E <employee-name> | -G <guest-name>) (-A | -L) [-R <room-id>] <log>
./logappend -B <file>

Example Queries:
./logappend -T 5 -K secret -A -E Fred -R 1 log1
./logappend -T 10 -K secret -L -G Alice -R 1 log1



- Logread:

Build Commands:
g++ -o logread logread.cpp -lssl -lcrypto -lsqlite3
./logread <query>

Query Format:
logread -K <token> -S <log>
logread -K <token> -R (-E <name> | -G <name>) <log>

Example Queries:
./logread -K secret -S log1
./logread -K secret -R -E Fred log1

## Logappend Documentation

Query Functions:
- void ProcessBatchFileDB(string): Loops through a text file of queries and processes them into the database.
- bool ParseQuery(int, char**): Ensures that the provided query matches the expected structure of a query and does not contain any forbidden characters. Returns false if the query violates any rules.
- void ProcessQuery(int, char**): Encrypts the data give in the query and writes to the database file.

Database Functions:
- void CreateDB(string, string): Creates the .db file and appends an entry containing the hashed security token.
- bool CheckDBExists(string): Returns true if the specified .db file already exists.
- bool CheckDBToken(string, string): Returns true if the hashed user token matches the token in the specified .db file.
- string sha256(const std::string): Hash function used to encrypt the token.
- string AESEncryptDB(string, string): AES implementation that is used to encrypt any string using the specified token. It uses the hash of the token as the key.


## Logread Documentation


## Test Suite Instructions

