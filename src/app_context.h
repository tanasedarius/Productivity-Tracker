/**
 * @file AppContext.h
 * @brief Global application state and core data structures.
 */

#pragma once
#include "enums.h"
#include "timer.h"
#include <deque>
#include <fstream>
#include <vector>
#include <string>
#include <ctime>

#define APP_VERSION "1.0.0"
#define APP_NAME "Productivity Tracker"

/**
 * @brief Aggregated statistical data for a specific activity group.
 *
 * Used primarily for rendering the Top Groups panel, keeping track of
 * total work duration, session counts, and overall efficiency metrics.
 */
struct GroupStats
{
    std::string name;            ///< The name of the activity group
    long long total_work = 0;    ///< Total accumulated work time in seconds
    int session_count = 0;       ///< Total number of sessions completed under this group
    float sum_efficiency = 0.0f; ///< Cumulative efficiency percentage used to calculate the average
};

/**
 * @brief Container for all UI state, session data, and backend buffers.
 *
 * Grouping these variables avoids scattering them as local/static in main()
 * and makes it easy to pass the entire state to rendering functions.
 */
struct AppContext
{
    // --- Core Application State ---
    AppState current_state = AppState::IDLE;        ///< Current application state (IDLE, WORKING, PAUSED)
    char group_name[128] = "";                      ///< Buffer for user-entered activity group name
    char activity_name[128] = "";                   ///< Buffer for user-entered activity name
    Timer session_timer;                            ///< Tracks active work time during a session
    Timer pause_timer;                              ///< Tracks total pause time during a session
    Timestamp session_start_time;                   ///< Captures the exact moment the session started
    std::string status_message = "Ready to start."; ///< Feedback message displayed on the main UI

    // --- UI Window & Modal Flags ---
    bool show_dashbord = false;       ///< Flag to display the main dashboard view
    bool show_history_window = false; ///< Flag to show/hide the history browser modal
    bool show_exit_popup = false;     ///< Flag to trigger the application exit confirmation modal
    bool open_delete_popup = false;   ///< Flag to trigger the session deletion confirmation modal
    bool should_close = false;        ///< Signal flag to safely terminate the main GLFW rendering loop
    bool force_window_resize = false; ///< Trigger to force layout recalculations or window resizing

    // --- Database & Cache Context ---
    int session_id_to_delete = -1;                   ///< ID of the session currently selected for deletion
    std::vector<Session> history_sessions;           ///< Cached list of all recorded sessions loaded from the database
    Session last_session{"", "", 0, 0, Timestamp()}; ///< Cache of the most recent session (used in Latest Activity & Group Stats)

    // --- Group Management ---
    std::vector<std::string> available_groups; ///< List of unique groups retrieved from the database
    std::vector<GroupStats> top_groups;        ///< Sorted cache containing the top 3 most active groups
    bool is_creating_new_group = false;        ///< Flag indicating the user is typing a custom group name
    bool groups_loaded = false;                ///< Ensures database group fetching only occurs when necessary
    int selected_group_index = 0;              ///< Currently selected index in the group dropdown menu

    // --- PQL (Productivity Query Language) Search ---
    char search_buffer[256] = "";           ///< Input buffer for the history PQL search bar
    bool clear_search_buffer = false;       ///< Trigger to clear the PQL search input field
    bool search_error = false;              ///< Flag indicating if the current PQL syntax is invalid
    std::vector<Session> filtered_sessions; ///< Subset of history_sessions matching the active PQL query
    bool show_pql_guide = false;            ///< Toggle for displaying the PQL syntax help tooltip/window

    // --- Auxiliary Widgets ---
    char notepad_buffer[4096] = ""; ///< Buffer for the persistent scratchpad/notepad widget

    // --- UI Auto-Focus Flags ---
    bool focus_textbox = false;           ///< Trigger to automatically set keyboard focus on the activity name input
    bool focus_new_group_textbox = false; ///< Trigger to automatically set keyboard focus on the new group input

    // --- System Terminal Logs ---
    std::deque<std::string> system_logs;                    ///< In-memory queue storing the latest 50 logs for the retro UI
    const std::string LOG_FILE = "data/system_logs.txt"; ///< Path to the persistent system log file

    bool show_info_window = false; ///< Flag to display the application info/about modal

    /**
     * @brief Loads recent system logs from persistent storage at application startup.
     *
     * Reads the entire log file, strips the date component to keep the UI clean,
     * limits the in-memory representation to the last 50 entries to maintain
     * rendering performance, and appends a synthetic boot event.
     */
    void load_logs()
    {
        std::ifstream in_file(LOG_FILE);
        if (!in_file.is_open())
            return;

        std::string line;
        std::vector<std::string> all_lines;

        while (std::getline(in_file, line))
        {
            if (!line.empty())
            {
                // Strip the [DD-MM-YYYY] prefix (13 chars) for a cleaner UI display
                if (line.size() > 13 && line[0] == '[' && line[11] == ']')
                    all_lines.push_back(line.substr(13));
                else
                    all_lines.push_back(line);
            }
        }
        in_file.close();

        int start_idx = 0;
        if (all_lines.size() > 50)
        {
            start_idx = all_lines.size() - 50;
        }

        for (size_t i = start_idx; i < all_lines.size(); ++i)
        {
            system_logs.push_back(all_lines[i]);
        }

        add_log("SYS_BOOT: Terminal initialized. Loaded " + std::to_string(system_logs.size() - 1) + " previous logs.");
    }

    /**
     * @brief Appends a new event message to both the UI and the persistent log file.
     *
     * Formats the string with the current time for the UI terminal display, and with
     * both date and time for long-term file storage. Limits the UI deque size to 50
     * to prevent ImGui rendering bottlenecks.
     *
     * @param msg The raw event message string to be logged.
     */
    void add_log(const std::string &msg)
    {
        std::time_t t = std::time(nullptr);
        std::tm *now = std::localtime(&t);

        char time_buf[16];
        snprintf(time_buf, sizeof(time_buf), "[%02d:%02d:%02d]", now->tm_hour, now->tm_min, now->tm_sec);

        char date_buf[16];
        snprintf(date_buf, sizeof(date_buf), "[%02d-%02d-%04d]", now->tm_mday, now->tm_mon + 1, now->tm_year + 1900);

        std::string ui_msg = std::string(time_buf) + " " + msg;
        std::string file_msg = std::string(date_buf) + " " + ui_msg;

        system_logs.push_back(ui_msg);
        if (system_logs.size() > 50)
        {
            system_logs.erase(system_logs.begin()); // Maintains max size of 50 in RAM
        }

        std::ofstream out_file(LOG_FILE, std::ios_base::app);
        if (out_file.is_open())
        {
            out_file << file_msg << "\n";
            out_file.close();
        }
    }
};