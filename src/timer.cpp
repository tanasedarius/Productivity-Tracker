#include <iomanip>
#include <sstream>
#include "timer.h"

// Initialize the timer in a stopped state.
Timer::Timer() : is_on_(false) {
}

void Timer::start() {
    accumulated_seconds_ = 0;
    start_time_ = std::chrono::high_resolution_clock::now();
    is_on_ = true;
}

void Timer::pause() {
    if (is_on_) {
        auto now = std::chrono::high_resolution_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - start_time_);

        // Commit the time elapsed since the last start/resume to the accumulator.
        accumulated_seconds_ += elapsed.count();
        is_on_ = false;
    }
}

void Timer::resume() {
    if (!is_on_) {
        // Record a new starting point without clearing the previously accumulated time.
        start_time_ = std::chrono::high_resolution_clock::now();
        is_on_ = true;
    }
}

void Timer::stop() {
    // Stopping is functionally identical to pausing, as it just commits the final time chunk.
    pause();
}

long long Timer::get_elapsed_seconds() const {
    if (is_on_) {
        // If the timer is active, calculate the live time by adding 
        // the current unpaused chunk to the stored accumulator.
        auto now = std::chrono::high_resolution_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - start_time_);
        return accumulated_seconds_ + elapsed.count();
    }

    // If paused, the accumulator holds the exact total time.
    return accumulated_seconds_;
}

std::string Timer::get_formatted_duration(long long seconds) {
    std::ostringstream oss;

    // Format the time as HH:MM:SS.
    // std::setfill('0') and std::setw(2) ensure that single digits are padded (e.g., "05" instead of "5").
    oss << std::setfill('0')
            << std::setw(2) << seconds / 3600 << ":"
            << std::setw(2) << (seconds % 3600) / 60 << ":"
            << std::setw(2) << seconds % 60;

    return oss.str();
}
