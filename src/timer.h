/**
 * @file timer.h
 * @brief High-resolution stopwatch with pause/resume support.
 *
 * Accumulates time across multiple pause/resume cycles.
 */

#pragma once

#include <chrono>
#include <string>

/**
 * @class Timer
 * @brief Tracks elapsed seconds, supporting pause and resume.
 */
class Timer
{
private:
    std::chrono::time_point<std::chrono::high_resolution_clock> start_time_;
    bool is_on_ = false;
    long long accumulated_seconds_ = 0;

public:
    Timer();

    /**
     * @brief Resets and starts a fresh timing session.
     */
    void start();

    /**
     * @brief Stops the timer and commits the final time chunk.
     */
    void stop();

    /**
     * @brief Pauses the timer, adding the current segment to the total.
     */
    void pause();

    /**
     * @brief Resumes the timer without resetting accumulated time.
     */
    void resume();

    /**
     * @brief Returns total elapsed seconds (live or accumulated).
     */
    long long get_elapsed_seconds() const;

    /**
     * @brief Formats seconds as "HH:MM:SS" with zero-padding.
     * @param seconds Total seconds to format.
     * @return Formatted time string.
     */
    static std::string get_formatted_duration(long long seconds);
};