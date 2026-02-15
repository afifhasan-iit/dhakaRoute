#include "Utils.h"
#include <cmath>
#include <sstream>
#include <algorithm>
#include <cctype>

using namespace std;

// ============================================================================
// MODE NAME HELPER
// ============================================================================

string getModeName(Mode mode) {
    switch(mode) {
        case MODE_WALK: return "Walk";
        case MODE_CAR: return "Car";
        case MODE_METRO: return "Metro";
        case MODE_BIKOLPO: return "Bikolpo Bus";
        case MODE_UTTARA: return "Uttara Bus";
        default: return "Unknown";
    }
}

// ============================================================================
// HAVERSINE DISTANCE CALCULATION
// ============================================================================

double haversineDistance(double lat1, double lon1, double lat2, double lon2) {
    // Convert latitude and longitude differences to radians
    double dLat = (lat2 - lat1) * PI / 180.0;
    double dLon = (lon2 - lon1) * PI / 180.0;
    
    // Convert latitudes to radians
    lat1 = lat1 * PI / 180.0;
    lat2 = lat2 * PI / 180.0;
    
    // Haversine formula
    // a = sin²(Δlat/2) + cos(lat1) * cos(lat2) * sin²(Δlon/2)
    double a = sin(dLat/2) * sin(dLat/2) +
               cos(lat1) * cos(lat2) *
               sin(dLon/2) * sin(dLon/2);
    
    // c = 2 * atan2(√a, √(1-a))
    double c = 2 * atan2(sqrt(a), sqrt(1-a));
    
    // Distance = Earth's radius * c
    return EARTH_RADIUS_KM * c;
}

// ============================================================================
// TIME PARSING AND FORMATTING
// ============================================================================

int parseTime(string timeStr) {     //timeStr= "5:43 PM" or "9:30 AM"

    // Example: "5:43 PM" -> 17*60 + 43 = 1063 minutes from 12 AM
    
    int hour, minute;
    char colon, space;
    string period;
    
    // Use stringstream to parse
    stringstream ss(timeStr);
    ss >> hour >> colon >> minute >> period;
    
    if (ss.fail() || colon != ':') {
        return -1;  // Invalid format
    }
    
    // Convert to uppercase for comparison
    transform(period.begin(), period.end(), period.begin(), ::toupper);
    
    // Convert to 24-hour format
    if (period == "PM") {
        if (hour != 12) hour += 12;  // 1 PM -> 13, 2 PM -> 14, etc.
    } else if (period == "AM") {
        if (hour == 12) hour = 0;    // 12 AM -> 0 (midnight)
    } else {
        return -1;  // Invalid period
    }
    
    // Convert to minutes from midnight
    int totalMinutes = hour * 60 + minute;
    
    // Validate range
    if (totalMinutes < 0 || totalMinutes >= 24 * 60) {
        return -1;
    }
    
    return totalMinutes;
}

string formatTime(int minutes) {
    // Convert minutes from midnight back to "5:43 PM" format
    
    int hour = (minutes / 60) % 24;  // Get hour (0-23)
    int min = minutes % 60;           // Get minute (0-59)
    
    // Determine AM/PM and convert to 12-hour format
    string period = "AM";
    int displayHour = hour;
    
    if (hour >= 12) {
        period = "PM";
        if (hour > 12) displayHour = hour - 12;  // 13 -> 1, 14 -> 2, etc.
    }
    if (displayHour == 0) displayHour = 12;  // Midnight -> 12 AM
    
    // Format as string
    stringstream ss;
    ss << displayHour << ":";
    if (min < 10) ss << "0";  // Add leading zero for minutes
    ss << min << " " << period;
    
    return ss.str();
}

// ============================================================================
// WAITING TIME CALCULATIONS (SCHEDULE HANDLING)
// ============================================================================

double getWaitingTime(int currentTimeMin, Mode mode) {
    // For Problems 4 & 5: Uniform schedule (every 15 minutes, 6 AM - 11 PM)
    
    // Cars and walking don't require waiting
    if (mode == MODE_CAR || mode == MODE_WALK) {
        return 0.0;
    }
    
    // Check if service is currently running
    if (currentTimeMin < SCHEDULE_START_MIN || currentTimeMin >= SCHEDULE_END_MIN) {
        return INF;  // Service not available at this time
    }
    
    // Calculate how long since service started today
    int minutesSinceStart = currentTimeMin - SCHEDULE_START_MIN;
    
    // Calculate time until next departure
    // Example: If service every 15 min, and 7 minutes have passed since last one,
    // then next one is in 15 - 7 = 8 minutes
    int minutesToNext = SCHEDULE_INTERVAL_MIN - (minutesSinceStart % SCHEDULE_INTERVAL_MIN);
    
    // If result is exactly the interval, it means vehicle is here now
    if (minutesToNext == SCHEDULE_INTERVAL_MIN) {
        minutesToNext = 0;
    }
    
    return (double)minutesToNext;
}

double getWaitingTimeProblem6(int currentTimeMin, Mode mode) {
    // For Problem 6: Different schedules for each mode
    
    // Cars and walking don't require waiting
    if (mode == MODE_CAR || mode == MODE_WALK) {
        return 0.0;
    }
    
    int startTime, endTime, interval;
    
    // Set schedule parameters based on mode
    switch(mode) {
        case MODE_METRO:
            startTime = METRO_START_MIN;      // 1 AM
            endTime = METRO_END_MIN;          // 11 PM
            interval = METRO_INTERVAL_MIN;    // Every 5 minutes
            break;
        case MODE_BIKOLPO:
            startTime = BIKOLPO_START_MIN;    // 7 AM
            endTime = BIKOLPO_END_MIN;        // 10 PM
            interval = BIKOLPO_INTERVAL_MIN;  // Every 20 minutes
            break;
        case MODE_UTTARA:
            startTime = UTTARA_START_MIN;     // 6 AM
            endTime = UTTARA_END_MIN;         // 11 PM
            interval = UTTARA_INTERVAL_MIN;   // Every 10 minutes
            break;
        default:
            return 0.0;
    }
    
    // Check if service is running at current time
    if (currentTimeMin < startTime || currentTimeMin >= endTime) {
        return INF;  // Service not available
    }
    
    // Calculate time to next departure (same logic as getWaitingTime)
    int minutesSinceStart = currentTimeMin - startTime;
    int minutesToNext = interval - (minutesSinceStart % interval);
    
    if (minutesToNext == interval) {
        minutesToNext = 0;  // Vehicle available now
    }
    
    return (double)minutesToNext;
}

// ============================================================================
// STRING UTILITY FUNCTIONS
// ============================================================================

void trim(string &s) {
    // Remove leading whitespace
    s.erase(s.begin(), find_if(s.begin(), s.end(), [](unsigned char ch) {
        return !isspace(ch);
    }));
    
    // Remove trailing whitespace
    s.erase(find_if(s.rbegin(), s.rend(), [](unsigned char ch) {
        return !isspace(ch);
    }).base(), s.end());
}

vector<string> splitCSV(string line) {
    // Split a CSV line by commas and trim each token
    vector<string> tokens;
    stringstream ss(line);
    string token;
    
    while (getline(ss, token, ',')) {
        trim(token);
        tokens.push_back(token);
    }
    
    return tokens;
}

bool isNumber(string s) {
    // Check if a string represents a valid number
    if (s.empty()) return false;
    
    trim(s);
    if (s.empty()) return false;
    
    // Try to convert to double
    char* end = nullptr;
    strtod(s.c_str(), &end);
    
    // If end points to null terminator, entire string was a number
    return end != nullptr && *end == '\0';
}