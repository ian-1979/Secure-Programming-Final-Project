#include <iostream>
#include <vector>
#include <string>
#include <sstream>
using namespace std;

// GCD function
int gcd(int a, int b) {
    return b == 0 ? a : gcd(b, a % b);
}

// Modular inverse (brute force)
int modInverse(int e, int phi) {
    for (int d = 1; d < phi; d++) {
        if ((e * d) % phi == 1)
            return d;
    }
    return -1;
}

// Modular exponentiation
int modExp(int base, int exp, int mod) {
    int result = 1;
    base %= mod;
    while (exp > 0) {
        if (exp % 2 == 1)
            result = (result * base) % mod;
        exp >>= 1;
        base = (base * base) % mod;
    }
    return result;
}

// RSA key pair
struct RSAKey {
    int e, d, n;
};

// Generate RSA keys from two primes
RSAKey generateKeys(int p, int q) {
    int n = p * q;
    int phi = (p - 1) * (q - 1);
    int e = 3;
    while (gcd(e, phi) != 1) e++;
    int d = modInverse(e, phi);
    return {e, d, n};
}

// Encrypt message using public key
vector<int> encrypt(const string& msg, int e, int n) {
    vector<int> encrypted;
    for (char c : msg) {
        encrypted.push_back(modExp((int)c, e, n));
    }
    return encrypted;
}

// Decrypt message using private key
string decrypt(const vector<int>& encrypted, int d, int n) {
    string msg;
    for (int c : encrypted) {
        msg += (char)modExp(c, d, n);
    }
    return msg;
}

int main() {
    // Simulate Alice and Bob generating their keys
    RSAKey alice = generateKeys(61, 53); // Alice's keys
    RSAKey bob   = generateKeys(47, 59); // Bob's keys

    // Alice writes an email to Bob
    string subject, body;
    cout << "=== Alice's Email Composer ===\n";
    cout << "Subject: ";
    getline(cin, subject);
    cout << "Body:\n";
    getline(cin, body);

    string fullMessage = "Subject: " + subject + "\nBody: " + body;

    // Encrypt with Bob's public key
    vector<int> encrypted = encrypt(fullMessage, bob.e, bob.n);
    cout << "\n[Encrypted Email Sent to Bob]\n";
    for (int c : encrypted) cout << c << " ";
    cout << "\n";

    // Bob decrypts the message
    string decrypted = decrypt(encrypted, bob.d, bob.n);
    cout << "\n=== Bob's Inbox ===\n";
    cout << decrypted << endl;

    return 0;
}
