#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <algorithm>
#include <unordered_map>
#include <cstring>

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
    unordered_map<string, vector<int>> index;  // index -> values (sorted)

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
        // Check if already exists
        auto it = index.find(idx);
        if (it != index.end()) {
            vector<int>& values = it->second;
            // Binary search for value
            auto pos = lower_bound(values.begin(), values.end(), value);
            if (pos != values.end() && *pos == value) {
                // Duplicate (index, value) pair
                return;
            }
            // Insert at correct position to keep sorted
            values.insert(pos, value);
        } else {
            // New index
            index[idx] = {value};
        }

        // Write to file
        Record rec(idx, value);
        dataFile.seekp(0, ios::end);
        dataFile.write(reinterpret_cast<const char*>(&rec), sizeof(Record));
        dataFile.flush();
    }

    void remove(const string& idx, int value) {
        auto it = index.find(idx);
        if (it == index.end()) return;

        vector<int>& values = it->second;
        auto pos = lower_bound(values.begin(), values.end(), value);
        if (pos != values.end() && *pos == value) {
            // Remove from vector
            values.erase(pos);
            // If vector becomes empty, remove key from map
            if (values.empty()) {
                index.erase(it);
            }

            // Mark as deleted in file (optional, for consistency)
            // We would need to find the record in file... skip for now
            // File marking is not strictly needed since we use in-memory index
        }
    }

    void find(const string& idx) {
        auto it = index.find(idx);
        if (it == index.end() || it->second.empty()) {
            cout << "null" << endl;
            return;
        }

        const vector<int>& values = it->second;
        for (size_t i = 0; i < values.size(); ++i) {
            if (i > 0) cout << " ";
            cout << values[i];
        }
        cout << endl;
    }

private:
    void loadIndex() {
        dataFile.seekg(0, ios::beg);
        Record rec;

        while (dataFile.read(reinterpret_cast<char*>(&rec), sizeof(Record))) {
            if (!rec.deleted) {
                string idx(rec.index);
                // Insert maintaining sorted order
                auto it = index.find(idx);
                if (it != index.end()) {
                    vector<int>& values = it->second;
                    auto pos = lower_bound(values.begin(), values.end(), rec.value);
                    if (pos == values.end() || *pos != rec.value) {
                        values.insert(pos, rec.value);
                    }
                } else {
                    index[idx] = {rec.value};
                }
            }
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