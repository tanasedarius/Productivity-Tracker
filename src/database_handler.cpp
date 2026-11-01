#include <iostream>
#include <cstdio>
#include <unordered_map>
#include <algorithm>
#include "database_handler.h"
#include "query_parser.h"

namespace {
    /**
     * @brief Safely extracts text from an SQLite column to prevent std::string(nullptr) crashes.
     */
    std::string SafeColumnText(sqlite3_stmt *stmt, int col) {
        const unsigned char *text = sqlite3_column_text(stmt, col);
        return text ? reinterpret_cast<const char *>(text) : "";
    }
}

DatabaseHandler::DatabaseHandler(const std::string &db_path) : db_(nullptr), db_path_(db_path) {
}

DatabaseHandler::~DatabaseHandler() {
    close();
}

bool DatabaseHandler::open() {
    int result = sqlite3_open(db_path_.c_str(), &db_);
    if (result != SQLITE_OK) {
        std::cerr << "Error opening database: " << sqlite3_errmsg(db_) << std::endl;
        return false;
    }
    return true;
}

void DatabaseHandler::close() {
    if (db_) {
        sqlite3_close(db_);
        db_ = nullptr;
    }
}

bool DatabaseHandler::create_table() {
    // Define the core schema. AUTOINCREMENT ensures chronological consistency
    // and unique identification for session history management.
    std::string sql = "CREATE TABLE IF NOT EXISTS sessions ("
            "id INTEGER PRIMARY KEY AUTOINCREMENT, "
            "group_name TEXT NOT NULL, "
            "name TEXT NOT NULL, "
            "duration_work INTEGER NOT NULL, "
            "duration_pause INTEGER NOT NULL, "
            "duration_total INTEGER NOT NULL, "
            "efficiency REAL NOT NULL, "
            "timestamp TEXT NOT NULL);";

    char *err_msg = nullptr;
    int result = sqlite3_exec(db_, sql.c_str(), nullptr, nullptr, &err_msg);

    if (result != SQLITE_OK) {
        std::cerr << "Error creating database table: " << err_msg << std::endl;
        sqlite3_free(err_msg);
        return false;
    }
    return true;
}

bool DatabaseHandler::save_session(Session &session) {
    Timestamp ts = session.get_timestamp();
    char ts_buffer[64];

    // Format timestamp for SQLite compatibility and consistent chronological sorting.
    snprintf(ts_buffer, sizeof(ts_buffer), "%04d-%02d-%02d %02d:%02d",
             ts.get_year(), ts.get_month(), ts.get_day(), ts.get_hour(), ts.get_minute());

    // Utilize prepared statements with placeholders (?) to mitigate SQL injection risks.
    std::string sql =
            "INSERT INTO sessions (group_name, name, duration_work, duration_pause, duration_total, efficiency, timestamp) VALUES (?, ?, ?, ?, ?, ?, ?);";

    sqlite3_stmt *stmt;

    // Pre-compile the SQL statement into bytecode.
    if (sqlite3_prepare_v2(db_, sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
        std::cerr << "Error preparing insert statement: " << sqlite3_errmsg(db_) << std::endl;
        return false;
    }

    // Bind the actual object data to the prepared statement parameters.
    // SQLITE_TRANSIENT instructs SQLite to allocate its own memory copy for the string data.
    sqlite3_bind_text(stmt, 1, session.get_group_name().c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, session.get_name().c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 3, session.get_duration_work());
    sqlite3_bind_int(stmt, 4, session.get_duration_pause());
    sqlite3_bind_int(stmt, 5, session.get_duration_total());
    sqlite3_bind_double(stmt, 6, session.get_efficiency());
    sqlite3_bind_text(stmt, 7, ts_buffer, -1, SQLITE_TRANSIENT);

    // Execute the prepared statement.
    if (sqlite3_step(stmt) != SQLITE_DONE) {
        std::cerr << "Error stepping insert statement: " << sqlite3_errmsg(db_) << std::endl;
        sqlite3_finalize(stmt);
        return false;
    }

    // Safely deallocate the statement memory to prevent leaks.
    sqlite3_finalize(stmt);

    // Retrieve the auto-generated primary key and update the in-memory session object
    // so the UI can immediately interact with the new record (e.g., for deletion).
    int generated_id = static_cast<int>(sqlite3_last_insert_rowid(db_));
    session.set_id(generated_id);

    return true;
}

bool DatabaseHandler::delete_session(int id) {
    const char *sql = "DELETE FROM sessions WHERE id = ?;";
    sqlite3_stmt *stmt;

    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        std::cerr << "Error preparing delete statement: " << sqlite3_errmsg(db_) << std::endl;
        return false;
    }

    sqlite3_bind_int(stmt, 1, id);

    if (sqlite3_step(stmt) != SQLITE_DONE) {
        std::cerr << "Error deleting session: " << sqlite3_errmsg(db_) << std::endl;
        sqlite3_finalize(stmt);
        return false;
    }
    sqlite3_finalize(stmt);
    return true;
}

