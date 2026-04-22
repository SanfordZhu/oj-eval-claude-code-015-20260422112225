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

// Simple hash function
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

    // Store hash instead of full string to save memory
    struct IndexEntry {
        string index;  // Still need string for collision resolution
        streamoff position;
        int value;
        bool deleted;
    };

    unordered_map<size_t, vector<IndexEntry>> hashIndex;

public:
    FileStorage() {
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

    void insert(const string& idx, int value) {
        size_t hash = stringHash(idx);

        // Check if already exists
        auto it = hashIndex.find(hash);
        if (it != hashIndex.end()) {
            for (auto& entry : it->second) {
                if (entry.index == idx && entry.value == value && !entry.deleted) {
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

        // Update index
        hashIndex[hash].push_back({idx, pos, value, false});
    }

    void remove(const string& idx, int value) {
        size_t hash = stringHash(idx);
        auto it = hashIndex.find(hash);
        if (it == hashIndex.end()) return;

        for (auto& entry : it->second) {
            if (entry.index == idx && entry.value == value && !entry.deleted) {
                // Mark as deleted in file
                Record rec;
                dataFile.seekg(entry.position);
                dataFile.read(reinterpret_cast<char*>(&rec), sizeof(Record));
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

    void find(const string& idx) {
        size_t hash = stringHash(idx);
        auto it = hashIndex.find(hash);
        if (it == hashIndex.end()) {
            cout << "null" << endl;
            return;
        }

        vector<int> values;
        for (auto& entry : it->second) {
            if (entry.index == idx && !entry.deleted) {
                values.push_back(entry.value);
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
            size_t hash = stringHash(idx);
            hashIndex[hash].push_back({idx, pos, rec.value, rec.deleted});
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