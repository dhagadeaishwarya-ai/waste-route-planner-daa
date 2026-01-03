/**********************************************************************************************
SMART CITY WASTE COLLECTION ROUTE PLANNER
TSP-Lite (Greedy) + BFS Spread + Capacity Return + Priority Sort
ASCII VERSION (NO UTF-8 / NO EMOJIS) | CODEBLOCKS COMPATIBLE
COLUMN WIDTH SETTING = 35 characters
ENHANCED VERSION WITH ALL REQUIRED ALGORITHMS AND DATA STRUCTURES
**********************************************************************************************/

#include <bits/stdc++.h>
#include <iomanip>
#include <fstream>
#include <limits>
#include <algorithm>
#include <numeric>
using namespace std;

/**********************************************************************************************
COLOR CODES (WINDOWS 10+ CMD SUPPORTED, CAN BE REMOVED IF REQUIRED)
**********************************************************************************************/
#define RESET   "\033[0m"
#define RED     "\033[31m"
#define YELLOW  "\033[33m"
#define GREEN   "\033[32m"
#define CYAN    "\033[36m"
#define MAGENTA "\033[35m"
#define BLUE    "\033[34m"
#define BOLD    "\033[1m"

/**********************************************************************************************
GLOBAL CONSTANTS
**********************************************************************************************/
const int INF = 1e9;
const int TRUCK_CAPACITY = 200;
const double FUEL_PRICE = 100.0;
const double TRUCK_MILEAGE = 3.0;
const int COLUMN_WIDTH = 35;
const int MAX_HISTORY_SIZE = 100;
string currentRole;
bool loggedIn = false;

/**********************************************************************************************
DATA STRUCTURES
**********************************************************************************************/
struct Zone {
    int id;
    string name;
    int currentWaste;
    vector<int> history;
    double x, y;  // Coordinates for distance calculation
    int priority;
    
    Zone(int id = 0, string name = "", int currentWaste = 0, double x = 0.0, double y = 0.0)
        : id(id), name(name), currentWaste(currentWaste), x(x), y(y), priority(0) {}
};

struct Edge {
    int to;
    int weight;
};

struct Route {
    vector<int> path;
    int totalDistance;
    double totalCost;
    int totalWaste;
    
    Route() : totalDistance(0), totalCost(0.0), totalWaste(0) {}
};

/**********************************************************************************************
HEAP DATA STRUCTURE (PRIORITY QUEUE IMPLEMENTATION)
**********************************************************************************************/
class MinHeap {
private:
    vector<pair<int, int>> heap;  // {priority, zone_id}
    int size;
    
    void heapifyUp(int index) {
        while (index > 0) {
            int parent = (index - 1) / 2;
            if (heap[parent].first > heap[index].first) {
                swap(heap[parent], heap[index]);
                index = parent;
            } else {
                break;
            }
        }
    }
    
    void heapifyDown(int index) {
        while (true) {
            int left = 2 * index + 1;
            int right = 2 * index + 2;
            int smallest = index;
            
            if (left < size && heap[left].first < heap[smallest].first) {
                smallest = left;
            }
            if (right < size && heap[right].first < heap[smallest].first) {
                smallest = right;
            }
            
            if (smallest != index) {
                swap(heap[index], heap[smallest]);
                index = smallest;
            } else {
                break;
            }
        }
    }
    
public:
    MinHeap() : size(0) {}
    
    void push(int priority, int zoneId) {
        heap.push_back({priority, zoneId});
        size++;
        heapifyUp(size - 1);
    }
    
    pair<int, int> pop() {
        if (size == 0) return {-1, -1};
        
        pair<int, int> top = heap[0];
        heap[0] = heap[size - 1];
        heap.pop_back();
        size--;
        
        if (size > 0) {
            heapifyDown(0);
        }
        
        return top;
    }
    
    bool empty() const {
        return size == 0;
    }
    
    int getSize() const {
        return size;
    }
    
    pair<int, int> top() const {
        if (size == 0) return {-1, -1};
        return heap[0];
    }
};

/**********************************************************************************************
SEGMENT TREE FOR WASTE PREDICTION AND QUERIES
**********************************************************************************************/
class SegmentTree {
private:
    vector<int> tree;
    vector<int> arr;
    int n;
    
    void build(int node, int start, int end) {
        if (start == end) {
            tree[node] = arr[start];
        } else {
            int mid = (start + end) / 2;
            build(2 * node, start, mid);
            build(2 * node + 1, mid + 1, end);
            tree[node] = max(tree[2 * node], tree[2 * node + 1]);
        }
    }
    
    void updateUtil(int node, int start, int end, int idx, int val) {
        if (start == end) {
            arr[idx] = val;
            tree[node] = val;
        } else {
            int mid = (start + end) / 2;
            if (start <= idx && idx <= mid) {
                updateUtil(2 * node, start, mid, idx, val);
            } else {
                updateUtil(2 * node + 1, mid + 1, end, idx, val);
            }
            tree[node] = max(tree[2 * node], tree[2 * node + 1]);
        }
    }
    
    int queryUtil(int node, int start, int end, int l, int r) {
        if (r < start || end < l) {
            return -INF;
        }
        if (l <= start && end <= r) {
            return tree[node];
        }
        int mid = (start + end) / 2;
        int leftQuery = queryUtil(2 * node, start, mid, l, r);
        int rightQuery = queryUtil(2 * node + 1, mid + 1, end, l, r);
        return max(leftQuery, rightQuery);
    }
    
public:
    SegmentTree(const vector<int>& data) {
        n = data.size();
        arr = data;
        tree.resize(4 * n);
        build(1, 0, n - 1);
    }
    
    void update(int idx, int val) {
        updateUtil(1, 0, n - 1, idx, val);
    }
    
    int query(int l, int r) {
        return queryUtil(1, 0, n - 1, l, r);
    }
    
    int getMax() {
        return tree[1];
    }
};

/**********************************************************************************************
SORTING ALGORITHMS
**********************************************************************************************/
class SortingAlgorithms {
public:
    // Quick Sort Implementation
    static void quickSort(vector<Zone>& zones, int low, int high, bool byWaste = true) {
        if (low < high) {
            int pivot = partition(zones, low, high, byWaste);
            quickSort(zones, low, pivot - 1, byWaste);
            quickSort(zones, pivot + 1, high, byWaste);
        }
    }
    
private:
    static int partition(vector<Zone>& zones, int low, int high, bool byWaste) {
        Zone pivot = zones[high];
        int i = low - 1;
        
        for (int j = low; j < high; j++) {
            bool condition = byWaste ? 
                (zones[j].currentWaste > pivot.currentWaste) : 
                (zones[j].id < pivot.id);
                
            if (condition) {
                i++;
                swap(zones[i], zones[j]);
            }
        }
        swap(zones[i + 1], zones[high]);
        return i + 1;
    }
    
public:
    // Merge Sort Implementation
    static void mergeSort(vector<Zone>& zones, int left, int right, bool byWaste = true) {
        if (left < right) {
            int mid = left + (right - left) / 2;
            mergeSort(zones, left, mid, byWaste);
            mergeSort(zones, mid + 1, right, byWaste);
            merge(zones, left, mid, right, byWaste);
        }
    }
    
private:
    static void merge(vector<Zone>& zones, int left, int mid, int right, bool byWaste) {
        int n1 = mid - left + 1;
        int n2 = right - mid;
        
        vector<Zone> leftArr(n1), rightArr(n2);
        
        for (int i = 0; i < n1; i++) {
            leftArr[i] = zones[left + i];
        }
        for (int j = 0; j < n2; j++) {
            rightArr[j] = zones[mid + 1 + j];
        }
        
        int i = 0, j = 0, k = left;
        
        while (i < n1 && j < n2) {
            bool condition = byWaste ?
                (leftArr[i].currentWaste >= rightArr[j].currentWaste) :
                (leftArr[i].id <= rightArr[j].id);
                
            if (condition) {
                zones[k] = leftArr[i];
                i++;
            } else {
                zones[k] = rightArr[j];
                j++;
            }
            k++;
        }
        
        while (i < n1) {
            zones[k] = leftArr[i];
            i++;
            k++;
        }
        
        while (j < n2) {
            zones[k] = rightArr[j];
            j++;
            k++;
        }
    }
};

/**********************************************************************************************
LOOKUP TABLE FOR FAST ZONE SEARCH
**********************************************************************************************/
class LookupTable {
private:
    unordered_map<string, int> nameToId;
    unordered_map<int, string> idToName;
    vector<string> namePrefixes;  // For autocomplete
    
public:
    void insert(int id, const string& name) {
        nameToId[name] = id;
        idToName[id] = name;
        
        // Build prefix table for autocomplete
        for (int len = 1; len <= name.length(); len++) {
            string prefix = name.substr(0, len);
            transform(prefix.begin(), prefix.end(), prefix.begin(), ::toupper);
            namePrefixes.push_back(prefix);
        }
    }
    
    int search(const string& name) {
        string upperName = name;
        transform(upperName.begin(), upperName.end(), upperName.begin(), ::toupper);
        
        if (nameToId.find(upperName) != nameToId.end()) {
            return nameToId[upperName];
        }
        
        // Partial match search
        for (auto& pair : nameToId) {
            string upperKey = pair.first;
            transform(upperKey.begin(), upperKey.end(), upperKey.begin(), ::toupper);
            if (upperKey.find(upperName) != string::npos || upperName.find(upperKey) != string::npos) {
                return pair.second;
            }
        }
        return -1;
    }
    
    vector<int> findAllMatches(const string& pattern) {
        vector<int> matches;
        string upperPattern = pattern;
        transform(upperPattern.begin(), upperPattern.end(), upperPattern.begin(), ::toupper);
        
        for (auto& pair : nameToId) {
            string upperKey = pair.first;
            transform(upperKey.begin(), upperKey.end(), upperKey.begin(), ::toupper);
            if (upperKey.find(upperPattern) != string::npos) {
                matches.push_back(pair.second);
            }
        }
        return matches;
    }
    
    string getName(int id) {
        if (idToName.find(id) != idToName.end()) {
            return idToName[id];
        }
        return "";
    }
    
    void clear() {
        nameToId.clear();
        idToName.clear();
        namePrefixes.clear();
    }
};

/**********************************************************************************************
MAIN CLASS - WasteRoutePlanner
**********************************************************************************************/
class WasteRoutePlanner {

private:

    vector<Zone> zones;
    unordered_map<string,int> id;
    vector<vector<Edge>> adj;
    vector<vector<int>> dist;
    LookupTable lookupTable;
    
    int dumpingYard;
    vector<int> referenceDayWaste;
    vector<SegmentTree*> segmentTrees;  // One per zone for history analysis
    
