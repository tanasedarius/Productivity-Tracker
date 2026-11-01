/**
 * @file test_core.cpp
 * @brief Unit tests for the pure-logic core: Timer formatting, Session
 *        derived statistics, and the PQL -> SQL QueryParser.
 *
 * These target deterministic logic only. Wall-clock-dependent behavior
 * (live Timer elapsed time, Timestamp::get_current_timestamp, relative
 * "last" dates) is tested for shape and invariants, not exact values, to
 * keep the suite fast and non-flaky.
 */

#include <gtest/gtest.h>
#include <string>
#include <vector>

#include "timer.h"
#include "session.h"
#include "timestamp.h"
#include "query_parser.h"

// ---------------------------------------------------------------------------
// Timer::get_formatted_duration  (pure, static)
// ---------------------------------------------------------------------------

TEST(TimerFormat, Zero) {
    EXPECT_EQ(Timer::get_formatted_duration(0), "00:00:00");
}

TEST(TimerFormat, SecondsOnly) {
    EXPECT_EQ(Timer::get_formatted_duration(5), "00:00:05");
    EXPECT_EQ(Timer::get_formatted_duration(59), "00:00:59");
}

TEST(TimerFormat, MinutesAndSeconds) {
    EXPECT_EQ(Timer::get_formatted_duration(60), "00:01:00");
    EXPECT_EQ(Timer::get_formatted_duration(125), "00:02:05");
}

TEST(TimerFormat, HoursMinutesSeconds) {
    EXPECT_EQ(Timer::get_formatted_duration(3661), "01:01:01");
    EXPECT_EQ(Timer::get_formatted_duration(3600), "01:00:00");
}

TEST(TimerFormat, LargeValueDoesNotWrapHours) {
    // 100 hours should render as "100:00:00" (hours field is not clamped to 2 digits).
    EXPECT_EQ(Timer::get_formatted_duration(360000), "100:00:00");
}

// ---------------------------------------------------------------------------
// Session  (derived total + efficiency computed in the constructor)
// ---------------------------------------------------------------------------

TEST(SessionStats, TotalIsWorkPlusPause) {
    Session s("Coding", "Parser", 3600, 900, Timestamp());
    EXPECT_EQ(s.get_duration_total(), 4500);
}

TEST(SessionStats, EfficiencyIsWorkOverTotal) {
    // 3600 work / 4500 total = 80%
    Session s("Coding", "Parser", 3600, 900, Timestamp());
    EXPECT_NEAR(s.get_efficiency(), 80.0, 1e-9);
}

TEST(SessionStats, PerfectEfficiencyWhenNoPause) {
    Session s("Study", "Reading", 1200, 0, Timestamp());
    EXPECT_EQ(s.get_duration_total(), 1200);
    EXPECT_NEAR(s.get_efficiency(), 100.0, 1e-9);
}

TEST(SessionStats, ZeroTotalDoesNotDivideByZero) {
    // Guard path: no recorded time must yield 0 efficiency, not NaN/crash.
    Session s("Idle", "Nothing", 0, 0, Timestamp());
    EXPECT_EQ(s.get_duration_total(), 0);
    EXPECT_NEAR(s.get_efficiency(), 0.0, 1e-9);
}

TEST(SessionStats, FieldsRoundTrip) {
    Timestamp ts(2026, 10, 31, 14, 30);
    Session s("Fitness", "Run", 2400, 180, ts);
    EXPECT_EQ(s.get_group_name(), "Fitness");
    EXPECT_EQ(s.get_name(), "Run");
    EXPECT_EQ(s.get_duration_work(), 2400);
    EXPECT_EQ(s.get_duration_pause(), 180);
    EXPECT_EQ(s.get_id(), -1); // unsaved sessions start at -1
}

// ---------------------------------------------------------------------------
// Timestamp  (pure formatting)
// ---------------------------------------------------------------------------

TEST(TimestampFormat, DatePaddedDDMMYYYY) {
    Timestamp ts(2026, 3, 7, 9, 5);
    EXPECT_EQ(ts.get_formatted_date(), "07/03/2026");
}

TEST(TimestampFormat, TimePaddedHHMM) {
    Timestamp ts(2026, 3, 7, 9, 5);
    EXPECT_EQ(ts.get_formatted_time(), "09:05");
}

// ---------------------------------------------------------------------------
// QueryParser::generate_where_clause  (PQL -> parameterized SQL)
//
// Helper: run a query and return the generated WHERE string.
// ---------------------------------------------------------------------------

static std::string Where(const std::string &pql, bool &err,
                         std::vector<std::string> &params) {
    return QueryParser::generate_where_clause(pql, err, params);
}

TEST(QueryParser, EmptyQueryIsAlwaysTrue) {
    bool err = false;
    std::vector<std::string> params;
    std::string sql = Where("", err, params);
    EXPECT_EQ(sql, "1=1");
    EXPECT_FALSE(err);
    EXPECT_TRUE(params.empty());
}

