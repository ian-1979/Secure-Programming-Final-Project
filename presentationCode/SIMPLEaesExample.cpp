#include <iostream>
#include <string>
#include <sstream>
#include <iomanip>
using namespace std;

// XOR encryption
string xorEncryptDecrypt(const string& data, const string& key) {
    string result = data; // copy plaintext into result 
    for (size_t i = 0; i < data.size(); ++i) {
        // xor each byte with the corresponding key byte 
        result[i] = data[i] ^ key[i % key.size()];
    }
    return result;
}

// Convert binary string to hex
string toHex(const string& data) {
    stringstream ss;
    ss << hex << setfill('0');
    for (unsigned char c : data) {
        // print 2 hex digits per byte (00-ff)
        ss << setw(2) << static_cast<int>(c);
    }
    return ss.str();
}

// Convert hex string back to binary
string fromHex(const string& hex) {
    string result;
    for (size_t i = 0; i < hex.length(); i += 2) {
        string byte = hex.substr(i, 2);
        char chr = static_cast<char>(stoi(byte, nullptr, 16));
        result += chr;
    }
    return result;
}

int main() {
    string cardNumber, expiryDate, cvv;
    string key = "mysecretkey";

    cout << "=== Enter Credit Card Info ===\n";
    cout << "Card Number: ";
    getline(cin, cardNumber);
    cout << "Expiry Date (MM/YY): ";
    getline(cin, expiryDate);
    cout << "CVV: ";
    getline(cin, cvv);

    // for simplicity, making into one plaintext line 
    string fullData = "Card Number:" + cardNumber + "|Exp:" + expiryDate + "|CVV:" + cvv;

    // Encrypt and encode
    string encrypted = xorEncryptDecrypt(fullData, key);
    string encryptedHex = toHex(encrypted);
    cout << "\n[Encrypted Data Stored]: " << encryptedHex << endl;

    // Decode and decrypt
    string decrypted = xorEncryptDecrypt(fromHex(encryptedHex), key);
    cout << "\n[Decrypted Data Retrieved]: " << decrypted << endl;

    return 0;
}
