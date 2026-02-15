#ifndef UTILS_H
#define UTILS_H

#include <string>
#include <vector>

using namespace std;



// Transportation modes available in Dhaka
enum Mode {
    MODE_WALK,      // Walking (2 km/h)
    MODE_CAR,       // Personal car
    MODE_METRO,     // Metro rail
    MODE_BIKOLPO,   // Bikolpo Bus
    MODE_UTTARA     // Uttara Bus
};

// Earth's radius in kilometers for Haversine formula
#define EARTH_RADIUS_KM 6371.0
#define PI 3.14159265358979323846

// Walking speed in km/h
#define WALK_SPEED_KMH 2.0

// Problem 4 & 5: All vehicles at same speed
#define VEHICLE_SPEED_KMH 30.0           // Problem 4
#define VEHICLE_SPEED_PROBLEM5_KMH 10.0  // Problem 5

// Problem 4 & 5: Uniform schedule (every 15 min, 6 AM to 11 PM)
#define SCHEDULE_INTERVAL_MIN 15
#define SCHEDULE_START_MIN (6 * 60)   // 6 AM in minutes
#define SCHEDULE_END_MIN (23 * 60)    // 11 PM in minutes

// Problem 6: Different speeds for each mode
#define CAR_SPEED_PROBLEM6_KMH 20.0
#define METRO_SPEED_PROBLEM6_KMH 15.0
#define BIKOLPO_SPEED_PROBLEM6_KMH 10.0
#define UTTARA_SPEED_PROBLEM6_KMH 12.0

// Problem 6: Different schedules for each mode
#define BIKOLPO_INTERVAL_MIN 20
#define BIKOLPO_START_MIN (7 * 60)    // 7 AM
#define BIKOLPO_END_MIN (22 * 60)     // 10 PM

#define UTTARA_INTERVAL_MIN 10
#define UTTARA_START_MIN (6 * 60)     // 6 AM
#define UTTARA_END_MIN (23 * 60)      // 11 PM

#define METRO_INTERVAL_MIN 5
#define METRO_START_MIN (1 * 60)      // 1 AM
#define METRO_END_MIN (23 * 60)       // 11 PM

// Large value representing infinity
#define INF 9999999999.0

// ============================================================================
// UTILITY FUNCTIONS
// ============================================================================

// Get human-readable name for a transportation mode
string getModeName(Mode mode);


// Returns: distance in kilometers
double haversineDistance(double lat1, double lon1, double lat2, double lon2);

// Parse time string "5:43 PM" or "9:30 AM" into minutes from midnight
// Returns: minutes (0-1439), or -1 if invalid format
int parseTime(string timeStr);

// Format minutes from midnight into "5:43 PM" format
// Parameters: minutes (0-1439)
// Returns: formatted time string
string formatTime(int minutes);

// Calculate waiting time for next scheduled bus/metro (Problems 4 & 5)
// Parameters: currentTimeMin = current time in minutes, mode = transportation mode
// Returns: minutes to wait, or INF if service not available
double getWaitingTime(int currentTimeMin, Mode mode);

// Calculate waiting time for Problem 6 (different schedules per mode)
// Parameters: currentTimeMin = current time in minutes, mode = transportation mode
// Returns: minutes to wait, or INF if service not available
double getWaitingTimeProblem6(int currentTimeMin, Mode mode);

// Trim whitespace from both ends of a string
void trim(string &s);

// Split CSV line by commas, handling whitespace
// Returns: vector of tokens
vector<string> splitCSV(string line);

// Check if a string represents a number
bool isNumber(string s);

#endif