#include "Graph.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <cmath>
#include <algorithm>
#include <queue>
#include <limits>

using namespace std;

// ============================================================================
// GRAPH BASIC OPERATIONS
// ============================================================================

int Graph::findOrAddNode(double lat, double lon, string name) {
    // Search for existing node within tolerance (~10 meters)
    // This prevents creating duplicate nodes for slightly different GPS coordinates
    double tolerance = 0.0001;  // Approximately 10 meters
    
    for (int i = 0; i < nodes.size(); i++) {
        if (fabs(nodes[i].lat - lat) < tolerance &&
            fabs(nodes[i].lon - lon) < tolerance) {
            // Found existing node within tolerance
            // Update name if new name is not empty and current name is default
            if (!name.empty() && nodes[i].name.find("Node") == 0) {
                nodes[i].name = name;
            }
            return i;
        }
    }
    
    // Node doesn't exist, create new one
    int newId = nodes.size();
    if (name.empty()) {
        name = "Node" + to_string(newId);
    }
    nodes.push_back(Node(newId, lat, lon, name));
    
    // Extend adjacency list for new node
    adjList.push_back(vector<Edge>());
    
    return newId;
}

int Graph::findNearestNode(double lat, double lon) {
    // Find the closest node to given coordinates (for handling off-graph locations)
    
    if (nodes.empty()) return -1;
    
    int bestNode = -1;
    double bestDist = INF;
    
    for (int i = 0; i < nodes.size(); i++) {
        double dist = haversineDistance(lat, lon, nodes[i].lat, nodes[i].lon);
        if (dist < bestDist) {
            bestDist = dist;
            bestNode = i;
        }
    }
    
    return bestNode;    // returning the ID of the nearest node
}

void Graph::addEdge(int from, int to, Mode mode, double distance) {
    // Add a directed edge from 'from' node to 'to' node
    adjList[from].push_back(Edge(to, mode, distance));
}

void parseRoadmapCSV(Graph &graph, string filename) {
    // Parse roadmap CSV and build car road network
    // Format: DhakaStreet, lon1, lat1, lon2, lat2, ..., altitude, length
    
    ifstream file(filename);
    if (!file.is_open()) {
        cout << "Error: Could not open " << filename << endl;
        return;
    }
    
    string line;
    int roadCount = 0;
    
    while (getline(file, line)) {
        if (line.empty()) continue;
        
        //each line have : type, lon1, lat1, lon2, lat2, ...(more lon,lat), altitude, length

        // Split CSV line
        vector<string> tokens = splitCSV(line);
        
        // Need at least: type, 2 pairs, altitude, length = 6 tokens
        if (tokens.size() < 6) continue;
        
        // Last two tokens should be altitude and length (numbers)
        string altToken = tokens[tokens.size() - 2];
        string lenToken = tokens[tokens.size() - 1];
        
        if (!isNumber(altToken) || !isNumber(lenToken)) continue;
        
        // Coordinates are from tokens[1] to tokens[size-3]
        // They should come in pairs (longitude, latitude)
        int coordCount = tokens.size() - 3;  // Exclude type, altitude, length
        if (coordCount < 4 || coordCount % 2 != 0) continue;  // Need at least 2 pairs
        
        roadCount++;
        
        // Process consecutive coordinate pairs and create edges
        // tokens[1] = lon1, tokens[2] = lat1, tokens[3] = lon2, tokens[4] = lat2, etc.
        for (int i = 1; i + 3 <= tokens.size() - 2; i += 2) {
            double lon1 = stod(tokens[i]);
            double lat1 = stod(tokens[i + 1]);
            double lon2 = stod(tokens[i + 2]);
            double lat2 = stod(tokens[i + 3]);
            
            // Create or find nodes
            int from = graph.findOrAddNode(lat1, lon1);
            int to = graph.findOrAddNode(lat2, lon2);
            
            // Calculate actual distance between these two points
            double segmentDist = haversineDistance(lat1, lon1, lat2, lon2);
            
            // Add bidirectional edges (roads go both ways)
            graph.addEdge(from, to, MODE_CAR, segmentDist);
            graph.addEdge(to, from, MODE_CAR, segmentDist);
        }
    }
    
    file.close();
    cout << "Parsed " << roadCount << " road segments from " << filename << endl;
}

void parseMetroCSV(Graph &graph, string filename) {
    // Parse metro route CSV and build metro network
    // Format: DhakaMetroRail, lon1, lat1, lon2, lat2, ..., StartStation, EndStation
    
    ifstream file(filename);
    if (!file.is_open()) {
        cout << "Error: Could not open " << filename << endl;
        return;
    }
    
    string line;
    int routeCount = 0;
    
    while (getline(file, line)) {
        if (line.empty()) continue;
        
        vector<string> tokens = splitCSV(line);
        
        // Need at least: type, 1 coordinate pair, 2 station names = 5 tokens
        if (tokens.size() < 5) continue;
        
        // Last 2 tokens should be station names (non-numeric)
        string startStation = tokens[tokens.size() - 2];
        string endStation = tokens[tokens.size() - 1];
        
        if (isNumber(startStation) || isNumber(endStation)) continue;
        
        // Coordinates are from tokens[1] to tokens[size-3]
        int coordCount = tokens.size() - 3;
        if (coordCount < 4 || coordCount % 2 != 0) continue;
        
        routeCount++;
        
        // Process consecutive coordinate pairs
        for (int i = 1; i + 3 <= tokens.size() - 2; i += 2) {
            double lon1 = stod(tokens[i]);
            double lat1 = stod(tokens[i + 1]);
            double lon2 = stod(tokens[i + 2]);
            double lat2 = stod(tokens[i + 3]);
            
            // Create or find nodes, set station names for first and last
            string name1 = (i == 1) ? startStation : "";
            string name2 = (i + 4 > tokens.size() - 2) ? endStation : "";
            
            int from = graph.findOrAddNode(lat1, lon1, name1);
            int to = graph.findOrAddNode(lat2, lon2, name2);
            
            double segmentDist = haversineDistance(lat1, lon1, lat2, lon2);
            
            // Add bidirectional metro edges
            graph.addEdge(from, to, MODE_METRO, segmentDist);
            graph.addEdge(to, from, MODE_METRO, segmentDist);
        }
    }
    
    file.close();
    cout << "Parsed " << routeCount << " metro routes from " << filename << endl;
}

