/**
 * @file timestamp.h
 * @brief Provides a simple date and time structure.
 */

#pragma once
#include <string>

/**
 * @class Timestamp
 * @brief Represents a specific date and time down to the minute.
 */
class Timestamp
{
private:
    int year_;
    int month_;
    int day_;
    int hour_;
    int minute_;

public:
    /**
     * @brief Initializes a zeroed timestamp.
     */
    Timestamp();

    /**
     * @brief Initializes the timestamp with specific date and time values.
     */
    Timestamp(int year, int month, int day, int hour, int minute);

    /**
     * @brief Returns the year.
     */
    int get_year() const;

    /**
     * @brief Returns the month (1-12).
     */
    int get_month() const;

    /**
     * @brief Returns the day of the month (1-31).
     */
    int get_day() const;

    /**
     * @brief Returns the hour in 24h format (0-23).
     */
    int get_hour() const;

    /**
     * @brief Returns the minute (0-59).
     */
    int get_minute() const;

    /**
     * @brief Formats the date as "DD/MM/YYYY".
     */
    std::string get_formatted_date() const;

    /**
     * @brief Formats the time as "HH:MM".
     */
    std::string get_formatted_time() const;

    /**
     * @brief Captures and returns the system's current local time.
     */
    static Timestamp get_current_timestamp();
};