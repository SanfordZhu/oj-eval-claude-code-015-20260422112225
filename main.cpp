#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <algorithm>
#include <unordered_map>
#include <cstring>
#include <cstdint>

using namespace std;

const int MAX_INDEX_LEN = 64;
const char DATA_FILE[] = "data.bin";

struct Record {
    char index[MAX_INDEX_LEN + 1];
    int value;
    bool deleted;

    Record() : value(0), deleted(false) {
        memset(index, 0, sizeof(index));
    }

    Record(const string& idx, int val) : value(val), deleted(false) {
        strncpy(index, idx.c_str(), MAX_INDEX_LEN);
        index[MAX_INDEX_LEN] = '\0';
    }
};

// djb2 hash
size_t stringHash(const string& str) {
    size_t hash = 5381;
    for (char c : str) {
        hash = ((hash << 5) + hash) + (unsigned char)c;
    }
    return hash;
}

class FileStorage {
private:
    fstream dataFile;

    // Very compact entry: fingerprint (4 bytes) + position (8 bytes) + value (4 bytes) + deleted (1 bit)
    // Store value to avoid file reads
    struct CompactEntry {
        uint32_t fingerprint;
        streamoff position;
        int value;
        bool deleted;

        CompactEntry(uint32_t fp, streamoff pos, int val, bool del)
            : fingerprint(fp), position(pos), value(val), deleted(del) {}
    };

    // Use vector instead of unordered_map for hash table
    // Simple open addressing with linear probing
    static const size_t HASH_TABLE_SIZE = 200003; // Prime > 2*100000
    vector<vector<CompactEntry>> hashTable;

public:
    FileStorage() : hashTable(HASH_TABLE_SIZE) {
        // Check if file exists
        ifstream test(DATA_FILE);
        bool exists = test.good();
        test.close();

        // Open data file
        if (exists) {
            dataFile.open(DATA_FILE, ios::in | ios::out | ios::binary);
            loadIndex();
        } else {
            dataFile.open(DATA_FILE, ios::in | ios::out | ios::binary | ios::trunc);
        }

        if (!dataFile) {
            ofstream create(DATA_FILE, ios::binary);
            create.close();
            dataFile.open(DATA_FILE, ios::in | ios::out | ios::binary);
        }
    }

    ~FileStorage() {
        if (dataFile.is_open()) {
            dataFile.close();
        }
    }

    uint32_t getFingerprint(const string& str) {
        uint32_t fp = 0;
        for (size_t i = 0; i < 4 && i < str.size(); ++i) {
            fp = (fp << 8) | (unsigned char)str[i];
        }
        return fp;
    }

    void insert(const string& idx, int value) {
        size_t hash = stringHash(idx) % HASH_TABLE_SIZE;
        uint32_t fp = getFingerprint(idx);

        // Check if already exists
        for (auto& entry : hashTable[hash]) {
            if (!entry.deleted && entry.fingerprint == fp) {
                // Need to check full string
                Record rec;
                dataFile.seekg(entry.position);
                dataFile.read(reinterpret_cast<char*>(&rec), sizeof(Record));
                if (strcmp(rec.index, idx.c_str()) == 0 && rec.value == value) {
                    return; // Duplicate
                }
            }
        }

        // Append new record
        Record rec(idx, value);
        dataFile.seekp(0, ios::end);
        streamoff pos = dataFile.tellp();
        dataFile.write(reinterpret_cast<const char*>(&rec), sizeof(Record));
        dataFile.flush();

        // Add to hash table
        hashTable[hash].emplace_back(fp, pos, value, false);
    }

    void remove(const string& idx, int value) {
        size_t hash = stringHash(idx) % HASH_TABLE_SIZE;
        uint32_t fp = getFingerprint(idx);

        for (auto& entry : hashTable[hash]) {
            if (!entry.deleted && entry.fingerprint == fp && entry.value == value) {
                // Need to verify full string
                Record rec;
                dataFile.seekg(entry.position);
                dataFile.read(reinterpret_cast<char*>(&rec), sizeof(Record));
                if (strcmp(rec.index, idx.c_str()) == 0) {
                    // Mark as deleted in file
                    rec.deleted = true;
                    dataFile.seekp(entry.position);
                    dataFile.write(reinterpret_cast<const char*>(&rec), sizeof(Record));
                    dataFile.flush();

                    // Mark as deleted in index
                    entry.deleted = true;
                    return;
                }
            }
        }
    }

    void find(const string& idx) {
        size_t hash = stringHash(idx) % HASH_TABLE_SIZE;
        uint32_t fp = getFingerprint(idx);

        vector<int> values;
        for (auto& entry : hashTable[hash]) {
            if (!entry.deleted && entry.fingerprint == fp) {
                // Verify full string
                Record rec;
                dataFile.seekg(entry.position);
                dataFile.read(reinterpret_cast<char*>(&rec), sizeof(Record));
                if (strcmp(rec.index, idx.c_str()) == 0) {
                    values.push_back(entry.value);
                }
            }
        }

        if (values.empty()) {
            cout << "null" << endl;
        } else {
            sort(values.begin(), values.end());
            for (size_t i = 0; i < values.size(); ++i) {
                if (i > 0) cout << " ";
                cout << values[i];
            }
            cout << endl;
        }
    }

private:
    void loadIndex() {
        dataFile.seekg(0, ios::beg);
        Record rec;
        streamoff pos = dataFile.tellg();

        while (dataFile.read(reinterpret_cast<char*>(&rec), sizeof(Record))) {
            string idx(rec.index);
            size_t hash = stringHash(idx) % HASH_TABLE_SIZE;
            uint32_t fp = getFingerprint(idx);
            hashTable[hash].emplace_back(fp, pos, rec.value, rec.deleted);
            pos = dataFile.tellg();
        }
        dataFile.clear();
    }
};

int main() {
    ios::sync_with_stdio(false);
    cin.tie(nullptr);

    FileStorage storage;
    int n;
    cin >> n;
    cin.ignore();

    for (int i = 0; i < n; ++i) {
        string line;
        getline(cin, line);

        size_t firstSpace = line.find(' ');
        if (firstSpace == string::npos) continue;

        string cmd = line.substr(0, firstSpace);
        string rest = line.substr(firstSpace + 1);

        if (cmd == "insert") {
            size_t secondSpace = rest.find(' ');
            if (secondSpace == string::npos) continue;
            string index = rest.substr(0, secondSpace);
            int value = stoi(rest.substr(secondSpace + 1));
            storage.insert(index, value);
        } else if (cmd == "delete") {
            size_t secondSpace = rest.find(' ');
            if (secondSpace == string::npos) continue;
            string index = rest.substr(0, secondSpace);
            int value = stoi(rest.substr(secondSpace + 1));
            storage.remove(index, value);
        } else if (cmd == "find") {
            string index = rest;
            storage.find(index);
        }
    }

    return 0;
}