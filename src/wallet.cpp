#include "wallet.h"
#include "blockchain.h"
#include "crypto_utils.h"
#include "proof_generator.h"
#include <filesystem>
#include <fstream>
#include <iostream>

void Wallet::updateAddressFromPQKeys() {
    std::string derivedAddress;

    if (!dilithiumKeys.publicKey.empty()) {
        std::vector<unsigned char> pubDil(dilithiumKeys.publicKey.begin(),
                                          dilithiumKeys.publicKey.end());
        derivedAddress = Crypto::deriveAddressFromPub(pubDil);
    }

    if (derivedAddress.empty() && !falconKeys.publicKey.empty()) {
        std::vector<unsigned char> pubFal(falconKeys.publicKey.begin(),
                                          falconKeys.publicKey.end());
        derivedAddress = Crypto::deriveAddressFromPub(pubFal);
    }

    if (!derivedAddress.empty()) {
        address = derivedAddress;
    } else {
        address = walletName;
    }
}

namespace fs = std::filesystem;

// Default wallet constructor
// Use the main constructor with default passphrase to avoid ambiguity with the
// overload that accepts a private key path.
Wallet::Wallet() : Wallet("defaultWallet", KEY_DIR) {}

// Main constructor: create or load keys for an address
Wallet::Wallet(const std::string& address, const std::string& keyDirectoryPath, const std::string& passphrase)
    : keyDirectory(keyDirectoryPath), walletName(address), address(address) {
    Crypto::ensureKeysDirectory();

    std::string privPath = keyDirectory + address + "_private.pem";
    std::string pubPath  = keyDirectory + address + "_public.pem";

    // Generate keys if missing
    if (!fs::exists(privPath) || !fs::exists(pubPath)) {
        std::cout << "🔐 Generating RSA key pair for address: " << address << std::endl;
        if (!passphrase.empty())
            Crypto::generateKeysForUser(address, passphrase);
        else
            Crypto::generateKeysForUser(address);
    }

    if (!passphrase.empty())
        privateKey = Crypto::loadPrivateKeyDecrypted(privPath, passphrase);
    else
        privateKey = loadKeyFile(privPath);
    publicKey  = loadKeyFile(pubPath);

    // --- Dilithium ---
    dilithiumKeys = Crypto::loadDilithiumKeys(address);
    if (dilithiumKeys.privateKey.empty() || dilithiumKeys.publicKey.empty()) {
        std::cout << "⚠️ Missing Dilithium keys. Generating...\n";
        Crypto::generateDilithiumKeys(address);
        dilithiumKeys = Crypto::loadDilithiumKeys(address);
    }

    // --- Falcon ---
    falconKeys = Crypto::loadFalconKeys(address);
    if (falconKeys.privateKey.empty() || falconKeys.publicKey.empty()) {
        std::cout << "⚠️ Missing Falcon keys. Generating...\n";
        Crypto::generateFalconKeys(address);
        falconKeys = Crypto::loadFalconKeys(address);
    }

    updateAddressFromPQKeys();

    std::cout << "✅ Wallet created successfully!\nAddress: " << address
              << "\nKey Identifier: " << walletName << std::endl;
}