void parseBusCSV(Graph &graph, string filename, Mode busMode) {
    // Parse bus route CSV and build bus network
    // Format: BusType, lon1, lat1, lon2, lat2, ..., StartStation, EndStation
    
    ifstream file(filename);
    if (!file.is_open()) {
        cout << "Error: Could not open " << filename << endl;
        return;
    }
    
    string line;
    int routeCount = 0;
    
    while (getline(file, line)) {
        if (line.empty()) continue;
        
        vector<string> tokens = splitCSV(line);
        if (tokens.size() < 5) continue;
        
        string startStation = tokens[tokens.size() - 2];
        string endStation = tokens[tokens.size() - 1];
        
        if (isNumber(startStation) || isNumber(endStation)) continue;
        
        int coordCount = tokens.size() - 3;
        if (coordCount < 4 || coordCount % 2 != 0) continue;
        
        routeCount++;
        
        for (int i = 1; i + 3 <= tokens.size() - 2; i += 2) {
            double lon1 = stod(tokens[i]);
            double lat1 = stod(tokens[i + 1]);
            double lon2 = stod(tokens[i + 2]);
            double lat2 = stod(tokens[i + 3]);
            
            string name1 = (i == 1) ? startStation : "";
            string name2 = (i + 4 > tokens.size() - 2) ? endStation : "";
            
            int from = graph.findOrAddNode(lat1, lon1, name1);
            int to = graph.findOrAddNode(lat2, lon2, name2);
            
            double segmentDist = haversineDistance(lat1, lon1, lat2, lon2);
            
            // Add bidirectional bus edges with specific bus mode
            graph.addEdge(from, to, busMode, segmentDist);
            graph.addEdge(to, from, busMode, segmentDist);
        }
    }
    
    file.close();
    cout << "Parsed " << routeCount << " " << getModeName(busMode) 
         << " routes from " << filename << endl;
}

// ============================================================================
// KML EXPORT
// ============================================================================

void exportPathToKML(Graph &graph, vector<int> &path, string filename) {
    // Export path to KML file for visualization in Google MyMaps
    
    ofstream file(filename);
    if (!file.is_open()) {
        cout << "Error: Could not create " << filename << endl;
        return;
    }
    
    // Write KML header
    file << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n";
    file << "<kml xmlns=\"http://earth.google.com/kml/2.1\">\n";
    file << "<Document>\n";
    file << "<Placemark>\n";
    file << "<name>Route</name>\n";
    file << "<LineString>\n";
    file << "<tessellate>1</tessellate>\n";
    file << "<coordinates>\n";
    
    // Write coordinates (path is stored in reverse order from Dijkstra)
    for (int i = path.size() - 1; i >= 0; i--) {
        int nodeId = path[i];
        // KML format: longitude,latitude,altitude
        file << graph.nodes[nodeId].lon << "," 
             << graph.nodes[nodeId].lat << ",0\n";
    }
    
    // Write KML footer
    file << "</coordinates>\n";
    file << "</LineString>\n";
    file << "</Placemark>\n";
    file << "</Document>\n";
    file << "</kml>\n";
    
    file.close();
    cout << "Exported path to " << filename << endl;
}




// PROBLEM 1: SHORTEST CAR ROUTE
void solveProblem1(Graph &graph, double srcLat, double srcLon, 
                   double destLat, double destLon) {
    // Find shortest distance route using only car (basic Dijkstra)
    
    cout << "\n=== Problem 1: Shortest Car Route ===\n";
    
    // Find id of nearest nodes to source and destination
    int source = graph.findNearestNode(srcLat, srcLon);
    int target = graph.findNearestNode(destLat, destLon);
    
    if (source == -1 || target == -1) {
        cout << "Error: Could not find nodes\n";
        return;
    }
    
    cout << "Source Node: " << graph.nodes[source].name 
         << " (" << graph.nodes[source].lat << ", " << graph.nodes[source].lon << ")\n";
    cout << "Target Node: " << graph.nodes[target].name 
         << " (" << graph.nodes[target].lat << ", " << graph.nodes[target].lon << ")\n";
    
    int n = graph.nodes.size();
    
    // Dijkstra's algorithm setup
    vector<double> dist(n, INF);           // Distance from source to each node
    vector<int> prev(n, -1);               // Previous node in shortest path
    vector<bool> visited(n, false);        // Whether node has been processed
    
    dist[source] = 0;
 
    priority_queue<pair<double, int>, vector<pair<double, int>>, greater<pair<double, int>>> pq;
    pq.push({0, source});

    //Dijkstra 
    while (!pq.empty()) {
        auto [currentDist, u] = pq.top();
        pq.pop();
        
        // Skip if already visited (can happen with duplicate entries in PQ)
        if (visited[u]) continue;
        visited[u] = true;
        
        // Early termination: reached target
        if (u == target) break;
        
        // Relax all edges from node u
        for (auto &edge : graph.adjList[u]) {
            // Only consider CAR edges for Problem 1
            if (edge.mode != MODE_CAR) continue;
            
            int v = edge.to;
            double newDist = dist[u] + edge.distance;
            
            // Update if found shorter path
            if (newDist < dist[v]) {
                dist[v] = newDist;
                prev[v] = u;
                pq.push({newDist, v});
            }
        }
    }
    

    // Check if path was found
    if (dist[target] >= INF) {
        cout << "No path found!\n";
        return;
    }
    
    // Reconstruct path from target to source
    vector<int> path;
    for (int at = target; at != -1; at = prev[at]) {    //previous of source is -1, 
        path.push_back(at);
    }
    
    cout << "\n--- Route Details ---\n";
    cout << "Total Distance: " << dist[target] << " km\n";
    
    // Print detailed route
    double carRate = 20.0;  // ₹20 per km
    double totalCost = 0.0;
    double totalDistance = 0.0;
    
    cout << "\nProblem No: 1\n";
    cout << "Source: (" << srcLon << ", " << srcLat << ")\n";
    cout << "Destination: (" << destLon << ", " << destLat << ")\n\n";
    
    // Walking segment from actual source to nearest node (if needed)
    if (fabs(graph.nodes[source].lat - srcLat) > 1e-6 ||    //fabs means absolute value of a floating point number
        fabs(graph.nodes[source].lon - srcLon) > 1e-6) {
        double walkDist = haversineDistance(srcLat, srcLon, 
                                           graph.nodes[source].lat, 
                                           graph.nodes[source].lon);
        cout << "Walk from Source (" << srcLon << ", " << srcLat << ") to "
             << graph.nodes[source].name << " (" << graph.nodes[source].lon 
             << ", " << graph.nodes[source].lat << "), Distance: " << walkDist 
             << " km, Cost: ৳0.00\n";
        totalDistance += walkDist;
    }
    
    // Print each car segment distance and cost
    for (int i = path.size() - 1; i > 0; i--) {
        int from = path[i];
        int to = path[i - 1];
        
        // Find the edge distance
        double segDist = 0;
        for (auto &edge : graph.adjList[from]) {
            if (edge.to == to && edge.mode == MODE_CAR) {
                segDist = edge.distance;
                break;
            }
        }
        
        double segCost = segDist * carRate;
        totalDistance += segDist;
        totalCost += segCost;
        
        cout << "Ride Car from " << graph.nodes[from].name 
             << " (" << graph.nodes[from].lon << ", " << graph.nodes[from].lat << ") to "
             << graph.nodes[to].name << " (" << graph.nodes[to].lon << ", " 
             << graph.nodes[to].lat << "), Distance: " << segDist 
             << " km, Cost: ৳" << segCost << "\n";
    }
    
 
    
    cout << "\nTotal Distance: " << totalDistance << " km\n";
    cout << "Total Cost: ৳" << totalCost << "\n";
    
    // Export to KML
    exportPathToKML(graph, path, "route_problem1.kml");

}