    // Route comparison storage
    vector<Route> savedRoutes;

public:

/**********************************************************************************************
DISPLAY ALL DRIVERS (ADMIN ONLY)
**********************************************************************************************/
void displayAllDrivers() {
    system("cls");

    cout << BLUE << "=============== REGISTERED DRIVERS LIST ===============\n" << RESET;

    ifstream file("drivers.txt");

    if (!file) {
        cout << RED << "Error: drivers.txt file not found.\n" << RESET;
        return;
    }

    string username, password;
    int count = 0;

    cout << left << setw(10) << "ID"
         << setw(25) << "DRIVER USERNAME" << "\n";
    cout << "-------------------------------------------------------\n";

    while (file >> username >> password) {
        cout << left << setw(10) << (++count)
             << setw(25) << username << "\n";
    }

    file.close();

    if (count == 0) {
        cout << YELLOW << "No drivers registered yet.\n" << RESET;
    }

    cout << "-------------------------------------------------------\n";
    cout << GREEN << "Total Drivers: " << count << RESET << "\n";
}


/**********************************************************************************************
LOAD ZONES FROM FILE
**********************************************************************************************/
void loadZonesFromFile(const string& filename) {
    ifstream file(filename);

    if (!file) {
        cout << RED << "Error: Unable to open " << filename << RESET << "\n";
        exit(1);
    }

    zones.clear();
    lookupTable.clear();
    id.clear();

    int zid, waste;
    double x, y;
    string name;

    while (file >> zid >> name >> waste >> x >> y) {
        Zone z;
        z.id = zid;
        z.name = name;
        z.currentWaste = waste;
        z.x = x;
        z.y = y;
        z.priority = waste > 80 ? 1 : (waste > 50 ? 2 : 3);
        z.history = { waste };

        zones.push_back(z);
        id[name] = zid;
        lookupTable.insert(zid, name);
    }

    file.close();

    adj.assign(zones.size(), {});
    dumpingYard = id["DUMPING_YARD"];

    cout << GREEN << "Zones loaded from file (" 
         << zones.size() << " zones)\n" << RESET;
}

/**********************************************************************************************
CONSTRUCTOR — INITIALIZATION SEQUENCE
**********************************************************************************************/
WasteRoutePlanner() {
    srand(time(0));
    initCity();
    referenceDayWaste.resize(zones.size());
    for(int i=0;i<zones.size();i++) {
        referenceDayWaste[i] = zones[i].currentWaste;
    }
    initSegmentTrees();
    floydWarshall();
}

/**********************************************************************************************
INITIALIZE SEGMENT TREES FOR ALL ZONES
**********************************************************************************************/
void initSegmentTrees() {
    segmentTrees.clear();
    for(auto& zone : zones) {
        if(zone.history.empty()) {
            zone.history = {rand()%100, rand()%100, zone.currentWaste};
        }
        SegmentTree* st = new SegmentTree(zone.history);
        segmentTrees.push_back(st);
    }
}

/**********************************************************************************************
CITY MAP INITIALIZATION
**********************************************************************************************/
void initCity() {

    // 1️⃣ Load zones from external file
    loadZonesFromFile("zones.txt");   // or "zones_extended.txt"

    // 2️⃣ Lambda to add bidirectional roads
    auto addRoad = [&](const string& a, const string& b, int d){
        adj[id[a]].push_back({id[b], d});
        adj[id[b]].push_back({id[a], d});
    };

    // ===================== ROADS (BASED ON YOUR MAP STRUCTURE) =====================

    addRoad("AZAM_NAGAR_1ST_CROSS","AZAM_NAGAR_2ND_CROSS",1);
    addRoad("AZAM_NAGAR_2ND_CROSS","AZAM_NAGAR_3RD_CROSS",3);
    addRoad("AZAM_NAGAR_3RD_CROSS","AZAM_NAGAR_4TH_CROSS",1);
    addRoad("AZAM_NAGAR_4TH_CROSS","AZAM_NAGAR_5TH_CROSS",1);
    addRoad("AZAM_NAGAR_5TH_CROSS","AZAM_NAGAR_CIRCLE",2);

    addRoad("AZAM_NAGAR_1ST_CROSS","GARDEN",2);
    addRoad("GARDEN","DAMRO",3);
    addRoad("DAMRO","AZAM_NAGAR_CIRCLE",4);

    addRoad("DHABA_POINT","CROSS_6",1);
    addRoad("DHABA_POINT","CROSS_7",2);
    addRoad("DHABA_POINT","CROSS_8",4);

    addRoad("CROSS_6","AZAM_NAGAR_CIRCLE",1);
    addRoad("CROSS_7","AZAM_NAGAR_CIRCLE",1);
    addRoad("CROSS_8","AZAM_NAGAR_CIRCLE",8);

    addRoad("AZAM_NAGAR_CIRCLE","JUNCTION_0",1);
    addRoad("JUNCTION_0","JUNCTION_1",2);
    addRoad("JUNCTION_1","JUNCTION_2",1);
    addRoad("JUNCTION_2","JUNCTION_3",1);
    addRoad("JUNCTION_3","JUNCTION_4",8);
    addRoad("JUNCTION_4","JUNCTION_5",5);

    addRoad("JUNCTION_0","CROSS_6",1);
    addRoad("JUNCTION_1","CROSS_7",1);
    addRoad("JUNCTION_2","CROSS_8",1);

    addRoad("JUNCTION_4","SAMATH",2);
    addRoad("SAMATH","DATT",1);
    addRoad("DATT","VANDAN_COLONY",1);
    addRoad("SAMATH","JUNCTION_5",1);

    addRoad("AZAM_NAGAR_CIRCLE","STEAM_OFFICE",6);
    addRoad("STEAM_OFFICE","TRAINING_CENTER",1);
    addRoad("STEAM_OFFICE","KALMESHWAR",4);

    addRoad("KALMESHWAR","JUNCTION_4",8);
    addRoad("JUNCTION_3","DAMRO",8);

    addRoad("DAMRO","DUMPING_YARD",8);
    addRoad("DUMPING_YARD","AZAM_NAGAR_CIRCLE",10);

    // =========================================================================

    cout << GREEN
         << "City Map Initialized using zones.txt ("
         << zones.size()
         << " zones loaded)"
         << RESET << "\n";
}
/**********************************************************************************************
FLOYD-WARSHALL (ALL-PAIRS SHORTEST PATHS)
**********************************************************************************************/
void floydWarshall(){
    int n = zones.size();
    dist.assign(n, vector<int>(n, INF));

    for(int i=0;i<n;i++) dist[i][i] = 0;
    for(int u=0;u<n;u++)
        for(auto &e: adj[u])
            dist[u][e.to] = e.weight;

    for(int k=0;k<n;k++)
    for(int i=0;i<n;i++)
    for(int j=0;j<n;j++)
        if(dist[i][k] != INF && dist[k][j] != INF)
            dist[i][j] = min(dist[i][j], dist[i][k] + dist[k][j]);

    cout << BLUE << "Distance matrix ready (Floyd-Warshall complete)" << RESET << "\n";
}

/**********************************************************************************************
DIJKSTRA'S ALGORITHM FOR SINGLE SOURCE SHORTEST PATH
**********************************************************************************************/
vector<int> dijkstra(int start) {
    int n = zones.size();
    vector<int> distance(n, INF);
    vector<bool> visited(n, false);
    MinHeap pq;
    
    distance[start] = 0;
    pq.push(0, start);
    
    while (!pq.empty()) {
        pair<int, int> current = pq.pop();
        int u = current.second;
        int dist_u = current.first;
        
        if (visited[u]) continue;
        visited[u] = true;
        
        for (auto& edge : adj[u]) {
            int v = edge.to;
            int weight = edge.weight;
            
            if (!visited[v] && dist_u + weight < distance[v]) {
                distance[v] = dist_u + weight;
                pq.push(distance[v], v);
            }
        }
    }
    
    return distance;
}

/**********************************************************************************************
SHOW ALL ZONES STATUS
**********************************************************************************************/
void showZones(){
    system("cls");
    cout << CYAN << "\n--------------- ZONES AND STATUS ---------------\n" << RESET;

    cout << left << setw(COLUMN_WIDTH) << "ZONE NAME"
         << setw(10) << "STATUS"
         << setw(15) << "WASTE (%)"
         << "PRIORITY" << "\n";

    cout << "--------------------------------------------------------------\n";

    for(auto &z : zones){
        string status = "HEALTHY";
        string col = GREEN;
        if(z.currentWaste > 80){ status="CRITICAL"; col=RED; }
        else if(z.currentWaste > 50){ status="WARNING"; col=YELLOW; }

        cout << left << setw(COLUMN_WIDTH) << z.name
             << col << setw(10) << status << RESET
             << setw(15) << z.currentWaste << "%"
             << z.priority << "\n";
    }

    cout << "--------------------------------------------------------------\n";
}

/**********************************************************************************************
PRIORITY SORT (BY WASTE, DESCENDING) - USING QUICK SORT
**********************************************************************************************/
static bool sortByWaste(const Zone &a, const Zone &b){
    return a.currentWaste > b.currentWaste;
}

void prioritySort(){
    system("cls");
    cout << CYAN << "Choose Sorting Algorithm:\n" << RESET;
    cout << "1. Quick Sort (Recommended for large data)\n";
    cout << "2. Merge Sort (Stable sorting)\n";
    cout << "3. Standard Sort (STL)\n";
    cout << "4. Heap-based Priority Queue\n";
    cout << "Enter choice: ";
    
    int choice;
    cin >> choice;
    
    vector<Zone> v = zones;
    
    system("cls");
    
    switch(choice) {
        case 1:
            SortingAlgorithms::quickSort(v, 0, v.size() - 1, true);
            cout << MAGENTA << "PRIORITY ORDER (Quick Sort - High to Low Waste)\n" << RESET;
            break;
        case 2:
            SortingAlgorithms::mergeSort(v, 0, v.size() - 1, true);
            cout << MAGENTA << "PRIORITY ORDER (Merge Sort - High to Low Waste)\n" << RESET;
            break;
        case 3:
            sort(v.begin(), v.end(), sortByWaste);
            cout << MAGENTA << "PRIORITY ORDER (STL Sort - High to Low Waste)\n" << RESET;
            break;
        case 4:
            prioritySortHeap(v);
            cout << MAGENTA << "PRIORITY ORDER (Heap - High to Low Waste)\n" << RESET;
            break;
        default:
            SortingAlgorithms::quickSort(v, 0, v.size() - 1, true);
            cout << MAGENTA << "PRIORITY ORDER (Quick Sort - High to Low Waste)\n" << RESET;
    }
    
    cout << left << setw(COLUMN_WIDTH) << "ZONE NAME"
         << setw(15) << "WASTE (%)"
         << "PRIORITY\n";
    cout << "--------------------------------------------------------------\n";

    for(auto &z : v){
        string col = GREEN;
        if(z.currentWaste > 80) col = RED;
        else if(z.currentWaste > 50) col = YELLOW;
        
        cout << left << setw(COLUMN_WIDTH) << z.name
             << col << setw(15) << z.currentWaste << "%" << RESET
             << z.priority << "\n";
    }

    cout << "--------------------------------------------------------------\n";
}

/**********************************************************************************************
HEAP-BASED PRIORITY SORT
**********************************************************************************************/
void prioritySortHeap(vector<Zone>& result) {
    MinHeap heap;
    result.clear();
    
    // Build max heap (using negative priority for min heap to act as max)
    for(auto& zone : zones) {
        heap.push(-zone.currentWaste, zone.id);  // Negative for max heap behavior
    }
    
    vector<pair<int, int>> sorted;
    while(!heap.empty()) {
        pair<int, int> top = heap.pop();
        sorted.push_back({-top.first, top.second});  // Negate back
    }
    
    reverse(sorted.begin(), sorted.end());
    
    for(auto& p : sorted) {
        result.push_back(zones[p.second]);
    }
}

/**********************************************************************************************
PREDICT NEXT DAY WASTE USING SEGMENT TREE AND DP
**********************************************************************************************/
void predictWaste(){
    cin.ignore(numeric_limits<streamsize>::max(), '\n');

    cout << CYAN << "Enter zone to predict (partial or full name): " << RESET;
    string input; getline(cin, input);

    for(char &c : input) c = toupper(c);

    vector<int> matches = lookupTable.findAllMatches(input);

    if(matches.empty()){
        cout << RED << "No matching zone found.\n" << RESET;
        return;
    }

    int selected;
    if(matches.size() == 1){
        selected = matches[0];
    } else {
        cout << YELLOW << "Multiple matches found. Select one:\n" << RESET;
        for(int i=0;i<matches.size();i++)
            cout << "(" << i+1 << ") " << zones[matches[i]].name << "\n";

        cout << CYAN << "Enter choice number: " << RESET;
        int c; cin >> c;
        if(c < 1 || c > matches.size()){
            cout << RED << "Invalid choice.\n" << RESET;
            return;
        }
        selected = matches[c-1];
    }

    // Enhanced prediction using Segment Tree and DP-style recurrence
    Zone& zone = zones[selected];
    
    // Calculate trend from history using segment tree
    if(segmentTrees[selected] != nullptr && zone.history.size() > 1) {
        int maxRecent = segmentTrees[selected]->getMax();
        int avgRecent = accumulate(zone.history.begin(), zone.history.end(), 0) / zone.history.size();
        
        // DP-style recurrence: next = weighted average + trend
        int trend = zone.currentWaste - (zone.history.size() > 1 ? zone.history[zone.history.size()-2] : zone.currentWaste);
        int nextPrediction = min(100, max(0, zone.currentWaste + trend + (rand()%20 - 10)));
        
        // Update history
        zone.history.push_back(nextPrediction);
        if(zone.history.size() > MAX_HISTORY_SIZE) {
            zone.history.erase(zone.history.begin());
        }
        
        // Update segment tree
        delete segmentTrees[selected];
        segmentTrees[selected] = new SegmentTree(zone.history);
        
        cout << GREEN << "\n========== PREDICTION RESULTS ==========\n" << RESET;
        cout << "Zone: " << zone.name << "\n";
        cout << "Current Waste: " << zone.currentWaste << "%\n";
        cout << "Max Recent: " << maxRecent << "%\n";
        cout << "Average Recent: " << avgRecent << "%\n";
        cout << "Predicted for Tomorrow: " << nextPrediction << "%\n";
        cout << "Trend: " << (trend > 0 ? "+" : "") << trend << "%\n";
        cout << "==========================================\n" << RESET;
    } else {
        // Simple prediction if no history
        int nextPrediction = min(100, zone.currentWaste + (rand()%30 + 10));
        zone.history.push_back(nextPrediction);
        cout << GREEN << "Predicted waste for tomorrow at "
             << zone.name << " = "
             << nextPrediction << "%" << RESET << "\n";
    }
}

/**********************************************************************************************
BFS SPREAD MODEL — EACH BFS LAYER ADDS 10% WASTE
**********************************************************************************************/
void bfsSpread(){
    cin.ignore(numeric_limits<streamsize>::max(), '\n');

    cout << CYAN << "Enter starting zone for BFS spread (partial/full): " << RESET;
    string input;
    getline(cin, input);

    for(char &c : input) c = toupper(c);

    vector<int> matches = lookupTable.findAllMatches(input);

    if(matches.empty()){
        cout << RED << "No matching zone found. Try again.\n" << RESET;
        cout << YELLOW << "Available zones:\n" << RESET;
        for(auto &z : zones) cout << " - " << z.name << "\n";
        return;
    }

    int start;

    if(matches.size() > 1){
        cout << YELLOW << "\nMultiple matches found:\n" << RESET;
        for(int i=0; i<matches.size(); i++)
            cout << "(" << i+1 << ") " << zones[matches[i]].name << "\n";

        cout << CYAN << "Select number: " << RESET;
        int c; cin >> c;

        if(c < 1 || c > matches.size()){
            cout << RED << "Invalid choice.\n" << RESET;
            return;
        }

        start = matches[c-1];
    }
    else {
        start = matches[0];
    }

    vector<int> level(zones.size(), -1);
    queue<int> q;
    q.push(start);
    level[start] = 0;

    cout << BLUE << "\nApplying BFS waste spread from: "
         << zones[start].name << RESET << "\n";

    vector<int> affectedZones;
    
    while(!q.empty()){
        int u = q.front(); q.pop();

        if(level[u] > 0){
            int increase = level[u] * 10;
            int oldWaste = zones[u].currentWaste;
            zones[u].currentWaste = min(100, zones[u].currentWaste + increase);
            if(zones[u].currentWaste != oldWaste) {
                affectedZones.push_back(u);
            }
        }

        for(auto &e : adj[u]){
            if(level[e.to] == -1){
                level[e.to] = level[u] + 1;
                q.push(e.to);
            }
        }
    }

    cout << GREEN << "\n✔ BFS Spread Complete. " << affectedZones.size() << " zones affected.\n" << RESET;
    
    if(!affectedZones.empty()) {
        cout << CYAN << "Affected zones:\n" << RESET;
        for(int zid : affectedZones) {
            cout << " - " << zones[zid].name << " (Level " << level[zid] << ")\n";
        }
    }
}

/**********************************************************************************************
WEEKLY PLAN — SIMULATE 7 DAY GROWTH
**********************************************************************************************/
void weeklyPlan(){
    cin.ignore(numeric_limits<streamsize>::max(), '\n');

    cout << CYAN << "Enter a day (ex: Mon, Monday, thu, thursday): " << RESET;
    string input;
    getline(cin, input);

    for(char &c : input) c = tolower(c);

    if(input == "thrusday") input = "thursday";

    int dayIndex = -1;
    if(input.find("mon") != string::npos) dayIndex = 0;
    else if(input.find("tue") != string::npos) dayIndex = 1;
    else if(input.find("wed") != string::npos) dayIndex = 2;
    else if(input.find("thu") != string::npos) dayIndex = 3;
    else if(input.find("fri") != string::npos) dayIndex = 4;
    else if(input.find("sat") != string::npos) dayIndex = 5;
    else if(input.find("sun") != string::npos) dayIndex = 6;
    else{
        cout << RED << "Invalid day. Try again.\n" << RESET;
        return;
    }

    vector<string> days = {"Monday","Tuesday","Wednesday","Thursday","Friday","Saturday","Sunday"};

    system("cls");
    cout << BLUE << "==================== "
         << days[dayIndex]
         << " ROUTE PLAN ====================\n" << RESET;

    if(dayIndex == 6){
        cout << BLUE << "NO COLLECTION — WEEKLY OFF (REST DAY)\n" << RESET;
        return;
    }

    cout << left << setw(30) << "ZONE"
         << setw(10) << "COLOR"
         << "WASTE\n";
    cout << "--------------------------------------------------------------\n";

    bool any = false;

    for(int i=0;i<zones.size();i++){
        int w = zones[i].currentWaste;
        bool visit = false;
        string color = GREEN;

        if(w > 50){
            visit = true;
            color = RED;
        }
        else if(w >= 30 && w <= 50){
            if(dayIndex % 2 == 0){
                visit = true;
                color = YELLOW;
            }
        }
        else if(w < 30){
            if(dayIndex == 0 || dayIndex == 3){
                visit = true;
                color = GREEN;
            }
        }

        if(visit){
            any = true;
            cout << left << setw(30)
                 << zones[i].name;

            if(color == RED) cout << RED   << setw(10) << "RED"    << RESET;
            if(color == YELLOW) cout << YELLOW << setw(10) << "YELLOW" << RESET;
            if(color == GREEN) cout << GREEN << setw(10) << "GREEN"  << RESET;

            cout << w << "%\n";
        }
    }

    if(!any){
        cout << GREEN << "\nNO ROUTES TODAY — ALL CLEAR! :)\n" << RESET;
    }

    cout << "--------------------------------------------------------------\n";

    cout << CYAN << "\nOPTIONS:\n";
    cout << "1. Check another day\n";
    cout << "2. Main menu\n";
    cout << "ENTER CHOICE: " << RESET;

    int c; cin >> c;
    if(c == 1) weeklyPlan();
    else return;
}

/**********************************************************************************************
DRIVER INCOME CALCULATOR
**********************************************************************************************/
void driverIncome(){
    cin.ignore(numeric_limits<streamsize>::max(), '\n');

    string name, id;
    float km, hours;

    cout << CYAN << "Enter Driver Name: " << RESET;
    getline(cin, name);

    cout << CYAN << "Enter Driver ID: " << RESET;
    getline(cin, id);

    cout << CYAN << "Enter total KM driven today: " << RESET;
    cin >> km;

    cout << CYAN << "Enter average working hours today: " << RESET;
    cin >> hours;

    float baseSalary = 500;
    float distancePay = km * 10;
    float bonus = 0;

    if(km > 100) bonus += 500;
    if(hours > 7) bonus += 300;

    float totalPay = baseSalary + distancePay + bonus;

    system("cls");
    cout << BLUE << "\n========== DRIVER INCOME SUMMARY ==========\n" << RESET;

    cout << left << setw(20) << "Driver Name:"  << name << "\n";
    cout << left << setw(20) << "Driver ID:"    << id << "\n";
    cout << left << setw(20) << "Base Salary:"   << baseSalary << "\n";
    cout << left << setw(20) << "KM Pay:"          << km << " km  = Rs." << distancePay << "\n";

    if(km > 100)
        cout << GREEN << left << setw(20) << "Mileage Bonus:" << "+500 Applied" << RESET << "\n";
    else
        cout << YELLOW << left << setw(20) << "Mileage Bonus:" << "Not Eligible" << RESET << "\n";

    if(hours > 7)
        cout << GREEN << left << setw(20) << "Overtime Bonus:" << "+300 Applied" << RESET << "\n";
    else
        cout << YELLOW << left << setw(20) << "Overtime Bonus:" << "Not Eligible" << RESET << "\n";

    cout << "--------------------------------------------\n";
    cout << MAGENTA << left << setw(20) << "TOTAL SALARY: " << "Rs." << totalPay << RESET << "\n";
    cout << "============================================\n\n";

    cout << CYAN << "Press Enter to return to main menu..." << RESET;
    cin.ignore();
    cin.get();
}

/**********************************************************************************************
TSP-LITE ROUTE (GREEDY NEAREST NEIGHBOR)
**********************************************************************************************/
void tspLite(){
    cin.ignore(numeric_limits<streamsize>::max(), '\n');
    cout << CYAN << "Enter starting zone (partial/full): " << RESET;
    string in; getline(cin,in);

    for(char &c:in) c = toupper(c);

    vector<int> match = lookupTable.findAllMatches(in);

    if(match.empty()){ cout<<RED<<"No match found.\n"<<RESET; return; }

    int start;
    if(match.size()==1) start = match[0];
    else{
        cout<<YELLOW<<"Multiple matches found:\n"<<RESET;
        for(int i=0;i<match.size();i++)
            cout<<"("<<i+1<<") "<<zones[match[i]].name<<"\n";
        int c; cout<<"Select: "; cin>>c;
        if(c<1||c>match.size()) return;
        start = match[c-1];
    }

    vector<int> vis(zones.size(),0), route;
    vis[start]=1; route.push_back(start);
    int cur = start;
    int totalDist = 0;

    while(true){
        int nxt=-1, best=INF;
        for(int i=0;i<zones.size();i++)
            if(!vis[i] && dist[cur][i]<best)
                best = dist[cur][i], nxt=i;
        if(nxt==-1) break;
        totalDist += best;
        vis[nxt]=1; route.push_back(nxt); cur=nxt;
    }

    cout << BLUE << "\n=================== TSP-LITE ROUTE ===================\n" << RESET;
    cout << left << setw(6) << "STOP" << setw(COLUMN_WIDTH) << "LOCATION" << "DIST FROM PREV\n";
    cout << "--------------------------------------------------------------\n";

    cout << left << setw(6) << "(1)" << setw(COLUMN_WIDTH) << zones[start].name << "---\n";
    int stop = 2;

    for(int i=1;i<route.size();i++){
        int prev = route[i-1];
        int now = route[i];
        cout << "(" << stop++ << ")   "
             << left << setw(COLUMN_WIDTH) << zones[now].name
             << dist[prev][now] << " km\n";
    }

    cout << "--------------------------------------------------------------\n";
    cout << "FINISH AT        : " << zones[cur].name << "\n";
    cout << "STOPS COVERED    : " << route.size() << "\n";
    cout << "TOTAL DISTANCE   : " << totalDist << " km\n";
    
    double cost = (totalDist / TRUCK_MILEAGE) * FUEL_PRICE;
    cout << "ESTIMATED COST   : Rs. " << fixed << setprecision(2) << cost << "\n";
    cout << "==============================================================\n";
    
    // Save route for comparison
    Route savedRoute;
    savedRoute.path = route;
    savedRoute.totalDistance = totalDist;
    savedRoute.totalCost = cost;
    for(int zid : route) {
        savedRoute.totalWaste += zones[zid].currentWaste;
    }
    savedRoutes.push_back(savedRoute);
}

/**********************************************************************************************
DRIVERS ROUTE
**********************************************************************************************/
void driversRoute(){
    cin.ignore(numeric_limits<streamsize>::max(), '\n');

    cout << BLUE << "\n=============== DRIVER ROUTE PLANNING ===============\n" << RESET;

    vector<int> todayRoute;
    vector<int> tomorrowRoute;

    for(auto &z : zones){
        if(z.currentWaste >= 50) todayRoute.push_back(z.id);
        else tomorrowRoute.push_back(z.id);
    }

    int current = id["AZAM_NAGAR_CIRCLE"];
    int totalKm = 0;

    cout << RED << "\nTODAY'S ROUTE (Waste >= 50%)\n" << RESET;
    cout << RED << "--------------------------------------------------------------\n" << RESET;
    cout << BOLD << left << setw(6) << "STOP" << setw(COLUMN_WIDTH) << "LOCATION" << "DISTANCE" << RESET << "\n";
    cout << RED << "--------------------------------------------------------------\n" << RESET;

    if(todayRoute.empty()){
        cout << GREEN << "No locations require visit today.\n" << RESET;
    } else {
        int stop = 1;
        for(int z : todayRoute){
            int km = dist[current][z];
            totalKm += km;

            cout << RED << "[" << stop++ << "] " << RESET
                 << left << setw(COLUMN_WIDTH) << zones[z].name
                 << km << " km\n";

            current = z;
        }
    }

    cout << RED << "--------------------------------------------------------------\n" << RESET;

    cout << YELLOW << "\nTOMORROW'S ROUTE (Waste < 50%)\n" << RESET;
    cout << YELLOW << "--------------------------------------------------------------\n" << RESET;
    cout << BOLD << left << setw(6) << "STOP" << setw(COLUMN_WIDTH) << "LOCATION" << RESET << "\n";
    cout << YELLOW << "--------------------------------------------------------------\n" << RESET;

    if(tomorrowRoute.empty()){
        cout << GREEN << "No scheduled visits for tomorrow.\n" << RESET;
    } else {
        int stop = 1;
        for(int z : tomorrowRoute){
            cout << YELLOW << "[" << stop++ << "] " << RESET
                 << left << setw(COLUMN_WIDTH) << zones[z].name << "\n";
        }
    }

    cout << YELLOW << "--------------------------------------------------------------\n" << RESET;

    if(todayRoute.empty()){
        cout << GREEN << "\nNo waste collected. No cost generated.\n" << RESET;
        return;
    }

    char collect;
    cout << CYAN << "\nDid the driver collect the waste today? (y/n): " << RESET;
    cin >> collect;

    if(collect == 'y' || collect == 'Y'){
        for(int z : todayRoute) zones[z].currentWaste = 0;

        double fuelUsed = double(totalKm) / TRUCK_MILEAGE;
        double cost = fuelUsed * FUEL_PRICE;

        cout << GREEN << "\n================ COLLECTION SUMMARY ================\n" << RESET;
        cout << GREEN << "Total Distance Travelled : " << RESET << totalKm << " km\n";
        cout << GREEN << "Fuel Used                : " << RESET << fixed << setprecision(2) << fuelUsed << " liters\n";
        cout << GREEN << "Fuel Cost                : Rs. " << RESET << fixed << setprecision(2) << cost << "\n";
        cout << GREEN << "=====================================================\n" << RESET;
        cout << GREEN << "✔ Waste levels updated.\n" << RESET;
    }
    else{
        cout << YELLOW << "Waste not collected. No changes applied.\n" << RESET;
    }

    cout << BLUE << "\n=============== ROUTE PLANNING COMPLETE ===============\n" << RESET;
}

/**********************************************************************************************
CAPACITY ROUTE — TRUCK RETURNS TO DUMPING YARD AT 200 UNITS
**********************************************************************************************/
void capacityRoute(){
    int cur = id["AZAM_NAGAR_CIRCLE"];
    int load = 0;
    vector<int> visited(zones.size(),0);

    cout << BLUE << "=========== CAPACITY ROUTE (200 UNITS) ===========\n" << RESET;
    cout << left << setw(6) << "STOP" << setw(COLUMN_WIDTH) << "LOCATION" << "LOAD\n";
    cout << "--------------------------------------------------------------\n";

    int step = 1;
    int totalDistance = 0;
    int dumpVisits = 0;
    
    while(true){
        if(!visited[cur] && cur != dumpingYard){
            load += zones[cur].currentWaste;
            zones[cur].currentWaste = 0;
            visited[cur] = 1;
        }

        cout << "(" << step++ << ")   "
             << left << setw(COLUMN_WIDTH) << zones[cur].name
             << load << "\n";

        if(load >= TRUCK_CAPACITY && cur != dumpingYard){
            int dumpDist = dist[cur][dumpingYard];
            totalDistance += dumpDist;
            cout << RED << "Truck full -> Going to Dumping Yard (" << dumpDist << " km)\n" << RESET;
            cur = dumpingYard;
            load = 0;
            dumpVisits++;
        }

        int nxt=-1, best=INF;
        for(int i=0;i<zones.size();i++)
            if(!visited[i] && i != dumpingYard && dist[cur][i]<best)
                best = dist[cur][i], nxt=i;

        if(nxt==-1) break;
        totalDistance += best;
        cur = nxt;
    }

    cout << "--------------------------------------------------------------\n";
    cout << GREEN << "Route Completed. Final location: " << zones[cur].name << RESET << "\n";
    cout << "Total Distance: " << totalDistance << " km\n";
    cout << "Dumping Yard Visits: " << dumpVisits << "\n";
    double cost = (totalDistance / TRUCK_MILEAGE) * FUEL_PRICE;
    cout << "Total Cost: Rs. " << fixed << setprecision(2) << cost << "\n";
}

/**********************************************************************************************
ROUTE COMPARISON FUNCTIONALITY
**********************************************************************************************/
void compareRoutes(){
    system("cls");
    
    if(savedRoutes.empty()) {
        cout << YELLOW << "No saved routes to compare. Generate routes first using TSP-Lite.\n" << RESET;
        return;
    }
    
    cout << BLUE << "=============== ROUTE COMPARISON ===============\n" << RESET;
    cout << "Number of saved routes: " << savedRoutes.size() << "\n\n";
    
    if(savedRoutes.size() < 2) {
        cout << YELLOW << "Need at least 2 routes for comparison.\n" << RESET;
        return;
    }
    
    cout << left << setw(10) << "ROUTE #"
         << setw(15) << "DISTANCE (km)"
         << setw(15) << "COST (Rs.)"
         << setw(15) << "WASTE"
         << "STOPS\n";
    cout << "--------------------------------------------------------------\n";
    
    for(int i = 0; i < savedRoutes.size(); i++) {
        Route& r = savedRoutes[i];
        cout << left << setw(10) << (i+1)
             << setw(15) << r.totalDistance
             << setw(15) << fixed << setprecision(2) << r.totalCost
             << setw(15) << r.totalWaste
             << r.path.size() << "\n";
    }
    
    cout << "--------------------------------------------------------------\n";
    
    // Find best route by different criteria
    int bestDistance = 0, bestCost = 0, bestWaste = 0;
    int minDist = savedRoutes[0].totalDistance;
    double minCost = savedRoutes[0].totalCost;
    int maxWaste = savedRoutes[0].totalWaste;
    
    for(int i = 0; i < savedRoutes.size(); i++) {
        if(savedRoutes[i].totalDistance < minDist) {
            minDist = savedRoutes[i].totalDistance;
            bestDistance = i;
        }
        if(savedRoutes[i].totalCost < minCost) {
            minCost = savedRoutes[i].totalCost;
            bestCost = i;
        }
        if(savedRoutes[i].totalWaste > maxWaste) {
            maxWaste = savedRoutes[i].totalWaste;
            bestWaste = i;
        }
    }
    
    cout << GREEN << "\nBEST ROUTES:\n" << RESET;
    cout << "Shortest Distance: Route #" << (bestDistance + 1) << " (" << minDist << " km)\n";
    cout << "Lowest Cost: Route #" << (bestCost + 1) << " (Rs. " << fixed << setprecision(2) << minCost << ")\n";
    cout << "Maximum Waste: Route #" << (bestWaste + 1) << " (" << maxWaste << " units)\n";
}

/**********************************************************************************************
STATISTICAL ANALYSIS
**********************************************************************************************/
void statisticalAnalysis(){
    system("cls");
    
    cout << BLUE << "=============== STATISTICAL ANALYSIS ===============\n" << RESET;
    
    int totalZones = zones.size();
    int totalWaste = 0;
    int criticalZones = 0;
    int warningZones = 0;
    int healthyZones = 0;
    int maxWaste = 0, minWaste = 100;
    
    for(auto& zone : zones) {
        totalWaste += zone.currentWaste;
        if(zone.currentWaste > maxWaste) maxWaste = zone.currentWaste;
        if(zone.currentWaste < minWaste) minWaste = zone.currentWaste;
        
        if(zone.currentWaste > 80) criticalZones++;
        else if(zone.currentWaste > 50) warningZones++;
        else healthyZones++;
    }
    
    double avgWaste = (double)totalWaste / totalZones;
    
    cout << left << setw(30) << "Total Zones:" << totalZones << "\n";
    cout << left << setw(30) << "Average Waste Level:" << fixed << setprecision(2) << avgWaste << "%\n";
    cout << left << setw(30) << "Maximum Waste Level:" << maxWaste << "%\n";
    cout << left << setw(30) << "Minimum Waste Level:" << minWaste << "%\n";
    cout << left << setw(30) << "Critical Zones (>80%):" << criticalZones << "\n";
    cout << left << setw(30) << "Warning Zones (50-80%):" << warningZones << "\n";
    cout << left << setw(30) << "Healthy Zones (<50%):" << healthyZones << "\n";
    
    cout << "\n" << CYAN << "WASTE DISTRIBUTION:\n" << RESET;
    cout << RED << "Critical: " << string(criticalZones * 2, '#') << " " << criticalZones << "\n" << RESET;
    cout << YELLOW << "Warning:  " << string(warningZones * 2, '#') << " " << warningZones << "\n" << RESET;
    cout << GREEN << "Healthy:  " << string(healthyZones * 2, '#') << " " << healthyZones << "\n" << RESET;
}

/**********************************************************************************************
SHORTEST PATH BETWEEN TWO ZONES (DIJKSTRA)
**********************************************************************************************/
void shortestPath(){
    cin.ignore(numeric_limits<streamsize>::max(), '\n');
    
    cout << CYAN << "Enter source zone (partial/full): " << RESET;
    string srcInput; getline(cin, srcInput);
    
    cout << CYAN << "Enter destination zone (partial/full): " << RESET;
    string dstInput; getline(cin, dstInput);
    
    for(char &c : srcInput) c = toupper(c);
    for(char &c : dstInput) c = toupper(c);
    
    vector<int> srcMatches = lookupTable.findAllMatches(srcInput);
    vector<int> dstMatches = lookupTable.findAllMatches(dstInput);
    
    if(srcMatches.empty() || dstMatches.empty()) {
        cout << RED << "Invalid zone names.\n" << RESET;
        return;
    }
    
    int src = srcMatches[0];
    int dst = dstMatches[0];
    
    if(srcMatches.size() > 1) {
        cout << YELLOW << "Multiple source matches. Using first: " << zones[src].name << "\n" << RESET;
    }
    if(dstMatches.size() > 1) {
        cout << YELLOW << "Multiple destination matches. Using first: " << zones[dst].name << "\n" << RESET;
    }
    
    // Use Floyd-Warshall distance matrix (already computed)
    if(dist[src][dst] == INF) {
        cout << RED << "No path exists between " << zones[src].name << " and " << zones[dst].name << "\n" << RESET;
        return;
    }
    
    cout << GREEN << "\n========== SHORTEST PATH RESULT ==========\n" << RESET;
    cout << "From: " << zones[src].name << "\n";
    cout << "To: " << zones[dst].name << "\n";
    cout << "Shortest Distance: " << dist[src][dst] << " km\n";
    cout << "Estimated Time: " << (dist[src][dst] * 2) << " minutes (assuming 30 km/h)\n";
    double cost = (dist[src][dst] / TRUCK_MILEAGE) * FUEL_PRICE;
    cout << "Fuel Cost: Rs. " << fixed << setprecision(2) << cost << "\n";
    cout << "==========================================\n" << RESET;
}

/**********************************************************************************************
KMP STRING MATCHING UTILS
**********************************************************************************************/
vector<int> buildLPS(const string &pattern){
    int m = pattern.size();
    vector<int> lps(m, 0);
    for(int i=1, len=0; i<m;){
        if(pattern[i] == pattern[len]) lps[i++] = ++len;
        else if(len != 0) len = lps[len-1];
        else lps[i++] = 0;
    }
    return lps;
}

bool KMPsearch(const string &text, const string &pattern){
    if(pattern.empty()) return true;
    vector<int> lps = buildLPS(pattern);
    int i=0, j=0;

    while(i < text.size()){
        if(toupper(text[i]) == toupper(pattern[j])) i++, j++;
        if(j == pattern.size()) return true;
        if(i < text.size() && toupper(text[i]) != toupper(pattern[j])){
            if(j != 0) j = lps[j-1];
            else i++;
        }
    }
    return false;
}

/**********************************************************************************************
ZONE MANAGEMENT OPERATIONS
**********************************************************************************************/
void zoneManagement(){
    int choice;
    do {
        system("cls");
        cout << BLUE << "=============== ZONE MANAGEMENT ===============\n" << RESET;
        cout << "1. Search Zone (By Name or ID)\n";
        cout << "2. Update Zone Waste Level\n";
        cout << "3. Add New Zone\n";
        cout << "4. Display Zone Details\n";
        cout << "5. List All Zones\n";
        cout << "0. Return to Main Menu\n";
        cout << "==============================================\n";
        cout << "Enter choice: ";
        cin >> choice;

        if(choice == 1){
            cin.ignore(numeric_limits<streamsize>::max(), '\n');
            cout << CYAN << "Enter zone name or ID to search: " << RESET;
            string input;
            getline(cin, input);

            bool isNumber = true;
            for(char c : input) if(!isdigit(c)) isNumber = false;

            if(isNumber){
                int zid = stoi(input);
                if(zid >= 0 && zid < zones.size()){
                    cout << GREEN << "\nZone Found:\n" << RESET;
                    cout << "ID: " << zid << "\n";
                    cout << "Name: " << zones[zid].name << "\n";
                    cout << "Waste Level: " << zones[zid].currentWaste << "%\n";
                    cout << "Priority: " << zones[zid].priority << "\n";
                } else {
                    cout << RED << "Invalid Zone ID.\n" << RESET;
                }
            }
            else {
                for(char &c : input) c = toupper(c);
                vector<int> matches = lookupTable.findAllMatches(input);

                if(matches.empty()){
                    cout << RED << "No matching zone found.\n" << RESET;
                    cout << YELLOW << "Suggestions: \n" << RESET;
                    for(auto &z : zones) cout << " - " << z.name << "\n";
                }
                else {
                    int zid;
                    if(matches.size() == 1) zid = matches[0];
                    else {
                        cout << YELLOW << "Multiple matches found:\n" << RESET;
                        for(int i=0;i<matches.size();i++)
                            cout << "(" << i+1 << ") " << zones[matches[i]].name << "\n";
                        cout << "Select: ";
                        int pick; cin >> pick;
                        zid = matches[pick-1];
                    }

                    cout << GREEN << "\nZone Found:\n" << RESET;
                    cout << "ID: " << zones[zid].id << "\n";
                    cout << "Name: " << zones[zid].name << "\n";
                    cout << "Waste Level: " << zones[zid].currentWaste << "%\n";
                    cout << "Priority: " << zones[zid].priority << "\n";
                }
            }
        }
        else if(choice == 2){
            cin.ignore(numeric_limits<streamsize>::max(), '\n');
            cout << CYAN << "Enter zone name to update (partial/full): " << RESET;
            string input;
            getline(cin, input);

            for(char &c : input) c = toupper(c);

            vector<int> matches = lookupTable.findAllMatches(input);

            if(matches.empty()){
                cout << RED << "Zone not found! Try again.\n" << RESET;
                cout << YELLOW << "Suggestions:\n" << RESET;
                for(auto &z : zones) cout << "  - " << z.name << "\n";
                continue;
            }

            int zid = -1;

            if(matches.size() > 1){
                cout << YELLOW << "\nMultiple matches found for \"" << input << "\"\n" << RESET;
                for(int i=0; i<matches.size(); i++)
                    cout << "(" << i+1 << ") " << zones[matches[i]].name << "\n";

                cout << CYAN << "Select number: " << RESET;
                int pick; cin >> pick;
                if(pick < 1 || pick > matches.size()){
                    cout << RED << "Invalid choice.\n" << RESET;
                    continue;
                }
                zid = matches[pick-1];
            }
            else {
                zid = matches[0];
            }

            cout << GREEN << "\nUpdating: " << zones[zid].name << RESET << "\n";
            cout << "Current Waste: " << zones[zid].currentWaste << "%\n";

            cout << CYAN << "Enter new waste level (0-100): " << RESET;
            int newWaste;
            cin >> newWaste;

            if(newWaste < 0 || newWaste > 100){
                cout << RED << "Invalid waste level. Must be 0-100.\n" << RESET;
                continue;
            }

            zones[zid].currentWaste = newWaste;
            zones[zid].history.push_back(newWaste);
            if(zones[zid].history.size() > MAX_HISTORY_SIZE)
                zones[zid].history.erase(zones[zid].history.begin());

            if(newWaste > 80) zones[zid].priority = 1;
            else if(newWaste > 50) zones[zid].priority = 2;
            else zones[zid].priority = 3;

            cout << GREEN << "\nZone updated successfully!\n" << RESET;
            cout << "New Priority: " << zones[zid].priority << "\n";
        }
        else if(choice == 3){
            cin.ignore(numeric_limits<streamsize>::max(), '\n');
            cout << CYAN << "Enter NEW zone name: " << RESET;
            string name;
            getline(cin, name);

            int newId = zones.size();
            Zone newZone(newId, name, rand()%40 + 30);

            newZone.priority = (newZone.currentWaste > 80 ? 1 :
                               newZone.currentWaste > 50 ? 2 : 3);

            zones.push_back(newZone);
            id[name] = newId;
            lookupTable.insert(newId, name);

            cout << GREEN << "Zone Added! ID: " << newId << RESET << "\n";
            cout << YELLOW << "Add road connections manually later.\n" << RESET;
        }
        else if(choice == 4){
            cin.ignore(numeric_limits<streamsize>::max(), '\n');
            cout << CYAN << "Enter zone name (partial/full): " << RESET;
            string input;
            getline(cin, input);

            for(char &c : input) c = toupper(c);

            vector<int> matches = lookupTable.findAllMatches(input);

            if(matches.empty()){
                cout << RED << "\nZone not found.\n" << RESET;
                cout << YELLOW << "Suggestions:\n" << RESET;
                for(auto &z : zones) cout << " - " << z.name << "\n";
                continue;
            }

            int zid = -1;

            if(matches.size() > 1){
                cout << YELLOW << "\nMultiple matching zones found:\n" << RESET;
                for(int i=0; i<matches.size(); i++)
                    cout << "(" << i+1 << ") " << zones[matches[i]].name << "\n";

                cout << CYAN << "Select number: " << RESET;
                int pick;
                cin >> pick;

                if(pick < 1 || pick > matches.size()){
                    cout << RED << "Invalid choice.\n" << RESET;
                    continue;
                }
                zid = matches[pick - 1];
            }
            else {
                zid = matches[0];
            }

            Zone &z = zones[zid];

            cout << GREEN << "\n========== ZONE DETAILS ==========\n" << RESET;
            cout << left << setw(15) << "ID:"         << z.id << "\n";
            cout << left << setw(15) << "Name:"       << z.name << "\n";
            cout << left << setw(15) << "Waste:"      << z.currentWaste << "%\n";
            cout << left << setw(15) << "Priority:"   << z.priority << "\n";
            cout << left << setw(15) << "History:"    << z.history.size() << " entries\n";
            cout << "=====================================\n\n";
        }
        else if(choice == 5){
            showZones();
        }

        if(choice != 0){
            cout << "\nPress ENTER to continue...";
            cin.ignore();
            cin.get();
        }

    } while(choice != 0);
}

/**********************************************************************************************
WASTE TREND ANALYSIS USING SEGMENT TREE
**********************************************************************************************/
void wasteTrendAnalysis(){
    system("cls");
    cout << BLUE << "=============== WASTE TREND ANALYSIS ===============\n" << RESET;

    if(segmentTrees.empty()) {
        cout << YELLOW << "Segment trees not initialized.\n" << RESET;
        return;
    }

    cout << left << setw(COLUMN_WIDTH) << "ZONE NAME"
         << setw(15) << "CURRENT"
         << setw(15) << "MAX (HIST)"
         << setw(15) << "TREND\n";
    cout << "--------------------------------------------------------------\n";

    for(int i=0; i<zones.size(); i++) {
        Zone& z = zones[i];
        if(segmentTrees[i] != nullptr && z.history.size() > 1) {
            int maxHist = segmentTrees[i]->getMax();
            int trend = 0;
            if(z.history.size() >= 2) {
                trend = z.currentWaste - z.history[z.history.size()-2];
            }

            string trendStr = (trend > 0) ? "+" + to_string(trend) : to_string(trend);
            string trendCol = (trend > 5) ? RED : (trend > 0) ? YELLOW : GREEN;

            cout << left << setw(COLUMN_WIDTH) << z.name
                 << setw(15) << z.currentWaste << "%"
                 << setw(15) << maxHist << "%"
                 << trendCol << trendStr << "%" << RESET << "\n";
        } else {
            cout << left << setw(COLUMN_WIDTH) << z.name
                 << setw(15) << z.currentWaste << "%"
                 << setw(15) << "N/A"
                 << setw(15) << "N/A\n";
        }
    }

    cout << "--------------------------------------------------------------\n";
}

/**********************************************************************************************
ADVANCED ROUTE OPTIMIZATION
**********************************************************************************************/
Route optimizedRouteGreedy(int start, vector<int>& targets) {
    Route route;
    if(targets.empty()) return route;

    vector<bool> visited(zones.size(), false);
    route.path.push_back(start);
    visited[start] = true;
    int current = start;
    int totalDist = 0;

    targets.erase(remove(targets.begin(), targets.end(), start), targets.end());

    while(!targets.empty()) {
        int nearest = -1;
        int minDist = INF;

        for(int target : targets) {
            if(!visited[target] && dist[current][target] < minDist) {
                minDist = dist[current][target];
                nearest = target;
            }
        }

        if(nearest == -1) break;

        totalDist += minDist;
        route.path.push_back(nearest);
        visited[nearest] = true;
        current = nearest;
        targets.erase(remove(targets.begin(), targets.end(), nearest), targets.end());
    }

    route.totalDistance = totalDist;
    route.totalCost = (totalDist / TRUCK_MILEAGE) * FUEL_PRICE;
    for(int zid : route.path) {
        route.totalWaste += zones[zid].currentWaste;
    }

    return route;
}

Route optimizedRoutePriority(int start, vector<int>& targets) {
    Route route;
    if(targets.empty()) return route;

    sort(targets.begin(), targets.end(), [this](int a, int b) {
        return zones[a].currentWaste > zones[b].currentWaste;
    });

    route.path.push_back(start);
    int current = start;
    int totalDist = 0;

    for(int target : targets) {
        if(target != start) {
            totalDist += dist[current][target];
            route.path.push_back(target);
            current = target;
        }
    }

    route.totalDistance = totalDist;
    route.totalCost = (totalDist / TRUCK_MILEAGE) * FUEL_PRICE;
    for(int zid : route.path) {
        route.totalWaste += zones[zid].currentWaste;
    }

    return route;
}

void advancedRouteOptimization(){
    cin.ignore(numeric_limits<streamsize>::max(), '\n');

    cout << BLUE << "=============== ADVANCED ROUTE OPTIMIZATION ===============\n" << RESET;
    cout << CYAN << "Enter starting zone (partial/full): " << RESET;
    string startInput;
    getline(cin, startInput);

    for(char &c : startInput) c = toupper(c);

    vector<int> matches;
    for(int i=0; i<zones.size(); i++){
        string name = zones[i].name;
        for(char &c : name) c = toupper(c);
        if(name.find(startInput) != string::npos)
            matches.push_back(i);
    }

    if(matches.empty()){
        cout << RED << "\nNo matching zone found. Try again.\n" << RESET;
        return;
    }

    cout << YELLOW << "\nSuggestions:\n" << RESET;
    for(int i=0; i<matches.size(); i++)
        cout << i+1 << ". " << zones[matches[i]].name << "\n";

    cout << CYAN << "Select starting point number: " << RESET;
    int choice; cin >> choice;

    if(choice < 1 || choice > matches.size()){
        cout << RED << "Invalid choice.\n" << RESET;
        return;
    }

    int start = matches[choice-1];

    cout << CYAN << "\nSelect optimization strategy:\n" << RESET;
    cout << "1. Greedy Nearest Neighbor\n";
    cout << "2. Priority-based (Highest Waste First)\n";
    cout << "3. Compare Both Strategies\n";
    cout << "Enter choice: ";
    int strategy;
    cin >> strategy;

    vector<int> targets;
    for(int i=0; i<zones.size(); i++){
        if(i != start && zones[i].currentWaste > 50 && i != dumpingYard){
            targets.push_back(i);
        }
    }

    if(targets.empty()){
        cout << YELLOW << "No high-priority zones to optimize.\n" << RESET;
        return;
    }

    system("cls");
    cout << BLUE << "=============== OPTIMIZATION RESULTS ===============\n" << RESET;

    if(strategy == 1 || strategy == 3){
        vector<int> temp = targets;
        Route R = optimizedRouteGreedy(start, temp);

        cout << GREEN << "\n--- GREEDY NEAREST NEIGHBOR ROUTE ---\n" << RESET;
        cout << "Distance: " << R.totalDistance << " km\n";
        cout << "Cost: Rs. " << fixed << setprecision(2) << R.totalCost << "\n";
        cout << "Stops: " << R.path.size() << "\n";
        cout << "Route Order: ";
        for(int id : R.path) cout << zones[id].name << " -> ";
        cout << "END\n";
    }

    if(strategy == 2 || strategy == 3){
        vector<int> temp = targets;
        Route R = optimizedRoutePriority(start, temp);

        cout << MAGENTA << "\n--- PRIORITY-BASED ROUTE (Highest Waste First) ---\n" << RESET;
        cout << "Distance: " << R.totalDistance << " km\n";
        cout << "Cost: Rs. " << fixed << setprecision(2) << R.totalCost << "\n";
        cout << "Stops: " << R.path.size() << "\n";
        cout << "Route Order: ";
        for(int id : R.path) cout << zones[id].name << " -> ";
        cout << "END\n";
    }

    if(strategy == 3){
        vector<int> t1 = targets, t2 = targets;
        Route G = optimizedRouteGreedy(start, t1);
        Route P = optimizedRoutePriority(start, t2);

        cout << CYAN << "\n--- STRATEGY COMPARISON ---\n" << RESET;

        if(G.totalDistance < P.totalDistance)
            cout << GREEN  << "Greedy route is shorter by " << (P.totalDistance - G.totalDistance) << " km\n" << RESET;
        else
            cout << GREEN  << "Priority route is shorter by " << (G.totalDistance - P.totalDistance) << " km\n" << RESET;

        if(G.totalCost < P.totalCost)
            cout << YELLOW << "Greedy saves Rs. " << (P.totalCost - G.totalCost) << "\n" << RESET;
        else
            cout << YELLOW << "Priority saves Rs. " << (G.totalCost - P.totalCost) << "\n" << RESET;
    }

    cout << "\n====================================================\n" << RESET;
}

/**********************************************************************************************
COST ANALYSIS AND REPORTING
**********************************************************************************************/
void costAnalysis(){
    system("cls");
    cout << BLUE << "=============== COST ANALYSIS REPORT ===============\n" << RESET;

    int totalZones = zones.size();
    int totalWaste = 0;
    int criticalCount = 0;

    for(auto& zone : zones) {
        totalWaste += zone.currentWaste;
        if(zone.currentWaste > 80) criticalCount++;
    }

    double avgWaste = (double)totalWaste / totalZones;

    int collectionsNeeded = 0;
    int totalDistanceEstimate = 0;

    for(auto& zone : zones) {
        if(zone.currentWaste > 50) {
            collectionsNeeded++;
            totalDistanceEstimate += 5;
        }
    }

    double estimatedFuel = (double)totalDistanceEstimate / TRUCK_MILEAGE;
    double estimatedCost = estimatedFuel * FUEL_PRICE;

    cout << left << setw(35) << "Total Zones:" << totalZones << "\n";
    cout << left << setw(35) << "Average Waste Level:" << fixed << setprecision(2) << avgWaste << "%\n";
    cout << left << setw(35) << "Critical Zones (>80%):" << criticalCount << "\n";
    cout << left << setw(35) << "Zones Requiring Collection (>50%):" << collectionsNeeded << "\n";
    cout << "\n" << CYAN << "ESTIMATED COLLECTION COSTS:\n" << RESET;
    cout << left << setw(35) << "Estimated Total Distance:" << totalDistanceEstimate << " km\n";
    cout << left << setw(35) << "Estimated Fuel Required:" << fixed << setprecision(2) << estimatedFuel << " liters\n";
    cout << left << setw(35) << "Estimated Total Cost:" << "Rs. " << fixed << setprecision(2) << estimatedCost << "\n";

    cout << "\n" << YELLOW << "COST BREAKDOWN PER ZONE:\n" << RESET;
    cout << left << setw(COLUMN_WIDTH) << "ZONE NAME"
         << setw(15) << "WASTE (%)"
         << setw(15) << "EST. COST\n";
    cout << "--------------------------------------------------------------\n";

    for(auto& zone : zones) {
        if(zone.currentWaste > 50) {
            double zoneCost = (5.0 / TRUCK_MILEAGE) * FUEL_PRICE;
            cout << left << setw(COLUMN_WIDTH) << zone.name
                 << setw(15) << zone.currentWaste << "%"
                 << "Rs. " << fixed << setprecision(2) << zoneCost << "\n";
        }
    }

    cout << "--------------------------------------------------------------\n";
    cout << "\n" << GREEN << "RECOMMENDATION:\n" << RESET;
    if(criticalCount > totalZones * 0.3) {
        cout << RED << "High number of critical zones. Immediate action required!\n" << RESET;
    } else if(criticalCount > 0) {
        cout << YELLOW << "Some critical zones detected. Schedule collection soon.\n" << RESET;
    } else {
        cout << GREEN << "System is in good condition. Regular collection sufficient.\n" << RESET;
    }
}

/**********************************************************************************************
HISTORICAL DATA ANALYSIS
**********************************************************************************************/
void historicalDataAnalysis(){
    system("cls");
    cout << BLUE << "=============== HISTORICAL DATA ANALYSIS ===============\n" << RESET;

    cin.ignore(numeric_limits<streamsize>::max(), '\n');
    cout << CYAN << "Enter zone name for analysis (partial/full): " << RESET;

    string input;
    getline(cin, input);

    for(char &c : input) c = toupper(c);

    vector<int> matches;
    for(int i=0; i<zones.size(); i++){
        string name = zones[i].name;
        if(KMPsearch(name, input)){
            matches.push_back(i);
        }
    }

    int zid;
    if(matches.size() > 1){
        cout << YELLOW << "\nMultiple matches found. Please choose:\n" << RESET;
        for(int i = 0; i < matches.size(); i++)
            cout << i+1 << ". " << zones[matches[i]].name << "\n";

        cout << CYAN << "\nEnter choice: " << RESET;
        int ch; cin >> ch;

        if(ch < 1 || ch > matches.size()){
            cout << RED << "Invalid selection.\n" << RESET;
            return;
        }
        zid = matches[ch-1];
    } else if(matches.size() == 1) {
        zid = matches[0];
    } else {
        cout << RED << "No matching zone found.\n" << RESET;
        return;
    }

    Zone &z = zones[zid];

    if(z.history.size() < 2){
        cout << YELLOW << "Not enough historical data to analyze.\n" << RESET;
        return;
    }

    int minVal = *min_element(z.history.begin(), z.history.end());
    int maxVal = *max_element(z.history.begin(), z.history.end());
    int sum = accumulate(z.history.begin(), z.history.end(), 0);
    double avg = (double)sum / z.history.size();
    double trend = (double)(z.history.back() - z.history.front()) / (z.history.size() - 1);

    system("cls");
    cout << GREEN << "============ ANALYSIS REPORT: " << z.name << " ============\n" << RESET;

    cout << left << setw(30) << "Total Records:"      << z.history.size() << "\n";
    cout << left << setw(30) << "Minimum Waste:"       << minVal << "%\n";
    cout << left << setw(30) << "Maximum Waste:"       << maxVal << "%\n";
    cout << left << setw(30) << "Average Waste:"       << fixed << setprecision(2) << avg << "%\n";
    cout << left << setw(30) << "Current Waste:"       << z.currentWaste << "%\n";

    cout << left << setw(30) << "Overall Trend:"
         << (trend > 0 ? "+" : "")
         << fixed << setprecision(2) << trend << "% change per reading\n";

    cout << "\n" << CYAN << "Recent 10 Records:\n" << RESET;
    int start = max(0, (int)z.history.size() - 10);
    for(int i = start; i < z.history.size(); i++){
        cout << "Record " << (i-start+1) << ": " << z.history[i] << "%\n";
    }

    cout << "\n" << BLUE << "String Matching Used:" << RESET << "\n";
    cout << "Matching Algorithm Used: KMP (Knuth–Morris–Pratt)\n";
    cout << "Time Complexity: O(N + M)\n";
    cout << "N = length of zone name, M = search input length\n";

    cout << "• Example: Checking if '" << input << "' exists inside zone name\n";

    cout << "\n=====================================================\n";
}

/**********************************************************************************************
BATCH ZONE UPDATE
**********************************************************************************************/
void batchZoneUpdate(){
    system("cls");
    cout << BLUE << "=============== BATCH ZONE UPDATE ===============\n" << RESET;

    cin.ignore(numeric_limits<streamsize>::max(), '\n');
    cout << CYAN << "Enter number of zones to update: " << RESET;
    int count;
    cin >> count;

    if(count <= 0 || count > zones.size()) {
        cout << RED << "Invalid count.\n" << RESET;
        return;
    }

    int updated = 0;

    for(int i = 0; i < count; i++) {

        cin.ignore(numeric_limits<streamsize>::max(), '\n');
        cout << "\nZone " << (i + 1) << ":\n";
        cout << CYAN << "Enter zone name (partial or full): " << RESET;
        string input;
        getline(cin, input);

        for(char &c : input) c = toupper(c);

        vector<int> matches;
        for(int z = 0; z < zones.size(); z++){
            string temp = zones[z].name;
            for(char &c : temp) c = toupper(c);
            if(temp.find(input) != string::npos){
                matches.push_back(z);
            }
        }

        if(matches.empty()){
            cout << YELLOW << "No matching zone found. Skipping.\n" << RESET;
            continue;
        }

        int zid;
        if(matches.size() > 1){
            cout << YELLOW << "Multiple matches found, choose one:" << RESET << "\n";
            for(int j = 0; j < matches.size(); j++)
                cout << (j + 1) << ". " << zones[matches[j]].name << "\n";

            int ch;
            cout << CYAN << "Enter choice: " << RESET;
            cin >> ch;

            if(ch < 1 || ch > matches.size()){
                cout << RED << "Invalid choice. Skipping.\n" << RESET;
                continue;
            }
            zid = matches[ch - 1];
        }
        else {
            zid = matches[0];
        }

        cout << "Current waste: " << zones[zid].currentWaste << "%\n";
        cout << CYAN << "Enter new waste level (0-100): " << RESET;
        int newWaste;
        cin >> newWaste;

        if(newWaste >= 0 && newWaste <= 100){
            zones[zid].currentWaste = newWaste;
            zones[zid].history.push_back(newWaste);
            updated++;
            cout << GREEN << "Updated successfully.\n" << RESET;
        }
        else{
            cout << RED << "Invalid waste level. Skipped.\n" << RESET;
        }
    }

    cout << GREEN << "\nBatch update complete. " << updated << " zones updated.\n" << RESET;
}

/**********************************************************************************************
ADVANCED ZONE SEARCH
**********************************************************************************************/
void advancedZoneSearch(){
    system("cls");
    cout << BLUE << "=============== ADVANCED ZONE SEARCH ===============\n" << RESET;

    cout << CYAN << "Search filters:\n" << RESET;
    cout << "1. By Waste Level Range\n";
    cout << "2. By Priority Level\n";
    cout << "3. By Name Pattern\n";
    cout << "4. Critical Zones Only\n";
    cout << "Enter filter type: ";

    int filterType;
    cin >> filterType;

    vector<int> results;

    switch(filterType) {
        case 1: {
            int minWaste, maxWaste;
            cout << CYAN << "Enter minimum waste level: " << RESET;
            cin >> minWaste;
            cout << CYAN << "Enter maximum waste level: " << RESET;
            cin >> maxWaste;

            for(int i = 0; i < zones.size(); i++) {
                if(zones[i].currentWaste >= minWaste && zones[i].currentWaste <= maxWaste) {
                    results.push_back(i);
                }
            }
            break;
        }
        case 2: {
            int priority;
            cout << CYAN << "Enter minimum priority: " << RESET;
            cin >> priority;

            for(int i = 0; i < zones.size(); i++) {
                if(zones[i].priority >= priority) {
                    results.push_back(i);
                }
            }
            break;
        }
        case 3: {
            cin.ignore(numeric_limits<streamsize>::max(), '\n');
            cout << CYAN << "Enter name pattern: " << RESET;
            string pattern;
            getline(cin, pattern);

            results = lookupTable.findAllMatches(pattern);
            break;
        }
        case 4: {
            for(int i = 0; i < zones.size(); i++) {
                if(zones[i].currentWaste > 80) {
                    results.push_back(i);
                }
            }
            break;
        }
        default:
            cout << RED << "Invalid filter type.\n" << RESET;
            return;
    }

    if(results.empty()) {
        cout << YELLOW << "No zones match the criteria.\n" << RESET;
        return;
    }

    cout << GREEN << "\nFound " << results.size() << " zone(s):\n" << RESET;
    cout << left << setw(COLUMN_WIDTH) << "ZONE NAME"
         << setw(15) << "WASTE (%)"
         << "PRIORITY\n";
    cout << "--------------------------------------------------------------\n";

    for(int zid : results) {
        string col = GREEN;
        if(zones[zid].currentWaste > 80) col = RED;
        else if(zones[zid].currentWaste > 50) col = YELLOW;

        cout << left << setw(COLUMN_WIDTH) << zones[zid].name
             << col << setw(15) << zones[zid].currentWaste << "%" << RESET
             << zones[zid].priority << "\n";
    }

    cout << "--------------------------------------------------------------\n";
}

/**********************************************************************************************
ROUTE EFFICIENCY CALCULATOR
**********************************************************************************************/
void routeEfficiencyCalculator(){
    system("cls");
    cout << BLUE << "=============== ROUTE EFFICIENCY CALCULATOR ===============\n" << RESET;

    if(savedRoutes.empty()) {
        cout << YELLOW << "No saved routes available. Generate routes first.\n" << RESET;
        return;
    }

    cout << CYAN << "Select route number (1-" << savedRoutes.size() << "): " << RESET;
    int routeNum;
    cin >> routeNum;

    if(routeNum < 1 || routeNum > savedRoutes.size()) {
        cout << RED << "Invalid route number.\n" << RESET;
        return;
    }

    Route& r = savedRoutes[routeNum - 1];

    if(r.path.empty()) {
        cout << RED << "Route is empty.\n" << RESET;
        return;
    }

    double avgWastePerKm = (double)r.totalWaste / max(1.0, (double)r.totalDistance);
    double costPerWaste  = r.totalCost / max(1.0, (double)r.totalWaste);
    double efficiency    = (double)r.totalWaste / max(1.0, r.totalCost);

    cout << GREEN << "\n========== EFFICIENCY METRICS ==========\n" << RESET;
    cout << "Route #" << routeNum << " Efficiency Analysis:\n\n";
    cout << left << setw(35) << "Total Distance:" << r.totalDistance << " km\n";
    cout << left << setw(35) << "Total Cost:" << "Rs. " << fixed << setprecision(2) << r.totalCost << "\n";
    cout << left << setw(35) << "Total Waste Collected:" << r.totalWaste << " units\n";
    cout << left << setw(35) << "Number of Stops:" << r.path.size() << "\n";
    cout << "\n" << CYAN << "EFFICIENCY INDICATORS:\n" << RESET;
    cout << left << setw(35) << "Waste per Kilometer:" << fixed << setprecision(3) << avgWastePerKm << " units/km\n";
    cout << left << setw(35) << "Cost per Waste Unit:" << "Rs. " << fixed << setprecision(2) << costPerWaste << "\n";
    cout << left << setw(35) << "Efficiency Ratio:" << fixed << setprecision(3) << efficiency << " units/Rs.\n";

    string rating;
    string ratingColor;
    if(efficiency > 5.0) {
        rating = "EXCELLENT";
        ratingColor = GREEN;
    } else if(efficiency > 3.0) {
        rating = "GOOD";
        ratingColor = CYAN;
    } else if(efficiency > 1.5) {
        rating = "AVERAGE";
        ratingColor = YELLOW;
    } else {
        rating = "POOR";
        ratingColor = RED;
    }

    cout << "\n" << ratingColor << "Overall Efficiency Rating: " << rating << RESET << "\n";
    cout << "===========================================\n" << RESET;
}

/**********************************************************************************************
SIMULATE WASTE GENERATION
**********************************************************************************************/
void simulateWasteGeneration(){
    system("cls");
    cout << BLUE << "=============== WASTE GENERATION SIMULATOR ===============\n" << RESET;

    cout << CYAN << "Enter number of days to simulate: " << RESET;
    int days;
    cin >> days;

    if(days <= 0 || days > 30) {
        cout << RED << "Invalid number of days. Please enter 1-30.\n" << RESET;
        return;
    }

    cout << CYAN << "Enter daily growth rate percentage (0-20): " << RESET;
    double growthRate;
    cin >> growthRate;

    if(growthRate < 0 || growthRate > 20) {
        cout << RED << "Invalid growth rate.\n" << RESET;
        return;
    }

    growthRate = growthRate / 100.0;

    cout << GREEN << "\nSimulating " << days << " days with " << (growthRate*100) << "% daily growth...\n\n" << RESET;

    vector<int> initialWaste(zones.size());
    for(int i = 0; i < zones.size(); i++) {
        initialWaste[i] = zones[i].currentWaste;
    }

    for(int day = 1; day <= days; day++) {
        for(int i = 0; i < zones.size(); i++) {
            if(zones[i].id != dumpingYard) {
                int increase = (int)(zones[i].currentWaste * growthRate) + (rand() % 5);
                zones[i].currentWaste = min(100, zones[i].currentWaste + increase);
                zones[i].history.push_back(zones[i].currentWaste);
                if(zones[i].history.size() > MAX_HISTORY_SIZE) {
                    zones[i].history.erase(zones[i].history.begin());
                }
            }
        }
    }

    cout << left << setw(COLUMN_WIDTH) << "ZONE NAME"
         << setw(15) << "INITIAL"
         << setw(15) << "FINAL"
         << "CHANGE\n";
    cout << "--------------------------------------------------------------\n";

    for(int i = 0; i < zones.size(); i++) {
        if(zones[i].id != dumpingYard) {
            int change = zones[i].currentWaste - initialWaste[i];
            string changeStr = (change > 0 ? "+" : "") + to_string(change);
            string changeCol = (change > 20) ? RED : (change > 0) ? YELLOW : GREEN;

            cout << left << setw(COLUMN_WIDTH) << zones[i].name
                 << setw(15) << initialWaste[i] << "%"
                 << setw(15) << zones[i].currentWaste << "%"
                 << changeCol << changeStr << "%" << RESET << "\n";
        }
    }

    cout << "--------------------------------------------------------------\n";
    cout << GREEN << "\nSimulation complete!\n" << RESET;
}

/**********************************************************************************************
GRAPH VISUALIZATION DATA EXPORT
**********************************************************************************************/
void exportGraphData(){
    system("cls");
    cout << BLUE << "=============== GRAPH DATA EXPORT ===============\n" << RESET;

    cout << CYAN << "Export format:\n" << RESET;
    cout << "1. Adjacency List\n";
    cout << "2. Distance Matrix\n";
    cout << "3. Zone Coordinates\n";
    cout << "Enter choice: ";

    int choice;
    cin >> choice;

    cout << "\n" << GREEN << "EXPORT DATA:\n" << RESET;
    cout << "==============================================\n";

    switch(choice) {
        case 1: {
            cout << "ADJACENCY LIST FORMAT:\n";
            for(int i = 0; i < zones.size(); i++) {
                cout << zones[i].name << " -> ";
                for(int j = 0; j < adj[i].size(); j++) {
                    cout << zones[adj[i][j].to].name << "(" << adj[i][j].weight << "km)";
                    if(j < adj[i].size() - 1) cout << ", ";
                }
                cout << "\n";
            }
            break;
        }
        case 2: {
            cout << "DISTANCE MATRIX (Floyd-Warshall):\n";
            cout << "Zones: ";
            for(int i = 0; i < zones.size(); i++) {
                cout << zones[i].name;
                if(i < zones.size() - 1) cout << ", ";
            }
            cout << "\n\nMatrix:\n";
            for(int i = 0; i < zones.size(); i++) {
                for(int j = 0; j < zones.size(); j++) {
                    if(dist[i][j] == INF) cout << "INF\t";
                    else cout << dist[i][j] << "\t";
                }
                cout << "\n";
            }
            break;
        }
        case 3: {
            cout << "ZONE COORDINATES:\n";
            for(int i = 0; i < zones.size(); i++) {
                cout << zones[i].name << ": (" << zones[i].x << ", " << zones[i].y << ")\n";
            }
            break;
        }
        default:
            cout << RED << "Invalid choice.\n" << RESET;
            return;
    }

    cout << "==============================================\n";
    cout << YELLOW << "\nNote: This is display-only. For file export, implement file I/O.\n" << RESET;
}

/**********************************************************************************************
LOGIN FUNCTIONS
**********************************************************************************************/
void adminLogin(){
    string u,p;
    cout<<"---ADMINLOGIN---\n";
    cout<<"USERNAME :";
    cin>>u;
    cout<<"PASSWORD :";
    cin>>p;
    if(u=="admin"&&p=="admin@123"){
        currentRole="admin";
        loggedIn=true;
        cout << GREEN << "Login successful!\n" << RESET;
    } else {
        cout << RED << "Invalid credentials.\n" << RESET;
    }
}

void registerUser(string role){
    ofstream fp;
    if(role=="user")fp.open("users.txt",ios::app);
    else fp.open("drivers.txt",ios::app);
    string u,p;
    cout<<"USERNAME: ";
    cin>>u;
    cout<<"PASSWORD :";
    cin>>p;
    fp<<u<<" "<<p<<"\n";
    fp.close();
    cout << GREEN << "Registration successful!\n" << RESET;
}

bool loginUser(string role){
    ifstream fp;
    if(role=="user")fp.open("users.txt");
    else fp.open("drivers.txt");
    string u,p,fu,fpw;
    cout<<"USERNAME :";
    cin>>u;
    cout<<"PASSWORD :";
    cin>>p;
    while(fp>>fu>>fpw){
        if(u==fu&&p==fpw){
            currentRole=role;
            loggedIn=true;
            fp.close();
            cout << GREEN << "Login successful!\n" << RESET;
            return true;
        }
    }
    fp.close();
    cout<<RED<<"Invalid credentials.\n"<<RESET;
    return false;
}

void loginPage(){
    string role;
    int ch;
    cout<<"Enter your role(admin/user/driver): ";
    cin>>role;
    if(role=="admin"){
        adminLogin();
    }
    else if(role=="user"||role=="driver"){
        cout<<"1.LOGIN\n2.REGISTER: ";
        cin>>ch;
        if(ch==1)loginUser(role);
        if(ch==2)registerUser(role);
    } else {
        cout << RED << "Invalid role.\n" << RESET;
    }
}

/**********************************************************************************************
MAIN MENU (ASCII CLEAN)
**********************************************************************************************/
void menu(){
    int choice;

    do{
        system("cls");

        cout << BOLD << BLUE << "=============== SMART WASTE MANAGEMENT SYSTEM ===============\n" << RESET;
        cout << " 1. Show Zones & Status\n";
        cout << " 2. Priority Sort By Waste\n";
        cout << " 3. Predict Next Day Waste\n";
        cout << " 4. BFS Spread Simulation\n";
        cout << " 5. TSP-Lite Route (Greedy)\n";
        cout << " 6. Capacity Route (200 units -> Dump Yard)\n";
        cout << " 7. Drivers Route\n";
        cout << " 8. Weekly Plan (7-Day Growth)\n";
        cout << " 9. Google Maps Sample Link\n";
        cout << "10. Driver Income Calculator\n";
        cout << "11. Compare Routes\n";
        cout << "12. Statistical Analysis\n";
        cout << "13. Shortest Path Between Zones\n";

        cout << " 0. Exit Program\n";
        cout << "==============================================================\n";
        cout << "Enter choice: ";
        cin >> choice;

        system("cls");

        switch(choice){
            case 1: showZones(); break;
            case 2: prioritySort(); break;
            case 3: predictWaste(); break;
            case 4: bfsSpread(); break;
            case 5: tspLite(); break;
            case 6: capacityRoute(); break;
            case 7: driversRoute(); break;
            case 8: weeklyPlan(); break;
            case 9:
                cout << MAGENTA << "Google Maps Example Route:\n"
                     << "https://www.google.com/maps/@15.886057,74.5143525,17z?entry=ttu&g_ep=EgoyMDI1MTIwOS4wIKXMDSoASAFQAw%3D%3D"
                     << RESET << "\n";
                break;
            case 10: driverIncome(); break;
            case 11: compareRoutes(); break;
            case 12: statisticalAnalysis(); break;
            case 13: shortestPath(); break;
            case 14: zoneManagement(); break;
            case 15: wasteTrendAnalysis(); break;
            case 16: advancedRouteOptimization(); break;
            case 17: costAnalysis(); break;
            case 18: historicalDataAnalysis(); break;
            case 19: batchZoneUpdate(); break;
            case 20: advancedZoneSearch(); break;
            case 21: routeEfficiencyCalculator(); break;
            case 22: simulateWasteGeneration(); break;
            case 23: exportGraphData(); break;
            case 0: return;
        }
        
        if(choice != 0){
            cout << "\nPress ENTER to continue...";
            cin.ignore();
            cin.get();
        }
    } while(choice != 0);
}
};

