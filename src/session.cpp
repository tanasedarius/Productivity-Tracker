#include "session.h"

// Creates a new session and auto-calculates total time and efficiency.
Session::Session(const std::string &group_name, const std::string &name, int duration_work, int duration_pause,
                 Timestamp timestamp)
    : id_(-1), group_name_(group_name), name_(name), duration_work_(duration_work), duration_pause_(duration_pause),
      timestamp_(timestamp) {
    duration_total_ = duration_work + duration_pause;

    // Prevent division by zero if the session has no recorded time.
    efficiency_ = (duration_total_ > 0) ? (static_cast<double>(duration_work) * 100.0 / duration_total_) : 0.0;
}

// Reconstructs an existing session (typically loaded from the database).
Session::Session(int id, const std::string &group_name, const std::string &name, int duration_work, int duration_pause,
                 int duration_total, double efficiency, Timestamp timestamp)
    : id_(id),
      group_name_(group_name),
      name_(name),
      duration_work_(duration_work),
      duration_pause_(duration_pause),
      duration_total_(duration_total),
      efficiency_(efficiency),
      timestamp_(timestamp) {
}

// --- Getters & Setters ---
std::string Session::get_group_name() const { return group_name_; }
std::string Session::get_name() const { return name_; }
int Session::get_duration_work() const { return duration_work_; }
int Session::get_duration_pause() const { return duration_pause_; }
int Session::get_duration_total() const { return duration_total_; }
double Session::get_efficiency() const { return efficiency_; }
void Session::set_id(int id) { id_ = id; }
int Session::get_id() const { return id_; }
Timestamp Session::get_timestamp() const { return timestamp_; }