// PROBLEM 2: Find cheapest route using car and metro
void solveProblem2(Graph &graph, double srcLat, double srcLon, 
                   double destLat, double destLon) {
    
    cout << "\n=== Problem 2: Cheapest Route (Car + Metro) ===\n";
    
    int source = graph.findNearestNode(srcLat, srcLon);
    int target = graph.findNearestNode(destLat, destLon);
    
    
    cout << "Source Node: " << graph.nodes[source].name << "\n";
    cout << "Target Node: " << graph.nodes[target].name << "\n";
    
    int n = graph.nodes.size();
    
    // Cost rates
    double carRate = 20.0;   // 20 per km
    double metroRate = 5.0;  // 5 per km
    
    // Dijkstra setup - optimizing for COST instead of distance
    vector<double> cost(n, INF);        // Minimum cost to reach each node
    vector<int> prev(n, -1);            // Previous node in path
    vector<int> prevEdgeIdx(n, -1);     // Which edge was used (to track mode)
    vector<bool> visited(n, false);
    
    cost[source] = 0;
    
    // Priority queue: (cost, node_id)
    priority_queue<pair<double, int>, vector<pair<double, int>>, greater<pair<double, int>>> pq;
    pq.push({0, source});
    
    // Dijkstra (weight is cost )
    while (!pq.empty()) {
        auto [currentCost, u] = pq.top();
        pq.pop();
        
        if (visited[u]) continue;
        visited[u] = true;
        
        if (u == target) break;
        
        for (int edgeIdx = 0; edgeIdx < graph.adjList[u].size(); edgeIdx++) {
            auto &edge = graph.adjList[u][edgeIdx];
            
            // skip if mode is not car or metro
            if (edge.mode != MODE_CAR && edge.mode != MODE_METRO) continue;
            
            int v = edge.to;
            
            // Calculate cost based on mode
            double rate = (edge.mode == MODE_METRO) ? 5.0 : 20.0;
            double edgeCost = edge.distance * rate;
            double newCost = cost[u] + edgeCost;
            
            // Update if cheaper path found
            if (newCost < cost[v]) {
                cost[v] = newCost;
                prev[v] = u;
                prevEdgeIdx[v] = edgeIdx;  // Remember which edge we used
                pq.push({newCost, v});
            }
        }
    }
    
    if (cost[target] >= INF) {
        cout << "No path found!\n";
        return;
    }
    
    // Reconstruct path
    vector<int> path;
    vector<int> edgePath;  // Store which edges were used
    for (int at = target; at != -1; at = prev[at]) {
        path.push_back(at); //storing id of node in path
        edgePath.push_back(prevEdgeIdx[at]);    // index of edge used to reach this node
    }
    
    cout << "\n--- Route Details ---\n";
    cout << "Total Cost: ৳" << cost[target] << "\n";
    
    // Print detailed route
    double totalDistance = 0.0;
    double totalCost = 0.0;
    
    cout << "\nProblem No: 2\n";
    cout << "Source: (" << srcLon << ", " << srcLat << ")\n";
    cout << "Destination: (" << destLon << ", " << destLat << ")\n\n";
    
    // Walking from source if needed
    if (fabs(graph.nodes[source].lat - srcLat) > 1e-6 || 
        fabs(graph.nodes[source].lon - srcLon) > 1e-6) {
        double walkDist = haversineDistance(srcLat, srcLon, 
                                           graph.nodes[source].lat, 
                                           graph.nodes[source].lon);
        cout << "Walk from Source to " << graph.nodes[source].name 
             << ", Distance: " << walkDist << " km, Cost: ৳0.00\n";
        totalDistance += walkDist;
    }
    
    // Print merged segments (only when mode changes)
    Mode currentMode = MODE_WALK;  // Initialize to something that won't match
    int segmentStart = path.size() - 1;
    double segmentDist = 0.0;
    double segmentCost = 0.0;
    
    for (int i = path.size() - 1; i > 0; i--) {
        int from = path[i];
        int to = path[i - 1];
        int edgeIdx = edgePath[i - 1];
        
        auto &edge = graph.adjList[from][edgeIdx];
        Mode mode = edge.mode;
        
        // Check if mode changed
        if (mode != currentMode) {
            // Print accumulated segment if exists
            if (currentMode != MODE_WALK && segmentDist > 0) {
                cout << "Ride " << getModeName(currentMode) << " from " 
                     << graph.nodes[segmentStart].name << " to " << graph.nodes[from].name 
                     << ", Distance: " << segmentDist << " km, Cost: ৳" << segmentCost << "\n";
            }
            
            // Start new segment
            currentMode = mode;
            segmentStart = from;
            segmentDist = 0.0;
            segmentCost = 0.0;
        }
        
        // Accumulate current edge
        double rate = (mode == MODE_METRO) ? metroRate : carRate;
        segmentDist += edge.distance;
        segmentCost += edge.distance * rate;
        totalDistance += edge.distance;
        totalCost += edge.distance * rate;
    }
    
    // Print last accumulated segment
    if (segmentDist > 0) {
        cout << "Ride " << getModeName(currentMode) << " from " 
             << graph.nodes[segmentStart].name << " to " << graph.nodes[target].name 
             << ", Distance: " << segmentDist << " km, Cost: ৳" << segmentCost << "\n";
    }
    
    // Walking to destination if needed
    if (fabs(graph.nodes[target].lat - destLat) > 1e-6 || 
        fabs(graph.nodes[target].lon - destLon) > 1e-6) {
        double walkDist = haversineDistance(graph.nodes[target].lat, 
                                           graph.nodes[target].lon,
                                           destLat, destLon);
        cout << "Walk from " << graph.nodes[target].name 
             << " to Destination, Distance: " << walkDist << " km, Cost: ৳0.00\n";
        totalDistance += walkDist;
    }
    
    cout << "\nTotal Distance: " << totalDistance << " km\n";
    cout << "Total Cost: ৳" << totalCost << "\n";
    
    exportPathToKML(graph, path, "route_problem2.kml");
}


