#ifndef GRAPH_H
#define GRAPH_H

#include "Utils.h"
#include <vector>
#include <string>

using namespace std;

// ============================================================================
// DATA STRUCTURES
// ============================================================================

// Represents a node (intersection/station/stop) in the graph
struct Node {
    int id;              // Unique identifier
    double lat, lon;     // GPS coordinates (latitude, longitude)
    string name;         // Station/stop name (e.g., "Mirpur 12", "Node42")
    
    // Constructor
    Node(int id, double lat, double lon, string name = "") 
        {
        this->id = id;
        this->lat = lat;
        this->lon = lon;
        this->name = name;

        // If no name provided, generate default name
        if (name.empty()) {
            name = "Node" + to_string(id);
        }
    }
};

// Represents an edge (connection) between two nodes
struct Edge {
    int to;              // Destination node ID
    Mode mode;           // Transportation mode (car, metro, bus, etc.)
    double distance;     // Distance in kilometers (Haversine)
    
    // Constructor
    Edge(int to, Mode mode, double distance)
        {
        this->to = to;
        this->mode = mode;
        this->distance = distance;
    }
};

// Main graph structure representing the transportation network
struct Graph {
    vector<Node> nodes;                    // All nodes in the graph
    vector<vector<Edge>> adjList;          // for each node, list of adjacent edges {to, mode, distance}
    
    // Constructor
    Graph() {}
    
    // Add a node or find existing node at given coordinates
    // Returns: node ID (index in nodes vector)
    int findOrAddNode(double lat, double lon, string name = "");
    
    // Find the nearest node of given coordinates
    // Returns: node ID, or -1 if no nodes exist
    int findNearestNode(double lat, double lon);
    
    // Add a directed edge from node 'from' to node 'to'
    void addEdge(int from, int to, Mode mode, double distance);
};

// ============================================================================
// CSV PARSING FUNCTIONS
// ============================================================================

// Parse roadmap CSV file and add car edges to graph
// File format: DhakaStreet, lon1, lat1, lon2, lat2, ..., altitude, length
void parseRoadmapCSV(Graph &graph, string filename);

// Parse metro route CSV file and add metro edges to graph
// File format: DhakaMetroRail, lon1, lat1, lon2, lat2, ..., StartStation, EndStation
void parseMetroCSV(Graph &graph, string filename);

// Parse bus route CSV file and add bus edges to graph
// File format: BusType, lon1, lat1, lon2, lat2, ..., StartStation, EndStation
void parseBusCSV(Graph &graph, string filename, Mode busMode);

// ============================================================================
// KML EXPORT
// ============================================================================

// Export a path (sequence of node IDs) to KML file for visualization
void exportPathToKML(Graph &graph, vector<int> &path, string filename);

// ============================================================================
// PROBLEM SOLVERS (DIJKSTRA VARIATIONS)
// ============================================================================

// Problem 1: Find shortest distance route using only car
// Optimizes: Distance (km)
// Modes: Car only
void solveProblem1(Graph &graph, 
                   double srcLat, double srcLon, 
                   double destLat, double destLon);


                   
// Problem 2: Find cheapest route using car and metro
// Optimizes: Cost (₹)
// Modes: Car (₹20/km), Metro (₹5/km)
void solveProblem2(Graph &graph, 
                   double srcLat, double srcLon, 
                   double destLat, double destLon);

// Problem 3: Find cheapest route using car, metro, and buses
// Optimizes: Cost (₹)
// Modes: Car (₹20/km), Metro (₹5/km), Bikolpo (₹7/km), Uttara (₹10/km)
void solveProblem3(Graph &graph, 
                   double srcLat, double srcLon, 
                   double destLat, double destLon);

// Problem 4: Find cheapest route with time schedules
// Optimizes: Cost (₹)
// Constraint: Schedule availability (every 15 min, 6 AM - 11 PM)
// Speed: 30 km/h for all vehicles
void solveProblem4(Graph &graph, 
                   double srcLat, double srcLon, 
                   double destLat, double destLon,
                   int startTimeMin);

// Problem 5: Find fastest route with time schedules
// Optimizes: Travel time (minutes)
// Constraint: Schedule availability (every 15 min, 6 AM - 11 PM)
// Speed: 10 km/h for all vehicles
void solveProblem5(Graph &graph, 
                   double srcLat, double srcLon, 
                   double destLat, double destLon,
                   int startTimeMin);

// Problem 6: Find cheapest route meeting deadline
// Optimizes: Cost (₹)
// Constraint: Must arrive before deadline
// Speeds: Different per mode (Car 20, Metro 15, Bikolpo 10, Uttara 12 km/h)
// Schedules: Different per mode
void solveProblem6(Graph &graph, 
                   double srcLat, double srcLon, 
                   double destLat, double destLon,
                   int startTimeMin,
                   int deadlineMin);

#endif