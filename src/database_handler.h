/**
 * @file database_handler.h
 * @brief Handles SQLite database operations for sessions.
 *
 * Provides a wrapper around SQLite3 to save, retrieve, and delete session data,
 * as well as advanced querying using the custom Productivity Query Language (PQL).
 */

#pragma once
#include <sqlite3.h>
#include <string>
#include <vector>
#include "session.h"
#include "app_context.h"

/**
 * @class DatabaseHandler
 * @brief Manages persistent storage and retrieval of productivity sessions.
 */
class DatabaseHandler
{
private:
    sqlite3 *db_;             ///< Pointer to the active SQLite database connection.
    std::string db_path_;     ///< Filepath to the SQLite database file.

public:
    /**
     * @brief Initializes the handler with a database file path.
     * @param db_path Path to the SQLite database file.
     */
    DatabaseHandler(const std::string &db_path);

    /**
     * @brief Ensures the database connection is closed upon destruction.
     */
    ~DatabaseHandler();

    /**
     * @brief Opens the connection to the SQLite database.
     * @return True if opened successfully, false otherwise.
     */
    bool open();

    /**
     * @brief Closes the active database connection.
     */
    void close();

    /**
     * @brief Creates the required sessions table if it does not exist.
     * @return True if created or already exists, false on error.
     */
    bool create_table();

    /**
     * @brief Inserts a session into the database and updates its internal ID.
     * @param session The session object to save.
     * @return True if saved successfully, false otherwise.
     */
    bool save_session(Session &session);

    /**
     * @brief Removes a session from the database by its ID.
     * @param id The unique identifier of the session to delete.
     * @return True if deleted successfully, false otherwise.
     */
    bool delete_session(int id);

    /**
     * @brief Retrieves all stored sessions from the database.
     * @return A vector containing all saved sessions, ordered descending by ID.
     */
    std::vector<Session> get_all_sessions();

    /**
     * @brief Retrieves a list of all distinct activity group names used in past sessions.
     * 
     * Useful for populating UI dropdowns or providing auto-completion suggestions.
     * 
     * @return A vector of unique group name strings.
     */
    std::vector<std::string> get_unique_groups();

    /**
     * @brief Fetches the group name of the most recently recorded session.
     * 
     * Used to pre-fill the group input field in the UI to streamline starting consecutive tasks.
     * 
     * @return The most recent group name as a string, or an empty string if no sessions exist.
     */
    std::string get_last_used_group();

    /**
     * @brief Retrieves the most recently completed session.
     * 
     * Primarily used to populate the "Latest Activity" UI panel.
     * 
     * @return A Session object representing the last entry. Returns a default/empty Session if the database is empty.
     */
    Session get_last_session();

    /**
     * @brief Searches for sessions using the Productivity Query Language (PQL).
     * 
     * Parses the custom PQL string to dynamically generate an SQL WHERE clause
     * and returns all matching sessions.
     * 
     * @param query The PQL string entered by the user (e.g., "date:last7d group:study work:>60m").
     * @param has_error Reference flag set to true if the PQL syntax is invalid.
     * @return A vector of Session objects that match the search criteria.
     */
    std::vector<Session> search_sessions(const std::string &query, bool &has_error);

    /**
     * @brief Recalculates and caches the top performing groups.
     * 
     * Aggregates all session data to determine total work time, session counts, 
     * and average efficiency per group. It sorts the results and updates the 
     * `top_groups` cache to reflect the Top 3 Groups for the UI dashboard.
     * 
     * @param db Reference to the DatabaseHandler (typically *this) to fetch current sessions.
     * @param ctx Reference to the global AppContext where the top_groups cache will be updated.
     */
    static void update_top_groups(DatabaseHandler& db, AppContext &ctx); 
};