std::vector<Session> DatabaseHandler::get_all_sessions() {
    std::vector<Session> sessions;
    std::string sql =
            "SELECT id, group_name, name, duration_work, duration_pause, duration_total, efficiency, timestamp FROM sessions ORDER BY id DESC;";

    sqlite3_stmt *stmt;

    // Compile the SQL query into a bytecode program for faster execution.
    if (sqlite3_prepare_v2(db_, sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
        std::cerr << "Error preparing select statement: " << sqlite3_errmsg(db_) << std::endl;
        return sessions;
    }

    // Process the result set row by row.
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        int index = 0;
        int id = sqlite3_column_int(stmt, index++);
        std::string group_name = SafeColumnText(stmt, index++);
        std::string name = SafeColumnText(stmt, index++);
        int duration_work = sqlite3_column_int(stmt, index++);
        int duration_pause = sqlite3_column_int(stmt, index++);
        int duration_total = sqlite3_column_int(stmt, index++);
        double efficiency = sqlite3_column_double(stmt, index++);
        std::string ts_str = SafeColumnText(stmt, index++);

        // Parse the serialized date string back into structural components for the Timestamp object.
        int year = 0, month = 0, day = 0, hour = 0, minute = 0;
        if (sscanf(ts_str.c_str(), "%d-%d-%d %d:%d", &year, &month, &day, &hour, &minute) == 5) {
            Timestamp ts(year, month, day, hour, minute);
            sessions.emplace_back(id, group_name, name, duration_work, duration_pause, duration_total, efficiency, ts);
        }
    }

    sqlite3_finalize(stmt);
    return sessions;
}

std::vector<std::string> DatabaseHandler::get_unique_groups() {
    std::vector<std::string> groups;
    std::string sql = "SELECT DISTINCT group_name FROM sessions WHERE group_name !='';";
    sqlite3_stmt *stmt;

    if (sqlite3_prepare_v2(db_, sql.c_str(), -1, &stmt, nullptr) == SQLITE_OK) {
        while (sqlite3_step(stmt) == SQLITE_ROW) {
            groups.push_back(SafeColumnText(stmt, 0));
        }
    }
    sqlite3_finalize(stmt);
    return groups;
}

std::string DatabaseHandler::get_last_used_group() {
    // Limit to 1 and order descending to quickly fetch the most recent context.
    std::string sql = "SELECT group_name FROM sessions ORDER BY id DESC LIMIT 1;";
    sqlite3_stmt *stmt;
    std::string last_group = "";

    if (sqlite3_prepare_v2(db_, sql.c_str(), -1, &stmt, nullptr) == SQLITE_OK) {
        if (sqlite3_step(stmt) == SQLITE_ROW) {
            last_group = SafeColumnText(stmt, 0);
        }
    }
    sqlite3_finalize(stmt);
    return last_group;
}