// PROBLEM 3: Find cheapest route using car, metro, and both bus types
void solveProblem3(Graph &graph, double srcLat, double srcLon, 
                   double destLat, double destLon) {
    
    cout << "\n=== Problem 3: Cheapest Route (All Modes) ===\n";
    
    int source = graph.findNearestNode(srcLat, srcLon);
    int target = graph.findNearestNode(destLat, destLon);
    
    
    cout << "Source Node: " << graph.nodes[source].name << "\n";
    cout << "Target Node: " << graph.nodes[target].name << "\n";
    
    int n = graph.nodes.size();
    
    // Cost rates for all modes
    double carRate = 20.0;
    double metroRate = 5.0;
    double bikolpoRate = 7.0;
    double uttaraRate = 10.0;
    
    // Dijkstra setup
    vector<double> cost(n, INF);
    vector<int> prev(n, -1);
    vector<int> prevEdgeIdx(n, -1);
    vector<bool> visited(n, false);
    
    cost[source] = 0;
    priority_queue<pair<double, int>, vector<pair<double, int>>, greater<pair<double, int>>> pq;
    pq.push({0, source});
    
    // Dijkstra
    while (!pq.empty()) {
        auto [currentCost, u] = pq.top();
        pq.pop();
        
        if (visited[u]) continue;
        visited[u] = true;
        
        if (u == target) break;
        
        // Relax edges - consider all 4 transport modes
        for (int edgeIdx = 0; edgeIdx < graph.adjList[u].size(); edgeIdx++) {
            auto &edge = graph.adjList[u][edgeIdx];
            
            int v = edge.to;
            
            // Get cost rate based on mode
            double rate = carRate;
            if (edge.mode == MODE_METRO) rate = metroRate;
            else if (edge.mode == MODE_BIKOLPO) rate = bikolpoRate;
            else if (edge.mode == MODE_UTTARA) rate = uttaraRate;
            
            double edgeCost = edge.distance * rate;
            double newCost = cost[u] + edgeCost;
            
            if (newCost < cost[v]) {
                cost[v] = newCost;
                prev[v] = u;
                prevEdgeIdx[v] = edgeIdx;
                pq.push({newCost, v});
            }
        }
    }
    
    if (cost[target] >= INF) {
        cout << "No path found!\n";
        return;
    }
    
    // Reconstruct path
    vector<int> path;
    vector<int> edgePath;
    for (int at = target; at != -1; at = prev[at]) {
        path.push_back(at);
        edgePath.push_back(prevEdgeIdx[at]);
    }
    
    cout << "\n--- Route Details ---\n";
    cout << "Total Cost: ৳" << cost[target] << "\n";
    
    // Print detailed route
    double totalDistance = 0.0;
    double totalCost = 0.0;
    
    cout << "\nProblem No: 3\n";
    cout << "Source: (" << srcLon << ", " << srcLat << ")\n";
    cout << "Destination: (" << destLon << ", " << destLat << ")\n\n";
    
    // Walking from source if needed
    if (fabs(graph.nodes[source].lat - srcLat) > 1e-6 || 
        fabs(graph.nodes[source].lon - srcLon) > 1e-6) {
        double walkDist = haversineDistance(srcLat, srcLon, 
                                           graph.nodes[source].lat, 
                                           graph.nodes[source].lon);
        cout << "Walk from Source to " << graph.nodes[source].name 
             << ", Distance: " << walkDist << " km, Cost: ৳0.00\n";
        totalDistance += walkDist;
    }
    
    // Print merged segments (only when mode changes)
    Mode currentMode = MODE_WALK;
    int segmentStart = path.size() - 1;
    double segmentDist = 0.0;
    double segmentCost = 0.0;
    
    for (int i = path.size() - 1; i > 0; i--) {
        int from = path[i];
        int to = path[i - 1];
        int edgeIdx = edgePath[i - 1];
        
        auto &edge = graph.adjList[from][edgeIdx];
        Mode mode = edge.mode;
        
        // Check if mode changed
        if (mode != currentMode) {
            // Print accumulated segment if exists
            if (currentMode != MODE_WALK && segmentDist > 0) {
                cout << "Ride " << getModeName(currentMode) << " from " 
                     << graph.nodes[segmentStart].name << " to " << graph.nodes[from].name 
                     << ", Distance: " << segmentDist << " km, Cost: ৳" << segmentCost << "\n";
            }
            
            // Start new segment
            currentMode = mode;
            segmentStart = from;
            segmentDist = 0.0;
            segmentCost = 0.0;
        }
        
        // Accumulate current edge
        double rate = carRate;
        if (mode == MODE_METRO) rate = metroRate;
        else if (mode == MODE_BIKOLPO) rate = bikolpoRate;
        else if (mode == MODE_UTTARA) rate = uttaraRate;
        
        segmentDist += edge.distance;
        segmentCost += edge.distance * rate;
        totalDistance += edge.distance;
        totalCost += edge.distance * rate;
    }
    
    // Print last accumulated segment
    if (segmentDist > 0) {
        cout << "Ride " << getModeName(currentMode) << " from " 
             << graph.nodes[segmentStart].name << " to " << graph.nodes[target].name 
             << ", Distance: " << segmentDist << " km, Cost: ৳" << segmentCost << "\n";
    }
    
    // Walking to destination if needed
    if (fabs(graph.nodes[target].lat - destLat) > 1e-6 || 
        fabs(graph.nodes[target].lon - destLon) > 1e-6) {
        double walkDist = haversineDistance(graph.nodes[target].lat, 
                                           graph.nodes[target].lon,
                                           destLat, destLon);
        cout << "Walk from " << graph.nodes[target].name 
             << " to Destination, Distance: " << walkDist << " km, Cost: ৳0.00\n";
        totalDistance += walkDist;
    }
    
    cout << "\nTotal Distance: " << totalDistance << " km\n";
    cout << "Total Cost: ৳" << totalCost << "\n";
    
    exportPathToKML(graph, path, "route_problem3.kml");
}