TEST(QueryParser, GroupUsesBoundParameterNotInlineValue) {
    // The security-critical behavior: the user value must be a bind
    // parameter (?), never concatenated into the SQL string.
    bool err = false;
    std::vector<std::string> params;
    std::string sql = Where("group:study", err, params);

    EXPECT_NE(sql.find("group_name LIKE ?"), std::string::npos);
    EXPECT_EQ(sql.find("study"), std::string::npos); // value NOT inlined
    ASSERT_EQ(params.size(), 1u);
    EXPECT_EQ(params[0], "%study%");
    EXPECT_FALSE(err);
}

TEST(QueryParser, InjectionAttemptIsNeutralizedAsData) {
    // A classic injection payload must end up as a bound parameter,
    // leaving the SQL structure untouched.
    bool err = false;
    std::vector<std::string> params;
    std::string malicious = "group:';DROP";
    std::string sql = Where(malicious, err, params);

    EXPECT_NE(sql.find("group_name LIKE ?"), std::string::npos);
    EXPECT_EQ(sql.find("DROP"), std::string::npos); // not in SQL text
    ASSERT_EQ(params.size(), 1u);
    EXPECT_EQ(params[0], "%';DROP%");             // preserved verbatim as data
}

TEST(QueryParser, NameAlsoBound) {
    bool err = false;
    std::vector<std::string> params;
    std::string sql = Where("name:refactor", err, params);
    EXPECT_NE(sql.find("name LIKE ?"), std::string::npos);
    ASSERT_EQ(params.size(), 1u);
    EXPECT_EQ(params[0], "%refactor%");
}

TEST(QueryParser, WorkDurationConvertedToSeconds) {
    // work: maps to duration_total, time converted to seconds.
    // 2h = 7200s.
    bool err = false;
    std::vector<std::string> params;
    std::string sql = Where("work:>2h", err, params);
    EXPECT_NE(sql.find("duration_total > 7200"), std::string::npos);
    EXPECT_FALSE(err);
}

TEST(QueryParser, CompositeDurationConverted) {
    // 1h30m = 5400s on actual-work column (awork -> duration_work).
    bool err = false;
    std::vector<std::string> params;
    std::string sql = Where("awork:>=1h30m", err, params);
    EXPECT_NE(sql.find("duration_work >= 5400"), std::string::npos);
    EXPECT_FALSE(err);
}

TEST(QueryParser, RangeWithAmpersandBecomesAnd) {
    // ">10m&<2h" -> both bounds, joined by AND, in seconds (600 / 7200).
    bool err = false;
    std::vector<std::string> params;
    std::string sql = Where("work:>10m&<2h", err, params);
    EXPECT_NE(sql.find("duration_total > 600"), std::string::npos);
    EXPECT_NE(sql.find("duration_total < 7200"), std::string::npos);
    EXPECT_NE(sql.find(" AND "), std::string::npos);
    EXPECT_FALSE(err);
}

TEST(QueryParser, EfficiencyExactUsesRounding) {
    // eff:=85 with exact operator should wrap the column in ROUND(...).
    bool err = false;
    std::vector<std::string> params;
    std::string sql = Where("eff:=85", err, params);
    EXPECT_NE(sql.find("ROUND(efficiency, 1) = 85"), std::string::npos);
    EXPECT_FALSE(err);
}

TEST(QueryParser, UnknownKeyFlagsError) {
    bool err = false;
    std::vector<std::string> params;
    Where("bogus:123", err, params);
    EXPECT_TRUE(err);
}

TEST(QueryParser, NonNumericDurationFlagsError) {
    // stof() on a non-numeric efficiency value throws -> caught -> error flag.
    bool err = false;
    std::vector<std::string> params;
    Where("eff:abc", err, params);
    EXPECT_TRUE(err);
}

TEST(QueryParser, TokenWithoutColonIgnored) {
    bool err = false;
    std::vector<std::string> params;
    std::string sql = Where("justtext", err, params);
    EXPECT_EQ(sql, "1=1");
    EXPECT_FALSE(err);
}

TEST(QueryParser, MultipleFiltersCombined) {
    bool err = false;
    std::vector<std::string> params;
    std::string sql = Where("group:study work:>1h", err, params);
    EXPECT_NE(sql.find("group_name LIKE ?"), std::string::npos);
    EXPECT_NE(sql.find("duration_total > 3600"), std::string::npos);
    ASSERT_EQ(params.size(), 1u);
    EXPECT_EQ(params[0], "%study%");
    EXPECT_FALSE(err);
}

TEST(QueryParser, ExactDateUsesLikePrefix) {
    // Absolute date with implicit "=" should use LIKE 'YYYY-MM-DD%'
    // so the time-of-day portion is ignored.
    bool err = false;
    std::vector<std::string> params;
    std::string sql = Where("date:31-10-2026", err, params);
    EXPECT_NE(sql.find("timestamp LIKE '2026-10-31%'"), std::string::npos);
    EXPECT_FALSE(err);
}