Session DatabaseHandler::get_last_session() {
    std::string sql =
            "SELECT id, group_name, name, duration_work, duration_pause, duration_total, efficiency, timestamp FROM sessions ORDER BY id DESC LIMIT 1;";
    sqlite3_stmt *stmt;

    if (sqlite3_prepare_v2(db_, sql.c_str(), -1, &stmt, nullptr) == SQLITE_OK) {
        if (sqlite3_step(stmt) == SQLITE_ROW) {
            int index = 0;
            int id = sqlite3_column_int(stmt, index++);
            std::string group_name = SafeColumnText(stmt, index++);
            std::string name = SafeColumnText(stmt, index++);
            int duration_work = sqlite3_column_int(stmt, index++);
            int duration_pause = sqlite3_column_int(stmt, index++);
            int duration_total = sqlite3_column_int(stmt, index++);
            double efficiency = sqlite3_column_double(stmt, index++);
            std::string ts_str = SafeColumnText(stmt, index++);

            int year = 0, month = 0, day = 0, hour = 0, minute = 0;
            Timestamp ts;
            if (sscanf(ts_str.c_str(), "%d-%d-%d %d:%d", &year, &month, &day, &hour, &minute) == 5) {
                ts = Timestamp(year, month, day, hour, minute);
            }
            sqlite3_finalize(stmt);

            return Session(id, group_name, name, duration_work, duration_pause, duration_total, efficiency, ts);
        }
    }
    sqlite3_finalize(stmt);
    return Session("", "", 0, 0, Timestamp());
}

std::vector<Session> DatabaseHandler::search_sessions(const std::string &query, bool &has_error) {
    std::vector<Session> sessions;
    std::vector<std::string> bind_params;

    // Delegate WHERE clause generation to QueryParser, extracting dynamic string parameters to prevent SQL injection.
    std::string sql_where = QueryParser::generate_where_clause(query, has_error, bind_params);
    std::string sql =
            "SELECT id, group_name, name, duration_work, duration_pause, duration_total, efficiency, timestamp FROM sessions WHERE "
            + sql_where + " ORDER BY id DESC;";

    sqlite3_stmt *stmt;

    // Compile the SQL query into a bytecode program for safe execution.
    if (sqlite3_prepare_v2(db_, sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
        std::cerr << "Error preparing search statement: " << sqlite3_errmsg(db_) << std::endl;
        return sessions;
    }

    // Bind the extracted string parameters dynamically.
    for (size_t i = 0; i < bind_params.size(); ++i) {
        sqlite3_bind_text(stmt, static_cast<int>(i + 1), bind_params[i].c_str(), -1, SQLITE_TRANSIENT);
    }

    // Process the result set row by row.
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        int index = 0;
        int id = sqlite3_column_int(stmt, index++);
        std::string group_name = SafeColumnText(stmt, index++);
        std::string name = SafeColumnText(stmt, index++);
        int duration_work = sqlite3_column_int(stmt, index++);
        int duration_pause = sqlite3_column_int(stmt, index++);
        int duration_total = sqlite3_column_int(stmt, index++);
        double efficiency = sqlite3_column_double(stmt, index++);
        std::string ts_str = SafeColumnText(stmt, index++);

        // Parse the serialized date string back into structural components for the Timestamp object.
        int year = 0, month = 0, day = 0, hour = 0, minute = 0;
        if (sscanf(ts_str.c_str(), "%d-%d-%d %d:%d", &year, &month, &day, &hour, &minute) == 5) {
            Timestamp ts(year, month, day, hour, minute);
            sessions.emplace_back(id, group_name, name, duration_work, duration_pause, duration_total, efficiency, ts);
        }
    }

    // Safely deallocate the compiled statement.
    sqlite3_finalize(stmt);

    return sessions;
}

void DatabaseHandler::update_top_groups(DatabaseHandler &db, AppContext &ctx) {
    std::unordered_map<std::string, GroupStats> stats_map;
    std::vector<Session> all_sessions = db.get_all_sessions();

    // Aggregate performance metrics grouped by project/group name.
    for (const auto &s: all_sessions) {
        stats_map[s.get_group_name()].name = s.get_group_name();
        stats_map[s.get_group_name()].total_work += s.get_duration_work();
        stats_map[s.get_group_name()].session_count++;
        stats_map[s.get_group_name()].sum_efficiency += s.get_efficiency();
    }

    // Transfer mapped data to a linear container for sorting.
    ctx.top_groups.clear();
    for (const auto &pair: stats_map) {
        ctx.top_groups.push_back(pair.second);
    }

    // Sort descending based on total work time to determine the most active groups.
    std::sort(ctx.top_groups.begin(), ctx.top_groups.end(), [](const GroupStats &a, const GroupStats &b) {
        return a.total_work > b.total_work;
    });

    // Clamp the results to fit the dashboard's Top 3 Groups UI panel constraints.
    if (ctx.top_groups.size() > 3) {
        ctx.top_groups.resize(3);
    }
}