// PROBLEM 4: cheapest with time schedules
void solveProblem4(Graph &graph, double srcLat, double srcLon, 
                   double destLat, double destLon, int startTimeMin) {
    // Find cheapest route considering bus/metro schedules
    // All vehicles move at 30 km/h, buses/metro every 15 min (6 AM - 11 PM)
    
    cout << "\n=== Problem 4: Cheapest with Time Schedules ===\n";
    
    int source = graph.findNearestNode(srcLat, srcLon);
    int target = graph.findNearestNode(destLat, destLon);
    
    cout << "Source Node: " << graph.nodes[source].name << "\n";
    cout << "Target Node: " << graph.nodes[target].name << "\n";
    cout << "Start Time: " << formatTime(startTimeMin) << "\n";
    
    int n = graph.nodes.size();
    
    double carRate = 20.0;
    double metroRate = 5.0;
    double bikolpoRate = 7.0;
    double uttaraRate = 10.0;
    
    // Track both cost and arrival time at each node
    vector<double> cost(n, INF);
    vector<double> arrivalTime(n, INF);
    vector<int> prev(n, -1);
    vector<int> prevEdgeIdx(n, -1);
    vector<bool> visited(n, false);
    
    cost[source] = 0;
    arrivalTime[source] = startTimeMin;
    
    // Priority queue: (cost, node_id) - still optimizing for cost
    priority_queue<pair<double, int>, vector<pair<double, int>>, greater<pair<double, int>>> pq;
    pq.push({0, source});
    
    //Dijkstra 
    while (!pq.empty()) {
        auto [currentCost, u] = pq.top();
        pq.pop();
        
        if (visited[u]) continue;
        visited[u] = true;
        
        if (u == target) break;
        
        // Get the mode used to ARRIVE at node u (for waiting time logic)
        Mode arrivalMode = MODE_CAR;  // Default for source
        if (prevEdgeIdx[u] >= 0) {
            arrivalMode = graph.adjList[prev[u]][prevEdgeIdx[u]].mode;
        }
        
        // Relax edges
        for (int edgeIdx = 0; edgeIdx < graph.adjList[u].size(); edgeIdx++) {
            auto &edge = graph.adjList[u][edgeIdx];
            
            if (edge.mode != MODE_CAR && edge.mode != MODE_METRO && 
                edge.mode != MODE_BIKOLPO && edge.mode != MODE_UTTARA) {
                continue;
            }
            
            int v = edge.to;
            
            // Calculate waiting time (only when BOARDING a scheduled service)
            double waitTime = 0.0;
            if (edge.mode != MODE_CAR && edge.mode != MODE_WALK) {
                // Only wait if switching modes or first boarding
                if (edge.mode != arrivalMode || u == source) {
                    waitTime = getWaitingTime((int)arrivalTime[u], edge.mode);
                    if (waitTime >= INF) continue;  // Service not available
                }
            }
            
            // Calculate travel time at 30 km/h
            double travelTime = (edge.distance / VEHICLE_SPEED_KMH) * 60.0;  // in minutes
            double newArrivalTime = arrivalTime[u] + waitTime + travelTime;
            
            // Calculate cost
            double rate = carRate;
            if (edge.mode == MODE_METRO) rate = metroRate;
            else if (edge.mode == MODE_BIKOLPO) rate = bikolpoRate;
            else if (edge.mode == MODE_UTTARA) rate = uttaraRate;
            
            double edgeCost = edge.distance * rate;
            double newCost = cost[u] + edgeCost;
            
            // Update if cheaper (still optimizing cost, not time)
            if (newCost < cost[v]) {
                cost[v] = newCost;
                arrivalTime[v] = newArrivalTime;
                prev[v] = u;
                prevEdgeIdx[v] = edgeIdx;
                pq.push({newCost, v});
            }
        }
    }
    
    if (cost[target] >= INF) {
        cout << "No path found!\n";
        return;
    }
    
    // Reconstruct path
    vector<int> path;
    vector<int> edgePath;
    for (int at = target; at != -1; at = prev[at]) {
        path.push_back(at);
        edgePath.push_back(prevEdgeIdx[at]);
    }
    
    cout << "\n--- Route Details ---\n";
    cout << "Total Cost: ৳" << cost[target] << "\n";
    cout << "Arrival Time: " << formatTime((int)arrivalTime[target]) << "\n";
    
    // Print detailed timed route
    double currentTime = startTimeMin;
    double totalDistance = 0.0;
    double totalCost = 0.0;
    double totalTravelTime = 0.0;
    
    cout << "\nProblem No: 4\n";
    cout << "Source: (" << srcLon << ", " << srcLat << ")\n";
    cout << "Destination: (" << destLon << ", " << destLat << ")\n";
    cout << "Start Time: " << formatTime(startTimeMin) << "\n\n";
    
    // Walking from source if needed
    if (fabs(graph.nodes[source].lat - srcLat) > 1e-6 || 
        fabs(graph.nodes[source].lon - srcLon) > 1e-6) {
        double walkDist = haversineDistance(srcLat, srcLon, 
                                           graph.nodes[source].lat, 
                                           graph.nodes[source].lon);
        double walkTime = (walkDist / WALK_SPEED_KMH) * 60.0;
        
        cout << "[" << formatTime((int)currentTime) << "] Walk from Source to "
             << graph.nodes[source].name << ", Distance: " << walkDist 
             << " km, Time: " << walkTime << " min, Cost: ৳0.00\n";
        
        totalDistance += walkDist;
        totalTravelTime += walkTime;
        currentTime += walkTime;
    }
    
    // Print merged segments with timing (only when mode changes)
    Mode currentMode = MODE_WALK;
    int segmentStart = path.size() - 1;
    double segmentDist = 0.0;
    double segmentCost = 0.0;
    double segmentTime = 0.0;
    double segmentStartTime = currentTime;
    
    for (int i = path.size() - 1; i > 0; i--) {
        int from = path[i];
        int to = path[i - 1];
        int edgeIdx = edgePath[i - 1];
        
        auto &edge = graph.adjList[from][edgeIdx];
        Mode mode = edge.mode;
        
        // Check if mode changed
        if (mode != currentMode) {
            // Print accumulated segment if exists
            if (currentMode != MODE_WALK && segmentDist > 0) {
                cout << "[" << formatTime((int)segmentStartTime) << "] Ride " 
                     << getModeName(currentMode) << " from " << graph.nodes[segmentStart].name 
                     << " to " << graph.nodes[from].name << ", Distance: " << segmentDist 
                     << " km, Time: " << segmentTime << " min, Cost: ৳" << segmentCost << "\n";
                currentTime = segmentStartTime + segmentTime;
            }
            
            // Check if need to wait for new vehicle
            if (mode != MODE_CAR && mode != MODE_WALK) {
                double waitTime = getWaitingTime((int)currentTime, mode);
                if (waitTime > 0 && waitTime < INF) {
                    cout << "[" << formatTime((int)currentTime) << "] Wait for "
                         << getModeName(mode) << ": " << waitTime << " minutes\n";
                    currentTime += waitTime;
                    totalTravelTime += waitTime;
                }
            }
            
            // Start new segment
            currentMode = mode;
            segmentStart = from;
            segmentDist = 0.0;
            segmentCost = 0.0;
            segmentTime = 0.0;
            segmentStartTime = currentTime;
        }
        
        // Accumulate current edge
        double rate = carRate;
        if (mode == MODE_METRO) rate = metroRate;
        else if (mode == MODE_BIKOLPO) rate = bikolpoRate;
        else if (mode == MODE_UTTARA) rate = uttaraRate;
        
        double travelTime = (edge.distance / VEHICLE_SPEED_KMH) * 60.0;
        
        segmentDist += edge.distance;
        segmentCost += edge.distance * rate;
        segmentTime += travelTime;
        totalDistance += edge.distance;
        totalCost += edge.distance * rate;
        totalTravelTime += travelTime;
    }
    
    // Print last accumulated segment
    if (segmentDist > 0) {
        cout << "[" << formatTime((int)segmentStartTime) << "] Ride " 
             << getModeName(currentMode) << " from " << graph.nodes[segmentStart].name 
             << " to " << graph.nodes[target].name << ", Distance: " << segmentDist 
             << " km, Time: " << segmentTime << " min, Cost: ৳" << segmentCost << "\n";
        currentTime = segmentStartTime + segmentTime;
    }
    
    // Walking to destination if needed
    if (fabs(graph.nodes[target].lat - destLat) > 1e-6 || 
        fabs(graph.nodes[target].lon - destLon) > 1e-6) {
        double walkDist = haversineDistance(graph.nodes[target].lat, 
                                           graph.nodes[target].lon,
                                           destLat, destLon);
        double walkTime = (walkDist / WALK_SPEED_KMH) * 60.0;
        
        cout << "[" << formatTime((int)currentTime) << "] Walk from "
             << graph.nodes[target].name << " to Destination, Distance: " 
             << walkDist << " km, Time: " << walkTime << " min, Cost: ৳0.00\n";
        
        totalDistance += walkDist;
        totalTravelTime += walkTime;
        currentTime += walkTime;
    }
    
    cout << "\nArrival Time: " << formatTime((int)currentTime) << "\n";
    cout << "Total Distance: " << totalDistance << " km\n";
    cout << "Total Travel Time: " << totalTravelTime << " minutes (" 
         << (totalTravelTime / 60.0) << " hours)\n";
    cout << "Total Cost: ৳" << totalCost << "\n";
    
    exportPathToKML(graph, path, "route_problem4.kml");
}

