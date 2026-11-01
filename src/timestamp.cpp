#include "timestamp.h"
#include <chrono>
#include <ctime>
#include <iomanip>
#include <sstream>

// Initialize to a zero state.
Timestamp::Timestamp() : year_(0), month_(0), day_(0), hour_(0), minute_(0) {}

// Initialize with specific date and time components.
Timestamp::Timestamp(int year, int month, int day, int hour, int minute) 
    : year_(year), month_(month), day_(day), hour_(hour), minute_(minute) {}

// --- Getters ---
int Timestamp::get_year() const { return year_; }
int Timestamp::get_month() const { return month_; }
int Timestamp::get_day() const { return day_; }
int Timestamp::get_hour() const { return hour_; }
int Timestamp::get_minute() const { return minute_; }

std::string Timestamp::get_formatted_date() const
{
    std::ostringstream oss;
    
    // Formats date as DD/MM/YYYY with zero-padding for single digits.
    oss << std::setfill('0')
        << std::setw(2) << day_ << "/"
        << std::setw(2) << month_ << "/"
        << year_;
        
    return oss.str();
}

std::string Timestamp::get_formatted_time() const
{
    std::ostringstream oss;
    
    // Formats time as HH:MM with zero-padding.
    oss << std::setfill('0')
        << std::setw(2) << hour_ << ":"
        << std::setw(2) << minute_;
        
    return oss.str();
}

Timestamp Timestamp::get_current_timestamp()
{
    std::time_t t = std::time(nullptr);
    std::tm* now = std::localtime(&t);

    return {
        now->tm_year + 1900,
        now->tm_mon + 1,
        now->tm_mday,
        now->tm_hour,
        now->tm_min
    };
}