#include <iostream>
#include <vector>
#include <string>
#include <sstream>
using namespace std;

// GCD function
int gcd(int a, int b) {
    return b == 0 ? a : gcd(b, a % b);
}

// modular inverse -> simp. not exactly extended euclidean algorithm 
// used to find the private key for rsa algorithm
int modInverse(int e, int phi) {
    for (int d = 1; d < phi; d++) { // trying to find what d satisfies 
        if ((e * d) % phi == 1) // e * d mod phi = 1 
            return d; 
    }
    return -1; // no inverse found
}

// modular exponentiation
int modExp(int base, int exp, int mod) {
    int result = 1;
    base %= mod; // reduce base by mod n 
    while (exp > 0) {
        if (exp % 2 == 1) // if the exp. bit is 1, mult. result by base 
            result = (result * base) % mod;
        exp >>= 1; // divide the exp. by 2 
        base = (base * base) % mod; // square the base and take mod n 
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
    int phi = (p - 1)  * (q - 1);
    int e = 3; // reasonable odd number to start 

    // find smallest e that satisfies gcd(e,phi) = 1 
    while (gcd(e, phi) != 1) e++;
    int d = modInverse(e, phi);
    return {e, d, n}; // returns the public and private key pair 
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
    cout << "Bob's Public Key (e, n): (" << bob.e << ", " << bob.n << ")\n";
    cout << "Bob's Private Key (d, n): (" << bob.d << ", " << bob.n << ")\n";

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