// PROBLEM 5: fastest way (schedules are still considered)
void solveProblem5(Graph &graph, double srcLat, double srcLon, 
                   double destLat, double destLon, int startTimeMin) {
    
    cout << "\n=== Problem 5: Fastest Route ===\n";
    
    int source = graph.findNearestNode(srcLat, srcLon);
    int target = graph.findNearestNode(destLat, destLon);
    
    cout << "Source Node: " << graph.nodes[source].name << "\n";
    cout << "Target Node: " << graph.nodes[target].name << "\n";
    cout << "Start Time: " << formatTime(startTimeMin) << "\n";
    
    int n = graph.nodes.size();
    
    double carRate = 20.0;
    double metroRate = 5.0;
    double bikolpoRate = 7.0;
    double uttaraRate = 10.0;
    
    // Now optimizing for TIME, not cost
    vector<double> arrivalTime(n, INF);
    vector<int> prev(n, -1);
    vector<int> prevEdgeIdx(n, -1);
    vector<bool> visited(n, false);
    
    arrivalTime[source] = startTimeMin;
    
    priority_queue<pair<double, int>, vector<pair<double, int>>, greater<pair<double, int>>> pq;
    pq.push({startTimeMin, source});
    
    //dijkstra 
    while (!pq.empty()) {
        auto [currentTime, u] = pq.top();
        pq.pop();
        
        if (visited[u]) continue;
        visited[u] = true;
        
        if (u == target) break;
        
        // Get arrival mode at node u
        Mode arrivalMode = MODE_CAR;
        if (prevEdgeIdx[u] >= 0) {
            arrivalMode = graph.adjList[prev[u]][prevEdgeIdx[u]].mode;
        }
        
        // Relax edges
        for (int edgeIdx = 0; edgeIdx < graph.adjList[u].size(); edgeIdx++) {
            auto &edge = graph.adjList[u][edgeIdx];
            
            int v = edge.to;
            
            // Calculate waiting time (when changing vehicle)
            double waitTime = 0.0;
            if (edge.mode != MODE_CAR && edge.mode != MODE_WALK) {
                if (edge.mode != arrivalMode || u == source) {
                    waitTime = getWaitingTime((int)arrivalTime[u], edge.mode);
                    if (waitTime >= INF) continue;
                }
            }
            
            double travelTime = (edge.distance / VEHICLE_SPEED_PROBLEM5_KMH) * 60.0;
            double newArrivalTime = arrivalTime[u] + waitTime + travelTime;
            
            if (newArrivalTime < arrivalTime[v]) {
                arrivalTime[v] = newArrivalTime;
                prev[v] = u;
                prevEdgeIdx[v] = edgeIdx;
                pq.push({newArrivalTime, v});
            }
        }
    }
    
    if (arrivalTime[target] >= INF) {
        cout << "No path found!\n";
        return;
    }
    
    // Reconstruct path
    vector<int> path;
    vector<int> edgePath;
    for (int at = target; at != -1; at = prev[at]) {
        path.push_back(at);
        edgePath.push_back(prevEdgeIdx[at]);
    }
    
    double totalTime = arrivalTime[target] - startTimeMin;
    cout << "\n--- Route Details ---\n";
    cout << "Total Travel Time: " << totalTime << " minutes (" 
         << (totalTime / 60.0) << " hours)\n";
    cout << "Arrival Time: " << formatTime((int)arrivalTime[target]) << "\n";
    
    // Print detailed timed route
    double currentTime = startTimeMin;
    double totalDistance = 0.0;
    double totalCost = 0.0;
    double totalTravelTime = 0.0;
    
    cout << "\nProblem No: 5\n";
    cout << "Source: (" << srcLon << ", " << srcLat << ")\n";
    cout << "Destination: (" << destLon << ", " << destLat << ")\n";
    cout << "Start Time: " << formatTime(startTimeMin) << "\n\n";
    
    // Walking from source if needed
    if (fabs(graph.nodes[source].lat - srcLat) > 1e-6 || 
        fabs(graph.nodes[source].lon - srcLon) > 1e-6) {
        double walkDist = haversineDistance(srcLat, srcLon, 
                                           graph.nodes[source].lat, 
                                           graph.nodes[source].lon);
        double walkTime = (walkDist / WALK_SPEED_KMH) * 60.0;
        
        cout << "[" << formatTime((int)currentTime) << "] Walk from Source to "
             << graph.nodes[source].name << ", Distance: " << walkDist 
             << " km, Time: " << walkTime << " min, Cost: ৳0.00\n";
        
        totalDistance += walkDist;
        totalTravelTime += walkTime;
        currentTime += walkTime;
    }
    
    // Print merged segments (only when mode changes)
    Mode currentMode = MODE_WALK;
    int segmentStart = path.size() - 1;
    double segmentDist = 0.0;
    double segmentCost = 0.0;
    double segmentTime = 0.0;
    double segmentStartTime = currentTime;
    
    for (int i = path.size() - 1; i > 0; i--) {
        int from = path[i];
        int to = path[i - 1];
        int edgeIdx = edgePath[i - 1];
        
        auto &edge = graph.adjList[from][edgeIdx];
        Mode mode = edge.mode;
        
        // Check if mode changed
        if (mode != currentMode) {
            // Print accumulated segment if exists
            if (currentMode != MODE_WALK && segmentDist > 0) {
                cout << "[" << formatTime((int)segmentStartTime) << "] Ride " 
                     << getModeName(currentMode) << " from " << graph.nodes[segmentStart].name 
                     << " to " << graph.nodes[from].name << ", Distance: " << segmentDist 
                     << " km, Time: " << segmentTime << " min, Cost: ৳" << segmentCost << "\n";
                currentTime = segmentStartTime + segmentTime;
            }
            
            // Wait for new vehicle
            if (mode != MODE_CAR && mode != MODE_WALK) {
                double waitTime = getWaitingTime((int)currentTime, mode);
                if (waitTime > 0 && waitTime < INF) {
                    cout << "[" << formatTime((int)currentTime) << "] Wait for "
                         << getModeName(mode) << ": " << waitTime << " minutes\n";
                    currentTime += waitTime;
                    totalTravelTime += waitTime;
                }
            }
            
            // Start new segment
            currentMode = mode;
            segmentStart = from;
            segmentDist = 0.0;
            segmentCost = 0.0;
            segmentTime = 0.0;
            segmentStartTime = currentTime;
        }
        
        // Accumulate current edge
        double rate = carRate;
        if (mode == MODE_METRO) rate = metroRate;
        else if (mode == MODE_BIKOLPO) rate = bikolpoRate;
        else if (mode == MODE_UTTARA) rate = uttaraRate;
        
        double travelTime = (edge.distance / VEHICLE_SPEED_PROBLEM5_KMH) * 60.0;  // 10 km/h
        
        segmentDist += edge.distance;
        segmentCost += edge.distance * rate;
        segmentTime += travelTime;
        totalDistance += edge.distance;
        totalCost += edge.distance * rate;
        totalTravelTime += travelTime;
    }
    
    // Print last accumulated segment
    if (segmentDist > 0) {
        cout << "[" << formatTime((int)segmentStartTime) << "] Ride " 
             << getModeName(currentMode) << " from " << graph.nodes[segmentStart].name 
             << " to " << graph.nodes[target].name << ", Distance: " << segmentDist 
             << " km, Time: " << segmentTime << " min, Cost: ৳" << segmentCost << "\n";
        currentTime = segmentStartTime + segmentTime;
    }
    
    // Walking to destination if needed
    if (fabs(graph.nodes[target].lat - destLat) > 1e-6 || 
        fabs(graph.nodes[target].lon - destLon) > 1e-6) {
        double walkDist = haversineDistance(graph.nodes[target].lat, 
                                           graph.nodes[target].lon,
                                           destLat, destLon);
        double walkTime = (walkDist / WALK_SPEED_KMH) * 60.0;
        
        cout << "[" << formatTime((int)currentTime) << "] Walk from "
             << graph.nodes[target].name << " to Destination, Distance: " 
             << walkDist << " km, Time: " << walkTime << " min, Cost: ৳0.00\n";
        
        totalDistance += walkDist;
        totalTravelTime += walkTime;
        currentTime += walkTime;
    }
    
    cout << "\nArrival Time: " << formatTime((int)currentTime) << "\n";
    cout << "Total Distance: " << totalDistance << " km\n";
    cout << "Total Travel Time: " << totalTravelTime << " minutes (" 
         << (totalTravelTime / 60.0) << " hours)\n";
    cout << "Total Cost: ৳" << totalCost << "\n";
    
    exportPathToKML(graph, path, "route_problem5.kml");
}