// Alternate constructor: load wallet from explicit private key path
Wallet::Wallet(const std::string& privateKeyPath, const std::string& keyDirectoryPath, const std::string& walletName, const std::string& passphrase)
    : keyDirectory(keyDirectoryPath), walletName(walletName), address(walletName) {
    // Load RSA private key
    if (!fs::exists(privateKeyPath)) {
        throw std::runtime_error("❌ Private key file not found: " + privateKeyPath);
    }
    if (!passphrase.empty())
        privateKey = Crypto::loadPrivateKeyDecrypted(privateKeyPath, passphrase);
    else
        privateKey = loadKeyFile(privateKeyPath);

    // Load RSA public key
    std::string publicKeyPath = privateKeyPath;
    size_t pos = publicKeyPath.find("_private.pem");
    if (pos != std::string::npos)
        publicKeyPath.replace(pos, 12, "_public.pem");

    if (!Crypto::ensureRsaPublicKey(walletName, privateKeyPath, publicKeyPath, passphrase)) {
        throw std::runtime_error("❌ Public key file not found: " + publicKeyPath);
    }
    publicKey = loadKeyFile(publicKeyPath);

    // --- Dilithium ---
    dilithiumKeys = Crypto::loadDilithiumKeys(walletName);
    if (dilithiumKeys.privateKey.empty() || dilithiumKeys.publicKey.empty()) {
        throw std::runtime_error("❌ Dilithium keys missing for wallet: " + walletName);
    }

    // --- Falcon ---
    falconKeys = Crypto::loadFalconKeys(walletName);
    if (falconKeys.privateKey.empty() || falconKeys.publicKey.empty()) {
        throw std::runtime_error("❌ Falcon keys missing for wallet: " + walletName);
    }

    updateAddressFromPQKeys();

    // Optional: verify address matches public key
    std::string derived = Crypto::generateAddress(publicKey);
    if (derived != walletName) {
        std::cerr << "⚠️ Warning: Loaded public key doesn't match provided address.\n";
    }

    std::cout << "✅ Wallet loaded successfully!\nAddress: " << address
              << "\nKey Identifier: " << walletName << std::endl;
}

// Key loader
std::string Wallet::loadKeyFile(const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open())
        throw std::runtime_error("❌ Failed to open key file: " + path);
    return std::string((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
}

std::string Wallet::getPrivateKeyPath() const {
    return keyDirectory + walletName + "_private.pem";
}

std::string Wallet::getAddress() const { return address; }
std::string Wallet::getPublicKey() const { return publicKey; }
void Wallet::generateDilithiumKeyPair() {
    dilithiumKeys = Crypto::generateDilithiumKeys(walletName);
    updateAddressFromPQKeys();
}
void Wallet::generateFalconKeyPair() {
    falconKeys = Crypto::generateFalconKeys(walletName);
    updateAddressFromPQKeys();
}
std::string Wallet::getDilithiumPublicKey() const {
    if (!dilithiumKeys.publicKeyHex.empty())
        return dilithiumKeys.publicKeyHex;
    return Crypto::toHex(dilithiumKeys.publicKey);
}
std::string Wallet::getFalconPublicKey() const {
    if (!falconKeys.publicKeyHex.empty())
        return falconKeys.publicKeyHex;
    return Crypto::toHex(falconKeys.publicKey);
}
double Wallet::getBalance() const {
    return Blockchain::getInstance().getBalance(address);
}

// Transaction creation
Transaction Wallet::createTransaction(const std::string& recipient, double amount) {
    if (amount <= 0)
        throw std::runtime_error("❌ Amount must be greater than zero.");

   int txCount = Blockchain::getInstance().getRecentTransactionCount();
    double burnRate = Transaction::calculateBurnRate(txCount);
    double burnAmount = amount * burnRate;
    amount -= burnAmount;

    std::cout << "🔥 Burned: " << burnAmount << " AlynCoin (" << (burnRate * 100) << "%)\n";

    Transaction tx(address, recipient, amount,
                   "",
                   "",
                   std::time(nullptr));

    if (dilithiumKeys.publicKey.empty() || falconKeys.publicKey.empty()) {
        throw std::runtime_error("❌ Post-quantum public keys missing for wallet.");
    }

    tx.setSenderPublicKeyDilithium(std::string(
        dilithiumKeys.publicKey.begin(), dilithiumKeys.publicKey.end()));
    tx.setSenderPublicKeyFalcon(std::string(
        falconKeys.publicKey.begin(), falconKeys.publicKey.end()));

    tx.signTransaction(dilithiumKeys.privateKey, falconKeys.privateKey);

    return tx;
}

// Address generator
std::string Wallet::generateAddress(const std::string& publicKey) {
    return Crypto::keccak256(publicKey).substr(0, 40);
}
