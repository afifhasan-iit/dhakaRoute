#include <iostream>
#include <string>
#include "Graph.h"
#include "Utils.h"

using namespace std;

int main() {
    // Build the multi-modal transportation graph by parsing CSV files
    cout << "=== Loading Transportation Network ===" << endl;
    
    Graph graph;
    
    // Parse road network (car edges)
    parseRoadmapCSV(graph, "Roadmap-Dhaka.csv");
    
    // Parse metro rail network
    parseMetroCSV(graph, "Routemap-DhakaMetroRail.csv");
    
    // Parse bus networks (two different bus companies)
    parseBusCSV(graph, "Routemap-BikolpoBus.csv", MODE_BIKOLPO);
    parseBusCSV(graph, "Routemap-UttaraBus.csv", MODE_UTTARA);
    
    cout << "\nTotal nodes in graph: " << graph.nodes.size() << endl;
    
    // Calculate total edges
    int totalEdges = 0;
    for (auto &adjList : graph.adjList) {
        totalEdges += adjList.size();
    }
    cout << "Total edges in graph: " << totalEdges << endl;
    
    if (graph.nodes.empty()) {
        cout << "Error: No nodes loaded. Please check CSV files." << endl;
        return 1;
    }
    
    // Main menu loop
    while (true) {
        cout << "\n======================================" << endl;
        cout << "         Mr. Efficient" << endl;
        cout << "======================================" << endl;
        cout << "[1] Shortest Car Route [Problem 1]" << endl;
        cout << "[2] Cheapest Route (Car + Metro) [Problem 2]" << endl;
        cout << "[3] Cheapest Route (All Modes) [Problem 3]" << endl;
        cout << "[4] Cheapest with Schedules [Problem 4]" << endl;
        cout << "[5] Fastest with Schedules [Problem 5]" << endl;
        cout << "[6] Cheapest with Deadline [Problem 6]" << endl;
        cout << "[7] Quit" << endl;
        cout << "======================================" << endl;
        
        int choice;
        cout << "Enter Choice: ";
        cin >> choice;
        
        if (choice == 7) {
            cout << "Goodbye!" << endl;
            break;
        }
        
        // Get source and destination coordinates
        double srcLat, srcLon, destLat, destLon;
        
        cout << "\nEnter source latitude and longitude: ";
        cin >> srcLat >> srcLon;
        
        cout << "Enter destination latitude and longitude: ";
        cin >> destLat >> destLon;
        
        // Call appropriate problem solver
        switch (choice) {
            case 1:
                // Problem 1: Shortest distance using only car
                solveProblem1(graph, srcLat, srcLon, destLat, destLon);
                break;
                
            case 2:
                // Problem 2: Cheapest route using car and metro
                solveProblem2(graph, srcLat, srcLon, destLat, destLon);
                break;
                
            case 3:
                // Problem 3: Cheapest route using all modes
                solveProblem3(graph, srcLat, srcLon, destLat, destLon);
                break;
                
            case 4: {
                // Problem 4: Cheapest with time schedules
                string timeInput;
                cout << "Enter starting time (e.g., '5:43 PM' or '9:30 AM'): ";
                cin.ignore();  // Clear newline from previous cin
                getline(cin, timeInput);
                
                int startTimeMin = parseTime(timeInput);
                if (startTimeMin < 0) {
                    cout << "Invalid time format. Please use format like '5:43 PM'" << endl;
                    break;
                }
                
                solveProblem4(graph, srcLat, srcLon, destLat, destLon, startTimeMin);
                break;
            }
                
            case 5: {
                // Problem 5: Fastest route (minimize time)
                string timeInput;
                cout << "Enter starting time (e.g., '5:43 PM' or '9:30 AM'): ";
                cin.ignore();
                getline(cin, timeInput);
                
                int startTimeMin = parseTime(timeInput);
                if (startTimeMin < 0) {
                    cout << "Invalid time format. Please use format like '5:43 PM'" << endl;
                    break;
                }
                
                solveProblem5(graph, srcLat, srcLon, destLat, destLon, startTimeMin);
                break;
            }
                
            case 6: {
                // Problem 6: Cheapest meeting deadline
                string timeInput, deadlineInput;
                
                cout << "Enter starting time (e.g., '5:43 PM' or '9:30 AM'): ";
                cin.ignore();
                getline(cin, timeInput);
                
                int startTimeMin = parseTime(timeInput);
                if (startTimeMin < 0) {
                    cout << "Invalid time format." << endl;
                    break;
                }
                
                cout << "Enter deadline time (e.g., '8:12 PM' or '11:00 AM'): ";
                getline(cin, deadlineInput);
                
                int deadlineMin = parseTime(deadlineInput);
                if (deadlineMin < 0) {
                    cout << "Invalid deadline format." << endl;
                    break;
                }
                
                solveProblem6(graph, srcLat, srcLon, destLat, destLon, 
                             startTimeMin, deadlineMin);
                break;
            }
                
            default:
                cout << "Invalid choice. Please select 1-7." << endl;
                break;
        }
    }
    
    return 0;
}