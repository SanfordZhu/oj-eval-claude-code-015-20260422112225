#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <algorithm>
#include <cstring>

using namespace std;

const int MAX_INDEX_LEN = 64;
const char DATA_FILE[] = "data.bin";

struct Record {
    char index[MAX_INDEX_LEN + 1];  // +1 for null terminator
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

public:
    FileStorage() {
        // Check if file exists
        ifstream test(DATA_FILE);
        bool exists = test.good();
        test.close();

        // Open data file in binary mode for reading and writing
        if (exists) {
            dataFile.open(DATA_FILE, ios::in | ios::out | ios::binary);
        } else {
            dataFile.open(DATA_FILE, ios::in | ios::out | ios::binary | ios::trunc);
        }

        if (!dataFile) {
            // Fallback: create empty file
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

    void insert(const string& index, int value) {
        // Check if record already exists
        vector<streampos> positions;
        findAllPositions(index, positions);

        for (streampos pos : positions) {
            Record rec;
            dataFile.seekg(pos);
            dataFile.read(reinterpret_cast<char*>(&rec), sizeof(Record));
            if (!rec.deleted && rec.value == value) {
                // Duplicate (index, value) pair, do nothing
                return;
            }
        }

        // Append new record
        Record rec(index, value);
        dataFile.seekp(0, ios::end);
        dataFile.write(reinterpret_cast<const char*>(&rec), sizeof(Record));
        dataFile.flush();
    }

    void remove(const string& index, int value) {
        vector<streampos> positions;
        findAllPositions(index, positions);

        for (streampos pos : positions) {
            Record rec;
            dataFile.seekg(pos);
            dataFile.read(reinterpret_cast<char*>(&rec), sizeof(Record));
            if (!rec.deleted && rec.value == value) {
                // Mark as deleted
                rec.deleted = true;
                dataFile.seekp(pos);
                dataFile.write(reinterpret_cast<const char*>(&rec), sizeof(Record));
                dataFile.flush();
                return;
            }
        }
        // Not found, do nothing as per problem statement
    }

    void find(const string& index) {
        vector<int> values;
        vector<streampos> positions;
        findAllPositions(index, positions);

        for (streampos pos : positions) {
            Record rec;
            dataFile.seekg(pos);
            dataFile.read(reinterpret_cast<char*>(&rec), sizeof(Record));
            if (!rec.deleted) {
                values.push_back(rec.value);
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
    void findAllPositions(const string& index, vector<streampos>& positions) {
        dataFile.seekg(0, ios::beg);
        Record rec;
        streampos pos = dataFile.tellg();

        while (dataFile.read(reinterpret_cast<char*>(&rec), sizeof(Record))) {
            if (strcmp(rec.index, index.c_str()) == 0) {
                positions.push_back(pos);
            }
            pos = dataFile.tellg();
        }
        dataFile.clear();  // Clear EOF flag
    }
};

int main() {
    ios::sync_with_stdio(false);
    cin.tie(nullptr);

    FileStorage storage;
    int n;
    cin >> n;
    cin.ignore();  // Ignore newline after n

    for (int i = 0; i < n; ++i) {
        string line;
        getline(cin, line);

        // Parse command
        size_t firstSpace = line.find(' ');
        if (firstSpace == string::npos) {
            continue;  // Invalid command
        }

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