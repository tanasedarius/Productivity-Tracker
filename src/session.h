/**
 * @file session.h
 * @brief Defines a tracked work session with statistics.
 */

#pragma once
#include <string>
#include "timestamp.h"

/**
 * @class Session
 * @brief Represents a productivity session, tracking work/pause times and efficiency.
 */
class Session
{
private:
    int id_;
    std::string group_name_;
    std::string name_;
    int duration_work_;
    int duration_pause_;
    int duration_total_;
    double efficiency_;
    Timestamp timestamp_;

public:
    /**
     * @brief Creates a new session, auto-calculating total time and efficiency.
     * @param group_name Name of the activity group.
     * @param name Name of the activity.
     * @param duration_work Work time in seconds.
     * @param duration_pause Pause time in seconds.
     * @param timestamp Date and time the session was recorded.
     */
    Session(const std::string &group_name, const std::string &name, int duration_work, int duration_pause, Timestamp timestamp);

    /**
     * @brief Reconstructs an existing session (e.g., loaded from a database).
     */
    Session(int id_, const std::string &group_name, const std::string &name, int duration_work, int duration_pause, int duration_total, double efficiency, Timestamp timestamp);

    /**
     * @brief Returns the activity group name.
     */
    std::string get_group_name() const;

    /**
     * @brief Returns the activity name.
     */
    std::string get_name() const;

    /**
     * @brief Returns the work duration in seconds.
     */
    int get_duration_work() const;

    /**
     * @brief Returns the pause duration in seconds.
     */
    int get_duration_pause() const;

    /**
     * @brief Returns the total session duration (work + pause) in seconds.
     */
    int get_duration_total() const;

    /**
     * @brief Returns the work efficiency as a percentage (0.0 to 100.0).
     */
    double get_efficiency() const;

    /**
     * @brief Sets the database ID.
     */
    void set_id(int id);

    /**
     * @brief Returns the database ID (-1 if unsaved).
     */
    int get_id() const;

    /**
     * @brief Returns the session's creation timestamp.
     */
    Timestamp get_timestamp() const;
};