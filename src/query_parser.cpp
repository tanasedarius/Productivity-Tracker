#include "query_parser.h"
#include <iomanip>
#include <vector>
#include <sstream>
#include <cctype>
#include <ctime>
#include <stdexcept>

int QueryParser::parse_time_to_seconds(const std::string &time_str) {
    int total_seconds = 0, current_val = 0;

    // Iterate through characters to extract values and apply their respective unit multipliers.
    for (char c: time_str) {
        if (std::isdigit(c)) {
            current_val = current_val * 10 + (c - '0');
        } else if (c == 'h') {
            total_seconds += current_val * 3600;
            current_val = 0;
        } else if (c == 'm') {
            total_seconds += current_val * 60;
            current_val = 0;
        } else if (c == 's') {
            total_seconds += current_val;
            current_val = 0;
        }
    }

    // Add any remaining trailing digits treated as seconds by default.
    if (current_val > 0) total_seconds += current_val;
    return total_seconds;
}

void QueryParser::extract_operator(std::string &token, std::string &op) {
    // Default operator is exact match if none is explicitly provided.
    op = "=";

    // Check for two-character relational operators (>=, <=) first to avoid partial matches.
    if (token.size() >= 2 && (token.substr(0, 2) == ">=" || token.substr(0, 2) == "<=")) {
        op = token.substr(0, 2);
        token = token.substr(2); // Remove the operator from the value string
    }
    // Check for single-character relational operators (>, <, =).
    else if (token.size() >= 1 && (token[0] == '>' || token[0] == '<' || token[0] == '=')) {
        op = std::string(1, token[0]);
        token = token.substr(1);
    }
}

std::string QueryParser::parse_condition(const std::string &column, const std::string &value, bool is_time) {
    std::string sql_cond = "(";
    std::stringstream ss(value);
    std::string token;
    bool first = true;

    // Tokenize by '&' to support logical AND conditions within the same filter key (e.g., >10&<50).
    while (std::getline(ss, token, '&')) {
        if (!first) sql_cond += " AND ";
        first = false;

        std::string op;
        extract_operator(token, op);

        if (is_time) {
            // Convert human-readable time (e.g., 1h30m) into raw seconds for accurate integer comparison in the database.
            int secs = parse_time_to_seconds(token);
            sql_cond += column + " " + op + " " + std::to_string(secs);
        } else {
            // Validate that the token is numerical; throws exception if invalid, handled by the caller.
            std::stof(token);

            // Handle floating-point precision issues in SQLite by rounding efficiency comparisons.
            if (column == "efficiency" && op == "=") {
                sql_cond += "ROUND(" + column + ", 1) = " + token;
            } else {
                sql_cond += column + " " + op + " " + token;
            }
        }
    }
    return sql_cond + ")";
}

std::string QueryParser::parse_date_value(const std::string &val) {
    std::time_t t = std::time(nullptr);
    std::tm *time_info = std::localtime(&t);
    bool include_time = false;

    // Process relative time keywords starting with 'last' (e.g., last7d, last2h).
    if (val.find("last") == 0) {
        char unit = val.back();
        std::string amount_str = "";

        // Extract the numerical amount between "last" and the unit character.
        if (val.length() > 5) {
            amount_str = val.substr(4, val.length() - 5);
        }

        int amount = amount_str.empty() ? 1 : std::stoi(amount_str);

        // Adjust the local time structure based on the provided unit.
        if (unit == 'h') {
            time_info->tm_hour -= amount;
            include_time = true;
        } else if (unit == 'M') {
            time_info->tm_min -= amount;
            include_time = true;
        } else if (unit == 'd') time_info->tm_mday -= amount;
        else if (unit == 'm') time_info->tm_mon -= amount;
        else if (unit == 'y') time_info->tm_year -= amount;

        // mktime normalizes the tm struct (e.g., handles negative days correctly by rolling back months).
        std::mktime(time_info);
    }
    // Process absolute date strings typically formatted as DD-MM-YYYY.
    else {
        std::stringstream ss(val);
        std::string part;
        std::vector<int> parts;

        while (std::getline(ss, part, '-')) { parts.push_back(std::stoi(part)); }

        if (parts.size() >= 1) time_info->tm_mday = parts[0];
        if (parts.size() >= 2) time_info->tm_mon = parts[1] - 1; // tm_mon is 0-indexed
        if (parts.size() >= 3) time_info->tm_year = parts[2] - 1900; // tm_year is years since 1900

        std::mktime(time_info);
    }

    // Format the normalized time back into an SQLite compatible string.
    std::stringstream out;
    if (!include_time) {
        out << std::put_time(time_info, "%Y-%m-%d");
    } else {
        out << std::put_time(time_info, "%Y-%m-%d %H:%M");
    }
    return out.str();
}

std::string QueryParser::parse_date_condition(const std::string &value) {
    std::string sql_cond = "(";
    std::stringstream ss(value);
    std::string token;
    bool first = true;

    // Tokenize by '&' to allow complex date range queries (e.g., >01-01-2023&<31-12-2023).
    while (std::getline(ss, token, '&')) {
        if (!first) sql_cond += " AND ";
        first = false;

        std::string op;
        extract_operator(token, op);

        // Relative 'last' queries imply a range from that point forward to the present.
        if (op == "=" && token.find("last") == 0) { op = ">="; }

        std::string parsed_date = parse_date_value(token);

        // Use LIKE for exact day matches to ignore the hour/minute portion of the timestamp.
        if (op == "=") {
            sql_cond += "timestamp LIKE '" + parsed_date + "%'";
        } else {
            sql_cond += "timestamp " + op + " '" + parsed_date + "'";
        }
    }
    return sql_cond + ")";
}

std::string QueryParser::generate_where_clause(const std::string &pql_query, bool &has_error,
                                               std::vector<std::string> &bind_params) {
    // Initialize with a base true condition to simplify appending logical AND clauses.
    std::string sql = "1=1";
    std::stringstream stream(pql_query);
    std::string token;
    has_error = false;

    // Process space-separated key:value pairs mapping to specific SQL columns.
    while (stream >> token) {
        auto pos = token.find(':');
        if (pos == std::string::npos) continue;

        std::string key = token.substr(0, pos);
        std::string val = token.substr(pos + 1);

        if (val.empty()) continue;

        try {
            // Map custom PQL tags to their corresponding database columns.
            // Utilize parameterized inputs (?) for string matching to strictly prevent SQL injection.
            if (key == "group") {
                sql += " AND group_name LIKE ?";
                bind_params.push_back("%" + val + "%");
            } else if (key == "name") {
                sql += " AND name LIKE ?";
                bind_params.push_back("%" + val + "%");
            }
            // Numeric and date parsed conditions are safely generated internally by the parser.
            else if (key == "eff") sql += " AND " + parse_condition("efficiency", val, false);
            else if (key == "awork") sql += " AND " + parse_condition("duration_work", val, true);
            else if (key == "pwork") sql += " AND " + parse_condition("duration_pause", val, true);
            else if (key == "work") sql += " AND " + parse_condition("duration_total", val, true);
            else if (key == "date") sql += " AND " + parse_date_condition(val);
            else has_error = true; // Flag invalid or unrecognized keys in the syntax.
        } catch (const std::exception &) {
            has_error = true;
            continue;
        }
    }
    return sql;
}