// PROBLEM 6: Find cheapest route that arrives before deadline
void solveProblem6(Graph &graph, double srcLat, double srcLon, 
                   double destLat, double destLon, int startTimeMin, int deadlineMin) {
    
    cout << "\n=== Problem 6: Cheapest with Deadline ===\n";
    
    int source = graph.findNearestNode(srcLat, srcLon);
    int target = graph.findNearestNode(destLat, destLon);
    
    cout << "Source Node: " << graph.nodes[source].name << "\n";
    cout << "Target Node: " << graph.nodes[target].name << "\n";
    cout << "Start Time: " << formatTime(startTimeMin) << "\n";
    cout << "Deadline: " << formatTime(deadlineMin) << "\n";
    
    if (deadlineMin <= startTimeMin) {
        cout << "Error: Deadline must be after start time\n";
        return;
    }
    
    int n = graph.nodes.size();
    
    double carRate = 20.0;
    double metroRate = 5.0;
    double bikolpoRate = 7.0;
    double uttaraRate = 10.0;
    
    // Track both cost and arrival time
    vector<double> cost(n, INF);
    vector<double> arrivalTime(n, INF);
    vector<int> prev(n, -1);
    vector<int> prevEdgeIdx(n, -1);
    vector<bool> visited(n, false);
    
    cost[source] = 0;
    arrivalTime[source] = startTimeMin;
    
    // Optimize for cost, but with deadline constraint
    priority_queue<pair<double, int>, vector<pair<double, int>>, greater<pair<double, int>>> pq;
    pq.push({0, source});
    
    while (!pq.empty()) {
        auto [currentCost, u] = pq.top();
        pq.pop();
        
        if (visited[u]) continue;
        visited[u] = true;
        
        if (u == target) break;
        
        Mode arrivalMode = MODE_CAR;
        if (prevEdgeIdx[u] >= 0) {
            arrivalMode = graph.adjList[prev[u]][prevEdgeIdx[u]].mode;
        }
        
        for (int edgeIdx = 0; edgeIdx < graph.adjList[u].size(); edgeIdx++) {
            auto &edge = graph.adjList[u][edgeIdx];
            
            if (edge.mode != MODE_CAR && edge.mode != MODE_METRO && 
                edge.mode != MODE_BIKOLPO && edge.mode != MODE_UTTARA) {
                continue;
            }
            
            int v = edge.to;
            
            double waitTime = 0.0;
            if (edge.mode != MODE_CAR && edge.mode != MODE_WALK) {
                if (edge.mode != arrivalMode || u == source) {
                    waitTime = getWaitingTimeProblem6((int)arrivalTime[u], edge.mode);
                    if (waitTime >= INF) continue;
                }
            }
            
            // Calculate travel time with mode-specific speeds
            double speed = CAR_SPEED_PROBLEM6_KMH;
            if (edge.mode == MODE_METRO) speed = METRO_SPEED_PROBLEM6_KMH;
            else if (edge.mode == MODE_BIKOLPO) speed = BIKOLPO_SPEED_PROBLEM6_KMH;
            else if (edge.mode == MODE_UTTARA) speed = UTTARA_SPEED_PROBLEM6_KMH;
            
            double travelTime = (edge.distance / speed) * 60.0;
            double newArrivalTime = arrivalTime[u] + waitTime + travelTime;
            
            // DEADLINE CONSTRAINT - Skip if would arrive late
            if (newArrivalTime > deadlineMin) {
                continue;  // This path would miss the deadline
            }
            
            // Calculate cost
            double rate = carRate;
            if (edge.mode == MODE_METRO) rate = metroRate;
            else if (edge.mode == MODE_BIKOLPO) rate = bikolpoRate;
            else if (edge.mode == MODE_UTTARA) rate = uttaraRate;
            
            double edgeCost = edge.distance * rate;
            double newCost = cost[u] + edgeCost;
            
            // Update if cheaper AND meets deadline
            if (newCost < cost[v]) {
                cost[v] = newCost;
                arrivalTime[v] = newArrivalTime;
                prev[v] = u;
                prevEdgeIdx[v] = edgeIdx;
                pq.push({newCost, v});
            }
        }
    }
    
    if (cost[target] >= INF) {
        cout << "No path found that meets the deadline!\n";
        return;
    }
    
    // Reconstruct path
    vector<int> path;
    vector<int> edgePath;
    for (int at = target; at != -1; at = prev[at]) {
        path.push_back(at);
        edgePath.push_back(prevEdgeIdx[at]);
    }
    
    cout << "\n--- Route Details ---\n";
    cout << "Total Cost: ৳" << cost[target] << "\n";
    cout << "Arrival Time: " << formatTime((int)arrivalTime[target]) << "\n";
    
    double slack = deadlineMin - arrivalTime[target];
    if (slack >= 0) {
        cout << "Status: ON TIME (" << slack << " minutes early)\n";
    } else {
        cout << "Status: LATE (" << (-slack) << " minutes late)\n";
    }
    
    // Print detailed route
    double currentTime = startTimeMin;
    double totalDistance = 0.0;
    double totalCost = 0.0;
    double totalTravelTime = 0.0;
    
    cout << "\nProblem No: 6\n";
    cout << "Source: (" << srcLon << ", " << srcLat << ")\n";
    cout << "Destination: (" << destLon << ", " << destLat << ")\n";
    cout << "Start Time: " << formatTime(startTimeMin) << "\n";
    cout << "Deadline: " << formatTime(deadlineMin) << "\n\n";
    
    // Walking from source if needed
    if (fabs(graph.nodes[source].lat - srcLat) > 1e-6 || 
        fabs(graph.nodes[source].lon - srcLon) > 1e-6) {
        double walkDist = haversineDistance(srcLat, srcLon, 
                                           graph.nodes[source].lat, 
                                           graph.nodes[source].lon);
        double walkTime = (walkDist / WALK_SPEED_KMH) * 60.0;
        
        cout << "[" << formatTime((int)currentTime) << "] Walk from Source to "
             << graph.nodes[source].name << ", Distance: " << walkDist 
             << " km, Time: " << walkTime << " min, Cost: ৳0.00\n";
        
        totalDistance += walkDist;
        totalTravelTime += walkTime;
        currentTime += walkTime;
    }
    
    // Print merged segments (only when mode changes)
    Mode currentMode = MODE_WALK;
    int segmentStart = path.size() - 1;
    double segmentDist = 0.0;
    double segmentCost = 0.0;
    double segmentTime = 0.0;
    double segmentStartTime = currentTime;
    
    for (int i = path.size() - 1; i > 0; i--) {
        int from = path[i];
        int to = path[i - 1];
        int edgeIdx = edgePath[i - 1];
        
        auto &edge = graph.adjList[from][edgeIdx];
        Mode mode = edge.mode;
        
        // Check if mode changed
        if (mode != currentMode) {
            // Print accumulated segment if exists
            if (currentMode != MODE_WALK && segmentDist > 0) {
                cout << "[" << formatTime((int)segmentStartTime) << "] Ride " 
                     << getModeName(currentMode) << " from " << graph.nodes[segmentStart].name 
                     << " to " << graph.nodes[from].name << ", Distance: " << segmentDist 
                     << " km, Time: " << segmentTime << " min, Cost: ৳" << segmentCost << "\n";
                currentTime = segmentStartTime + segmentTime;
            }
            
            // Wait for new vehicle with Problem 6 schedules
            if (mode != MODE_CAR && mode != MODE_WALK) {
                double waitTime = getWaitingTimeProblem6((int)currentTime, mode);
                if (waitTime > 0 && waitTime < INF) {
                    cout << "[" << formatTime((int)currentTime) << "] Wait for "
                         << getModeName(mode) << ": " << waitTime << " minutes\n";
                    currentTime += waitTime;
                    totalTravelTime += waitTime;
                }
            }
            
            // Start new segment
            currentMode = mode;
            segmentStart = from;
            segmentDist = 0.0;
            segmentCost = 0.0;
            segmentTime = 0.0;
            segmentStartTime = currentTime;
        }
        
        // Accumulate current edge with mode-specific speed
        double speed = CAR_SPEED_PROBLEM6_KMH;
        if (mode == MODE_METRO) speed = METRO_SPEED_PROBLEM6_KMH;
        else if (mode == MODE_BIKOLPO) speed = BIKOLPO_SPEED_PROBLEM6_KMH;
        else if (mode == MODE_UTTARA) speed = UTTARA_SPEED_PROBLEM6_KMH;
        
        double rate = carRate;
        if (mode == MODE_METRO) rate = metroRate;
        else if (mode == MODE_BIKOLPO) rate = bikolpoRate;
        else if (mode == MODE_UTTARA) rate = uttaraRate;
        
        double travelTime = (edge.distance / speed) * 60.0;
        
        segmentDist += edge.distance;
        segmentCost += edge.distance * rate;
        segmentTime += travelTime;
        totalDistance += edge.distance;
        totalCost += edge.distance * rate;
        totalTravelTime += travelTime;
    }
    
    // Print last accumulated segment
    if (segmentDist > 0) {
        cout << "[" << formatTime((int)segmentStartTime) << "] Ride " 
             << getModeName(currentMode) << " from " << graph.nodes[segmentStart].name 
             << " to " << graph.nodes[target].name << ", Distance: " << segmentDist 
             << " km, Time: " << segmentTime << " min, Cost: ৳" << segmentCost << "\n";
        currentTime = segmentStartTime + segmentTime;
    }
    
    // Walking to destination if needed
    if (fabs(graph.nodes[target].lat - destLat) > 1e-6 || 
        fabs(graph.nodes[target].lon - destLon) > 1e-6) {
        double walkDist = haversineDistance(graph.nodes[target].lat, 
                                           graph.nodes[target].lon,
                                           destLat, destLon);
        double walkTime = (walkDist / WALK_SPEED_KMH) * 60.0;
        
        cout << "[" << formatTime((int)currentTime) << "] Walk from "
             << graph.nodes[target].name << " to Destination, Distance: " 
             << walkDist << " km, Time: " << walkTime << " min, Cost: ৳0.00\n";
        
        totalDistance += walkDist;
        totalTravelTime += walkTime;
        currentTime += walkTime;
    }
    
    cout << "\nArrival Time: " << formatTime((int)currentTime) << "\n";
    cout << "Total Distance: " << totalDistance << " km\n";
    cout << "Total Travel Time: " << totalTravelTime << " minutes (" 
         << (totalTravelTime / 60.0) << " hours)\n";
    cout << "Total Cost: ৳" << totalCost << "\n";
    
    exportPathToKML(graph, path, "route_problem6.kml");
}