/**
 * @file query_parser.h
 * @brief Parses Productivity Query Language (PQL) strings into SQL WHERE clauses.
 */

#pragma once
#include <string>
#include <vector>

/**
 * @class QueryParser
 * @brief A static utility class responsible for translating user-facing PQL syntax into valid SQLite conditions.
 * 
 * PQL (Productivity Query Language) allows users to filter sessions using a compact 
 * key:value syntax (e.g., "group:study date:last7d work:>2h"). This parser handles 
 * tokenization, operator extraction, and date/time unit conversions.
 */
class QueryParser
{
private:
    /**
     * @brief Converts a human-readable time string into total seconds.
     * 
     * Parses formats like "1h30m", "45m", or "120s" into raw integers, enabling 
     * the database to perform accurate numeric comparisons on duration columns.
     * 
     * @param time_str The time string to parse (e.g., "2h15m").
     * @return The equivalent duration in total seconds.
     */
    static int parse_time_to_seconds(const std::string &time_str);

    /**
     * @brief Extracts relational operators from a search token.
     * 
     * Checks for operators (>=, <=, >, <, =) at the beginning of a token, 
     * separating the operator from the actual value. Defaults to "=" if none is found.
     * 
     * @param token The input string token, which will be modified to remove the operator.
     * @param op Output parameter that will store the extracted operator.
     */
    static void extract_operator(std::string &token, std::string &op);

    /**
     * @brief Generates a SQL condition block for a generic or time-based column.
     * 
     * Handles complex filtering such as logical AND chains (e.g., ">10&<50"). 
     * Automatically applies time-to-seconds conversion if the column represents a duration.
     * 
     * @param column The database column name being queried.
     * @param value The PQL value string containing the constraints.
     * @param is_time Flag indicating if the value should be parsed as a time duration.
     * @return A safely formatted SQL condition string wrapped in parentheses.
     */
    static std::string parse_condition(const std::string &column, const std::string &value, bool is_time);

    /**
     * @brief Parses relative or absolute date strings into SQLite timestamp formats.
     * 
     * Supports relative keywords (e.g., "last7d", "last2h") by calculating backward 
     * from the current system time, as well as absolute dates (e.g., "26-09-2026").
     * 
     * @param val The date string to parse.
     * @return A formatted date string compatible with SQLite's text-based date comparisons.
     */
    static std::string parse_date_value(const std::string &val);

    /**
     * @brief Generates a complete SQL condition for date filtering.
     * 
     * Expands upon `parse_date_value` to support range queries (using '&') and 
     * handles the semantic difference between exact matches (using LIKE) and 
     * relative forward-looking ranges.
     * 
     * @param value The raw PQL date value string.
     * @return A formatted SQL condition string for the timestamp column.
     */
    static std::string parse_date_condition(const std::string &value);

public:
/**
     * @brief Translates a complete PQL query into a valid SQL WHERE clause.
     * 
     * Extracts dynamic string parameters into a separate vector to prevent 
     * SQL injection vulnerabilities when executing LIKE clauses.
     * 
     * @param pql_query The raw search string entered by the user.
     * @param has_error Reference flag set to true if any syntax errors are encountered.
     * @param bind_params Vector populated with extracted string parameters for safe SQLite binding.
     * @return A fully constructed SQL WHERE clause string with placeholders (?).
     */
    static std::string generate_where_clause(const std::string &pql_query, bool &has_error, std::vector<std::string> &bind_params);
};