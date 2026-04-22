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

class FileStorage {
private:
    fstream dataFile;
    unordered_map<string, vector<streamoff>> index;  // index -> positions in file
    unordered_map<streamoff, uint8_t> deletedMap;  // position -> deleted status (uint8_t to save memory)

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
        // Check if already exists using index
        if (index.find(idx) != index.end()) {
            for (streampos pos : index[idx]) {
                if (deletedMap[pos]) continue;
                Record rec;
                dataFile.seekg(pos);
                dataFile.read(reinterpret_cast<char*>(&rec), sizeof(Record));
                if (rec.value == value) {
                    // Duplicate (index, value) pair
                    return;
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
        index[idx].push_back(pos);
        deletedMap[pos] = 0;
    }

    void remove(const string& idx, int value) {
        if (index.find(idx) == index.end()) return;

        for (streamoff pos : index[idx]) {
            if (deletedMap[pos]) continue;
            Record rec;
            dataFile.seekg(pos);
            dataFile.read(reinterpret_cast<char*>(&rec), sizeof(Record));
            if (rec.value == value) {
                // Mark as deleted
                rec.deleted = true;
                dataFile.seekp(pos);
                dataFile.write(reinterpret_cast<const char*>(&rec), sizeof(Record));
                dataFile.flush();
                deletedMap[pos] = 1;
                return;
            }
        }
    }

    void find(const string& idx) {
        if (index.find(idx) == index.end()) {
            cout << "null" << endl;
            return;
        }

        vector<int> values;
        for (streamoff pos : index[idx]) {
            if (deletedMap[pos]) continue;
            Record rec;
            dataFile.seekg(pos);
            dataFile.read(reinterpret_cast<char*>(&rec), sizeof(Record));
            values.push_back(rec.value);
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
            index[idx].push_back(pos);
            deletedMap[pos] = rec.deleted ? 1 : 0;
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