/**********************************************************************************************
MAIN FUNCTION
**********************************************************************************************/
int main()
{
    WasteRoutePlanner app;
    app.loginPage();
    
    if(loggedIn == true && currentRole == "admin")
    {
        int choice;
        do{
            system("cls");

            cout << BOLD << BLUE << "=============== SMART WASTE MANAGEMENT SYSTEM ===============\n" << RESET;
            cout << " 1. Show Zones & Status\n";
            cout << " 2. Priority Sort By Waste\n";
            cout << " 3. Predict Next Day Waste\n";
            cout << " 4. BFS Spread Simulation\n";
            cout << " 5. TSP-Lite Route (Greedy)\n";
            cout << " 6. Capacity Route (200 units -> Dump Yard)\n";
            cout << " 7. Drivers Route\n";
            cout << " 8. Weekly Plan (7-Day Growth)\n";
            cout << " 9. Google Maps Sample Link\n";
            cout << "10. Driver Income Calculator\n";
            cout << "11. Compare Routes\n";
            cout << "12. Statistical Analysis\n";
            cout << "13. Shortest Path Between Zones\n";
            cout << "14. Zone Management (Insert/Update/Search)\n";
            cout << "15. Waste Trend Analysis\n";
            cout << "16. Advanced Route Optimization\n";
            cout << "17. Cost Analysis & Reporting\n";
            cout << "18. Historical Data Analysis\n";
            cout << "19. Batch Zone Update\n";
            cout << "20. Advanced Zone Search\n";
            cout << "21. Route Efficiency Calculator\n";
            cout << "22. Simulate Waste Generation\n";
            cout << "23. Export Graph Data\n";
            cout << "24. Display All Drivers\n";

            cout << " 0. Exit Program\n";
            cout << "==============================================================\n";
            cout << "Enter choice: ";
            cin >> choice;

            system("cls");

            switch(choice){
                case 1: app.showZones(); break;
                case 2: app.prioritySort(); break;
                case 3: app.predictWaste(); break;
                case 4: app.bfsSpread(); break;
                case 5: app.tspLite(); break;
                case 6: app.capacityRoute(); break;
                case 7: app.driversRoute(); break;
                case 8: app.weeklyPlan(); break;
                case 9:
                    cout << MAGENTA << "Google Maps Example Route:\n"
                         << "https://www.google.com/maps/@15.886057,74.5143525,17z?entry=ttu&g_ep=EgoyMDI1MTIwOS4wIKXMDSoASAFQAw%3D%3D"
                         << RESET << "\n";
                    break;
                case 10: app.driverIncome(); break;
                case 11: app.compareRoutes(); break;
                case 12: app.statisticalAnalysis(); break;
                case 13: app.shortestPath(); break;
                case 14: app.zoneManagement(); break;
                case 15: app.wasteTrendAnalysis(); break;
                case 16: app.advancedRouteOptimization(); break;
                case 17: app.costAnalysis(); break;
                case 18: app.historicalDataAnalysis(); break;
                case 19: app.batchZoneUpdate(); break;
                case 20: app.advancedZoneSearch(); break;
                case 21: app.routeEfficiencyCalculator(); break;
                case 22: app.simulateWasteGeneration(); break;
                case 23: app.exportGraphData(); break;
                case 24: app.displayAllDrivers(); break;

                case 0: return 0;
            }
            
            if(choice != 0){
                cout << "\nPress ENTER to continue...";
                cin.ignore();
                cin.get();
            }
        } while(choice != 0);
    }
    else if(loggedIn == true && currentRole == "user")
    {
        int choice1;
        do{
            system("cls");

            cout << BOLD << BLUE << "=============== SMART WASTE MANAGEMENT SYSTEM ===============\n" << RESET;
            cout << " 1. Show Zones & Status\n";
            cout << "20. Advanced Zone Search\n";
            cout << "22. Simulate Waste Generation\n";
            cout << "23. Export Graph Data\n";
            cout << " 0. Exit Program\n";
            cout << "==============================================================\n";
            cout << "Enter choice: ";
            cin >> choice1;

            system("cls");

            switch(choice1){
                case 1: app.showZones(); break;
                case 20: app.advancedZoneSearch(); break;
                case 22: app.simulateWasteGeneration(); break;
                case 23: app.exportGraphData(); break;
                case 0: return 0;
            }
            
            if(choice1 != 0){
                cout << "\nPress ENTER to continue...";
                cin.ignore();
                cin.get();
            }
        } while(choice1 != 0);
    }
    else if(loggedIn == true && currentRole == "driver")
    {
        int choice2;
        do{
            system("cls");

            cout << BOLD << BLUE << "=============== SMART WASTE MANAGEMENT SYSTEM ===============\n" << RESET;
            cout << " 7. Drivers Route\n";
            cout << " 8. Weekly Plan (7-Day Growth)\n";
            cout << " 9. Google Maps Sample Link\n";
            cout << "10. Driver Income Calculator\n";
            cout << "13. Shortest Path Between Zones\n";
            cout << "17. Cost Analysis & Reporting\n";
            cout << "21. Route Efficiency Calculator\n";
            cout << " 0. Exit Program\n";
            cout << "==============================================================\n";
            cout << "Enter choice: ";
            cin >> choice2;

            system("cls");

            switch(choice2){
                case 7: app.driversRoute(); break;
                case 8: app.weeklyPlan(); break;
                case 9:
                
                    cout << MAGENTA << "Google Maps Example Route:\n"
                         << "https://www.google.com/maps/@15.886057,74.5143525,17z?entry=ttu&g_ep=EgoyMDI1MTIwOS4wIKXMDSoASAFQAw%3D%3D"
                         << RESET << "\n";
                    break;
                case 10: app.driverIncome(); break;
                case 13: app.shortestPath(); break;
                case 17: app.costAnalysis(); break;
                case 21: app.routeEfficiencyCalculator(); break;
                case 0: return 0;
            }
            
            if(choice2 != 0){
                cout << "\nPress ENTER to continue...";
                cin.ignore();
                cin.get();
            }
        } while(choice2 != 0);
    }
    else 
    {
        cout << RED << "Invalid login. Please try again.\n" << RESET;
    }
    
    return 0;
}
