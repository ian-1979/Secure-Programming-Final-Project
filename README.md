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
`g++ -o logread logread.cpp -lssl -lcrypto -lsqlite3`\
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


## Test Suite Instructions

