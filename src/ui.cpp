/**
 * @file ui.cpp
 * @brief Implementation of the Dear ImGui rendering functions for the Productivity Tracker.
 */

#include "ui.h"
#include "imgui.h"
#include "imgui_internal.h"
#include "database_handler.h"
#include "timestamp.h"
#include <cstring>
#include <iostream>
#include <set>
#include <map>
#include <ctime>
#include <fstream>
#include <GLFW/glfw3.h>

void RenderDebugMenu(AppContext &ctx, DatabaseHandler &db) {
    static bool show_debug_window = false;

    // Trigger button located within the sidebar component.
    if (ImGui::IsKeyPressed(ImGuiKey_0) && !ImGui::GetIO().WantCaptureKeyboard) {
        show_debug_window = !show_debug_window;
    }

    // Render floating developer tool for direct database mutation.
    if (show_debug_window) {
        ImGui::Begin(" Developer Debug: Inject Session", &show_debug_window);

        // Default test payloads
        static char d_group[128] = "TEST_GROUP";
        static char d_name[128] = "Debug Task";
        static int d_work_mins = 120; // Default 2 hours
        static int d_pause_mins = 15;

        static int d_year = 2026;
        static int d_month = 9;
        static int d_day = 17;
        static int d_hour = 14;
        static int d_min = 30;

        ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "DANGER ZONE: Direct DB Injection");
        ImGui::Separator();

        ImGui::InputText("Group Name", d_group, sizeof(d_group));
        ImGui::InputText("Activity Name", d_name, sizeof(d_name));

        ImGui::Spacing();
        ImGui::InputInt("Work (minutes)", &d_work_mins);
        ImGui::InputInt("Pause (minutes)", &d_pause_mins);

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Text("Custom Timestamp:");
        ImGui::InputInt("Year", &d_year);
        ImGui::InputInt("Month", &d_month);
        ImGui::InputInt("Day", &d_day);
        ImGui::InputInt("Hour", &d_hour);
        ImGui::InputInt("Minute", &d_min);

        ImGui::Spacing();
        ImGui::Separator();

        // Perform injection sequence and flush state caches to immediately reflect changes.
        if (ImGui::Button("INJECT INTO DATABASE", ImVec2(-1, 50))) {
            int work_sec = d_work_mins * 60;
            int pause_sec = d_pause_mins * 60;

            Timestamp custom_ts(d_year, d_month, d_day, d_hour, d_min);
            Session custom_session(std::string(d_group), std::string(d_name), work_sec, pause_sec, custom_ts);

            if (db.save_session(custom_session)) {
                ctx.status_message = "DEBUG: Session injected successfully!";

                // Force UI cache invalidation to trigger rendering updates on the dashboard.
                ctx.last_session = db.get_last_session();
                ctx.groups_loaded = false;

                if (ctx.show_history_window) {
                    ctx.history_sessions = db.get_all_sessions();
                }

                db.update_top_groups(db, ctx);
            } else {
                ctx.status_message = "DEBUG: Injection failed!";
            }
        }
        ImGui::End();
    }
}

// =====================================================================
// MODULAR WIDGET PANELS
// ====================================================================

void RenderTimeAwarenessPanel() {
    float app_width = ImGui::GetWindowWidth();
    float h = ImGui::GetWindowHeight() * 0.25f;

    ImGui::BeginChild("TimeAwarenessPanel", ImVec2(0, h), true);

    // Apply dynamic font scaling based on viewport width to maintain legibility.
    float text_scale = app_width / 1000.0f;
    if (text_scale < 0.8f)
        text_scale = 0.8f;
    if (text_scale > 2.0f)
        text_scale = 2.0f;

    ImGui::SetWindowFontScale(text_scale);

    ImGui::Spacing();
    ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 10.0f);

    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.8f, 0.4f, 1.0f, 1.0f));
    ImGui::Text(" TIME SLIPPING AWAY");
    ImGui::PopStyleColor();
    ImGui::Separator();

    for (int i = 0; i < 4; i++)
        ImGui::Spacing(); // Padding block

    // Fetch system time for absolute chronometric calculations.
    std::time_t t = std::time(nullptr);
    std::tm *now = std::localtime(&t);

    // --- TEMPORAL CALCULATIONS ---
    float seconds_today = now->tm_hour * 3600.0f + now->tm_min * 60.0f + now->tm_sec;

    // Daily metric
    float day_pct = seconds_today / 86400.0f;
    int hours_left = 23 - now->tm_hour;
    int mins_left = 59 - now->tm_min;

    // Weekly metric (normalized to Monday=0, Sunday=6)
    int wday = (now->tm_wday == 0) ? 6 : now->tm_wday - 1;
    float week_pct = (wday * 86400.0f + seconds_today) / (7.0f * 86400.0f);
    int seconds_left_today = 86400 - seconds_today;
    int seconds_in_future_days_week = (6 - wday) * 86400;
    int total_seconds_left_week = seconds_in_future_days_week + seconds_left_today;
    int days_left_week = total_seconds_left_week / 86400;
    int hours_left_week = (total_seconds_left_week % 86400) / 3600;

    // Monthly metric
    int year = now->tm_year + 1900;
    bool is_leap = (year % 4 == 0 && year % 100 != 0) || (year % 400 == 0);
    int days_in_month[] = {31, is_leap ? 29 : 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    int dim = days_in_month[now->tm_mon];
    float month_pct = ((now->tm_mday - 1) * 86400.0f + seconds_today) / (dim * 86400.0f);
    int days_left_month = dim - now->tm_mday;

    // Yearly metric
    int diy = is_leap ? 366 : 365;
    float year_pct = ((now->tm_yday) * 86400.0f + seconds_today) / (diy * 86400.0f);
    int days_left_year = diy - now->tm_yday - 1;

    // Vertical centering offset calculation for the progress bar cluster.
    float bar_height = 18.0f * text_scale;
    float single_spacing = ImGui::GetTextLineHeight();
    float content_height = 4.0f * (bar_height + single_spacing);
    float available_space = ImGui::GetWindowHeight() - ImGui::GetCursorPosY() - ImGui::GetStyle().WindowPadding.y;

    if (available_space > content_height) {
        float offset_y = (available_space - content_height) / 2.0f;
        ImGui::SetCursorPosY(ImGui::GetCursorPosY() + offset_y);
    }

    ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 15.0f);
    ImGui::BeginGroup();

    // Lambda for rendering custom progress bars with integrated responsive typography.
    auto draw_bar = [&](const char *label, float pct, const char *time_val, ImVec4 color) {
        ImGui::TextDisabled("%s", label);
        ImGui::SameLine(130.0f * text_scale);
        ImGui::PushStyleColor(ImGuiCol_PlotHistogram, color);

        float b_height = 18.0f * text_scale;
        ImGui::ProgressBar(pct, ImVec2(-15, b_height), "");
        ImGui::PopStyleColor();

        // Retrieve global coordinates of the rendered progress bar geometry.
        ImVec2 bar_min = ImGui::GetItemRectMin();
        ImVec2 bar_max = ImGui::GetItemRectMax();
        float bar_width = bar_max.x - bar_min.x;

        char left_full[32], right_full[32];
        char left_short[32], right_short[32];
        char sep_full[32] = " | ", sep_short[32] = "|";

        snprintf(left_full, sizeof(left_full), "%.1f%% passed", pct * 100.0f);
        snprintf(right_full, sizeof(right_full), "%s left", time_val);

        snprintf(left_short, sizeof(left_short), "%.1f%%", pct * 100.0f);
        snprintf(right_short, sizeof(right_short), "%s", time_val);

        // Fallback to concise text formatting if the bar width is too narrow (Responsive UI logic).
        char test_full[128];
        snprintf(test_full, sizeof(test_full), "%s | %s", left_full, right_full);
        float full_width = ImGui::CalcTextSize(test_full).x;

        const char *final_left = left_full;
        const char *final_right = right_full;
        const char *final_sep = sep_full;

        if (full_width > bar_width - 30.0f) {
            final_left = left_short;
            final_right = right_short;
            final_sep = sep_short;
        }

        // Apply a clipping mask to constrain text within the bar perimeter.
        ImGui::PushClipRect(bar_min, bar_max, true);

        ImDrawList *draw_list = ImGui::GetWindowDrawList();
        ImU32 text_color = ImGui::GetColorU32(ImGuiCol_Text);
        float text_y = bar_min.y + (bar_max.y - bar_min.y - ImGui::CalcTextSize("A").y) * 0.5f;

        float center_x = bar_min.x + bar_width * 0.5f;
        float sep_width = ImGui::CalcTextSize(final_sep).x;

        // Overlay text elements directly onto the progress bar surface using absolute coordinates.
        ImVec2 sep_pos(center_x - sep_width * 0.5f, text_y);
        draw_list->AddText(sep_pos, text_color, final_sep);

        float left_width = ImGui::CalcTextSize(final_left).x;
        ImVec2 left_pos(sep_pos.x - left_width, text_y);
        draw_list->AddText(left_pos, text_color, final_left);

        ImVec2 right_pos(sep_pos.x + sep_width, text_y);
        draw_list->AddText(right_pos, text_color, final_right);

        ImGui::PopClipRect();
        ImGui::Spacing();
    };

    char buf[64];
    snprintf(buf, sizeof(buf), "%dh %dm", hours_left, mins_left);
    draw_bar("Today:  ", day_pct, buf, ImVec4(0.15f, 0.40f, 0.50f, 1.0f));

    snprintf(buf, sizeof(buf), "%dd %dh", days_left_week, hours_left_week);
    draw_bar("This Week:  ", week_pct, buf, ImVec4(0.20f, 0.50f, 0.25f, 1.0f));

    snprintf(buf, sizeof(buf), "%dd", days_left_month);
    draw_bar("This Month:  ", month_pct, buf, ImVec4(0.65f, 0.35f, 0.15f, 1.0f));

    snprintf(buf, sizeof(buf), "%dd", days_left_year);
    draw_bar("This Year:  ", year_pct, buf, ImVec4(0.60f, 0.20f, 0.25f, 1.0f));

    ImGui::EndGroup();
    ImGui::EndChild();
}

struct ToDoItem {
    bool done;
    std::string text;
};

void RenderToDoPanel(AppContext &ctx) {
    float app_width = ImGui::GetWindowWidth();

    float w = ImGui::GetWindowWidth() * 0.30f;
    float h = ImGui::GetWindowHeight() * 0.25f;
    ImGui::BeginChild("ToDoPanel", ImVec2(w, h), true);

    float text_scale = app_width / 1000.0f;
    if (text_scale < 0.8f)
        text_scale = 0.8f;
    if (text_scale > 2.0f)
        text_scale = 2.0f;

    ImGui::SetWindowFontScale(text_scale);

    // Static variables for state persistence across render frames.
    static std::vector<ToDoItem> todos;
    static bool is_loaded = false;

    auto save_todos = [&]() {
        std::ofstream out("data/todos.txt");
        for (const auto &t: todos) {
            out << (t.done ? "1" : "0") << "|" << t.text << "\n";
        }
        out.close();
    };

    if (!is_loaded) {
        std::ifstream in("data/todos.txt");
        std::string line;
        while (std::getline(in, line)) {
            if (line.length() >= 2 && line[1] == '|') {
                todos.push_back({line[0] == '1', line.substr(2)});
            }
        }
        in.close();
        is_loaded = true;
    }

    ImGui::Spacing();
    ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 10.0f);
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.4f, 0.8f, 1.0f));
    ImGui::Text(" TO-DO LIST");
    ImGui::PopStyleColor();
    ImGui::Separator();

    ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 10.0f);
    static char new_task[128] = "";

    float button_width = 50.0f * text_scale;
    float input_margin = button_width + 25.0f;

    ImGui::PushItemWidth(ImGui::GetWindowWidth() - input_margin);
    bool enter_pressed = ImGui::InputText("##NewTask", new_task, sizeof(new_task),
                                          ImGuiInputTextFlags_EnterReturnsTrue);
    ImGui::PopItemWidth();

    ImGui::SameLine();
    // Commit new task on UI trigger (Enter key or Add button press).
    if ((ImGui::Button("Add", ImVec2(button_width, 0)) || enter_pressed) && strlen(new_task) > 0) {
        todos.push_back({false, std::string(new_task)});
        new_task[0] = '\0';
        save_todos();
        ctx.add_log("TODO_EVENT: New task added to the workspace.");
    }

    ImGui::Separator();

    // Secondary scrolling region for the task elements.
    ImGui::BeginChild("TaskList", ImVec2(0, 0), false);
    ImGui::SetWindowFontScale(text_scale);

    bool list_changed = false;
    int to_delete = -1;

    for (size_t i = 0; i < todos.size(); ++i) {
        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 10.0f);

        if (ImGui::Checkbox(("##chk" + std::to_string(i)).c_str(), &todos[i].done)) {
            list_changed = true;
            if (todos[i].done)
                ctx.add_log("TODO_EVENT: Task marked as completed.");
            else
                ctx.add_log("TODO_EVENT: Task status reverted to uncompleted.");
        }

        ImGui::SameLine();

        ImGui::PushTextWrapPos(ImGui::GetWindowWidth() - (ImGui::GetWindowWidth() * 0.15f));

        // Dim the text representation if the task is flagged as completed.
        if (todos[i].done) {
            ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetStyleColorVec4(ImGuiCol_TextDisabled));
            ImGui::TextWrapped("%s", todos[i].text.c_str());
            ImGui::PopStyleColor();
        } else {
            ImGui::TextWrapped("%s", todos[i].text.c_str());
        }
        ImGui::PushTextWrapPos();

        // Invisible delete button styled for a minimalist look.
        ImGui::SameLine(ImGui::GetWindowWidth() - 30.0f);
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.0f, 0.0f, 0.0f, 0.0f));
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.8f, 0.2f, 0.2f, 1.0f));
        if (ImGui::Button(("X##del" + std::to_string(i)).c_str())) {
            to_delete = i;
            ctx.add_log("TODO_EVENT: Task deleted.");
        }
        ImGui::PopStyleColor(2);
    }

    // Safely mutate the container outside the iteration loop.
    if (to_delete != -1) {
        todos.erase(todos.begin() + to_delete);
        list_changed = true;
    }

    if (list_changed) {
        save_todos();
    }

    ImGui::EndChild();
    ImGui::EndChild();
}

struct DayStats {
    int session_count = 0;
    long long total_work_time = 0;
    std::set<std::string> active_groups;
};

void RenderHeatmapPanel(AppContext &ctx, DatabaseHandler &db) {
    float app_width = ImGui::GetWindowWidth();

    float w = ImGui::GetWindowWidth() * 0.50f;
    float h = ImGui::GetWindowHeight() * 0.25f;
    ImGui::BeginChild("HeatmapPanel", ImVec2(w, h), true);

    float text_scale = app_width / 1000.0f;
    if (text_scale < 0.8f)
        text_scale = 0.8f;
    if (text_scale > 2.0f)
        text_scale = 2.0f;

    ImGui::SetWindowFontScale(text_scale);

    ImGui::Spacing();
    ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 10.0f);
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.4f, 0.0f, 1.0f));
    ImGui::Text(" GLOBAL ACTIVITY HEATMAP (Last 90 Days)");
    ImGui::PopStyleColor();
    ImGui::Separator();

    static int last_processed_id = -1;
    static std::map<std::string, DayStats> heatmap_data;

    // Cache invalidation mechanism leveraging PQL database querying.
    if (last_processed_id != ctx.last_session.get_id()) {
        heatmap_data.clear();

        bool dummy_err = false;
        std::vector<Session> recent_sessions = db.search_sessions("date:>=last90d", dummy_err);

        for (const auto &s: recent_sessions) {
            std::string date_str = s.get_timestamp().get_formatted_date();
            heatmap_data[date_str].session_count++;
            heatmap_data[date_str].total_work_time += s.get_duration_work();
            heatmap_data[date_str].active_groups.insert(s.get_group_name());
        }
        last_processed_id = ctx.last_session.get_id();
    }

    ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 10.0f);
    ImGui::BeginGroup();

    const int DAYS = 90;
    const int ROWS = 5;
    const int COLS = DAYS / ROWS;

    float square_size = 18.0f * text_scale;
    float padding = 7.0f * text_scale;

    float grid_width = COLS * (square_size + padding) - padding;
    float grid_height = ROWS * (square_size + padding) - padding;

    float avail_width = ImGui::GetWindowWidth();
    float avail_height = ImGui::GetContentRegionAvail().y;

    float offset_x = (avail_width - grid_width) * 0.5f;
    float offset_y = (avail_height - grid_height) * 0.45f;

    ImGui::SetCursorPosX(offset_x);
    ImGui::SetCursorPosY(ImGui::GetCursorPosY() + offset_y);

    ImDrawList *draw_list = ImGui::GetWindowDrawList();
    ImVec2 start_pos = ImGui::GetCursorScreenPos();

    // Lock reference time to noon to avoid daylight saving time shift distortions.
    std::time_t t = std::time(nullptr);
    std::tm *now = std::localtime(&t);
    now->tm_hour = 12;
    t = std::mktime(now);

    // Iteratively render the heatmap geometry and construct interactive tooltips.
    for (int i = 0; i < DAYS; ++i) {
        std::time_t day_t = t - (DAYS - 1 - i) * 86400;
        std::tm *time_info = std::localtime(&day_t);

        char date_key[16];
        snprintf(date_key, sizeof(date_key), "%02d/%02d/%04d", time_info->tm_mday, time_info->tm_mon + 1,
                 time_info->tm_year + 1900);

        std::string date_str(date_key);
        DayStats stats = heatmap_data[date_str];

        // Map aggregated work time metric to rendering color intensity.
        ImVec4 color;
        if (stats.total_work_time == 0)
            color = ImVec4(0.12f, 0.12f, 0.12f, 1.0f);
        else if (stats.total_work_time < 3600)
            color = ImVec4(0.0f, 0.3f, 0.1f, 1.0f);
        else if (stats.total_work_time < 10800)
            color = ImVec4(0.0f, 0.5f, 0.15f, 1.0f);
        else if (stats.total_work_time < 18000)
            color = ImVec4(0.1f, 0.7f, 0.2f, 1.0f);
        else
            color = ImVec4(0.2f, 0.9f, 0.3f, 1.0f);

        // Evaluate grid location geometry.
        int col = i / ROWS;
        int row = i % ROWS;
        ImVec2 pos(start_pos.x + col * (square_size + padding), start_pos.y + row * (square_size + padding));

        draw_list->AddRectFilled(pos, ImVec2(pos.x + square_size, pos.y + square_size),
                                 ImGui::ColorConvertFloat4ToU32(color), 2.0f * text_scale);

        // Map UI bounding boxes over drawn vectors for hover detection.
        ImGui::SetCursorScreenPos(pos);
        bool is_clicked = ImGui::InvisibleButton(("##heat" + std::to_string(i)).c_str(),
                                                 ImVec2(square_size, square_size));

        // Inject date-specific PQL query on click event.
        if (is_clicked) {
            ctx.show_history_window = true;
            char pql_cmd[64];

            snprintf(pql_cmd, sizeof(pql_cmd), "date:%02d-%02d-%04d", time_info->tm_mday, time_info->tm_mon + 1,
                     time_info->tm_year + 1900);
            strncpy(ctx.search_buffer, pql_cmd, sizeof(ctx.search_buffer) - 1);
            ctx.search_buffer[sizeof(ctx.search_buffer) - 1] = '\0';

            ctx.filtered_sessions = db.search_sessions(ctx.search_buffer, ctx.search_error);
            ctx.add_log("SYS_CALL: ctx.show_history_window = true; History opened!");
        }

        if (ImGui::IsItemHovered()) {
            ImGui::BeginTooltip();
            ImGui::SetWindowFontScale(text_scale);

            ImGui::Spacing();
            ImGui::TextColored(ImVec4(1.0f, 1.0f, 1.0f, 1.0f), " %s", date_str.c_str());
            ImGui::Separator();

            if (stats.session_count > 0) {
                ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 10.0f);

                ImGui::TextColored(ImVec4(0.0f, 1.0f, 1.0f, 1.0f), "Work Time: %s",
                                   Timer::get_formatted_duration(stats.total_work_time).c_str());
                ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 10.0f);

                ImGui::Text("Sessions: %d", stats.session_count);
                ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 10.0f);

                std::string groups_str = "Groups: ";
                for (auto const &g: stats.active_groups) {
                    groups_str += "[" + g + "] ";
                }
                ImGui::TextColored(ImVec4(1.0f, 0.84f, 0.0f, 1.0f), "%s", groups_str.c_str());
                ImGui::Spacing();
                ImGui::Spacing();
            } else {
                ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 10.0f);
                ImGui::TextDisabled("No work recorded on this day.");
                ImGui::Spacing();
                ImGui::Spacing();
            }
            ImGui::EndTooltip();
        }
    }

    ImGui::EndGroup();
    ImGui::EndChild();
}

void RenderNotepadPanel(AppContext &ctx) {
    float app_width = ImGui::GetWindowWidth();
    float h = ImGui::GetWindowHeight() * 0.25f;

    ImGui::BeginChild("NotepadPanel", ImVec2(0, h), true);

    float text_scale = app_width / 1000.0f;
    if (text_scale < 0.8f)
        text_scale = 0.8f;
    if (text_scale > 2.0f)
        text_scale = 2.0f;

    ImGui::SetWindowFontScale(text_scale);

    static bool is_loaded = false;
    static bool file_found = false;

    if (!is_loaded) {
        std::ifstream in_file("data/quick_notes.txt");
        if (in_file.is_open()) {
            std::string content((std::istreambuf_iterator<char>(in_file)), std::istreambuf_iterator<char>());
            strncpy(ctx.notepad_buffer, content.c_str(), sizeof(ctx.notepad_buffer) - 1);
            ctx.notepad_buffer[sizeof(ctx.notepad_buffer) - 1] = '\0';

            in_file.close();
            file_found = true;
        }
        is_loaded = true;
    }

    ImGui::Spacing();
    ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 10.0f);

    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.5f, 1.0f, 0.5f, 1.0f));
    ImGui::Text("QUICK NOTEPAD");
    ImGui::PopStyleColor();

    if (!file_found) {
        ImGui::SameLine();
        ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 0.8f), " (New File)");
    }

    ImGui::Separator();
    ImGui::Spacing();
    ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 10.0f);

    ImGuiInputTextFlags flags = ImGuiInputTextFlags_AllowTabInput | ImGuiInputTextFlags_WordWrap;

    // Apply scaling directly to font system for the multiline InputText widget support.
    ImGuiIO &io = ImGui::GetIO();
    float old_global_scale = io.FontGlobalScale;
    io.FontGlobalScale = old_global_scale * text_scale;

    // Persist edits natively upon user modifications.
    if (ImGui::InputTextMultiline("##Notepad", ctx.notepad_buffer, IM_ARRAYSIZE(ctx.notepad_buffer), ImVec2(-10, -10),
                                  flags)) {
        std::ofstream out_file("data/quick_notes.txt");
        if (out_file.is_open()) {
            out_file << ctx.notepad_buffer;
            out_file.close();
            file_found = true;

            ctx.add_log("I/O_EVENT: Buffer flushed to quick_notes.txt");
            if (strcmp((const char *) ctx.notepad_buffer, "i am sad") == 0)
                ctx.add_log("HUMAN_SPIRIT: You can do it man!! I believe in YOU!!");
        }
    }
    io.FontGlobalScale = old_global_scale;
    ImGui::EndChild();
}

void RenderGroupStatsPanel(AppContext &ctx, DatabaseHandler &db) {
    float app_width = ImGui::GetWindowWidth();

    float w = ImGui::GetWindowWidth() * 0.35f;
    float h = ImGui::GetWindowHeight() * 0.25f;
    ImGui::BeginChild("GroupStatsPanel", ImVec2(w, h), true);

    float text_scale = app_width / 1000.0f;
    if (text_scale < 0.8f)
        text_scale = 0.8f;
    if (text_scale > 2.0f)
        text_scale = 2.0f;

    ImGui::SetWindowFontScale(text_scale);

    // Gate execution based on the presence of an actively initialized context.
    if (ctx.last_session.get_id() != -1) {
        std::string current_group = ctx.last_session.get_group_name();

        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.0f, 0.8f, 1.0f, 1.0f));
        ImGui::Spacing();
        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 10.0f);
        ImGui::Text("GROUP STATS: [%s]", current_group.c_str());
        ImGui::SetCursorPosX(ImGui::GetCursorPosX() - 10.0f);

        ImGui::PopStyleColor();
        ImGui::Separator();

        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 20.0f);
        ImGui::BeginGroup();
        ImGui::Spacing();

        static int last_processed_id = -1;
        static std::vector<Session> group_sessions;

        if (last_processed_id != ctx.last_session.get_id()) {
            ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 20.0f);
            bool dummy_error = false;
            group_sessions = db.search_sessions("group:" + current_group, dummy_error);
            last_processed_id = ctx.last_session.get_id();
        }

        if (!group_sessions.empty()) {
            float line_height = ImGui::GetTextLineHeightWithSpacing();
            float spacing_height = ImGui::GetTextLineHeight();
            float content_height = (6.0f * line_height) + (4.0f * spacing_height) + 30.0f;
            float available_space = ImGui::GetWindowHeight() - ImGui::GetCursorPosY() - ImGui::GetStyle().WindowPadding.
                                    y;

            if (available_space > content_height) {
                float offset_y = (available_space - content_height) / 2.0f;
                ImGui::SetCursorPosY(ImGui::GetCursorPosY() + offset_y);
            }

            int count = group_sessions.size();
            long long total_work = 0, total_time = 0, total_pause = 0;
            double total_eff = 0.0;

            int count_morn = 0; // 06:00 - 13:59
            int count_aft = 0; // 14:00 - 21:59
            int count_night = 0; // 22:00 - 05:59

            for (const auto &s: group_sessions) {
                total_work += s.get_duration_work();
                total_pause += s.get_duration_pause();
                total_time += s.get_duration_total();
                total_eff += s.get_efficiency();

                int h_timestamp = s.get_timestamp().get_hour();
                if (h_timestamp >= 6 && h_timestamp < 14)
                    count_morn++;
                else if (h_timestamp >= 14 && h_timestamp < 22)
                    count_aft++;
                else
                    count_night++;
            }

            long long avg_work = total_work / count;
            long long avg_pause = total_pause / count;
            long long avg_time = total_time / count;
            double avg_eff = total_eff / count;

            bool compact = app_width < 600.0f;

            if (compact)
                ImGui::Text("%d", count);
            else
                ImGui::Text("Total Sessions: %d", count);

            ImGui::Spacing();

            auto print_stat = [&](const char *label, long long total, long long avg) {
                if (compact) {
                    ImGui::TextColored(ImVec4(1.0f, 1.0f, 1.0f, 1.0f), "%s",
                                       Timer::get_formatted_duration(total).c_str());
                    ImGui::SameLine(90 * text_scale);
                    ImGui::TextDisabled("%s", Timer::get_formatted_duration(avg).c_str());
                } else {
                    ImGui::Text("%s", label);
                    ImGui::SameLine(80 * text_scale);
                    ImGui::TextColored(ImVec4(1.0f, 1.0f, 1.0f, 1.0f), "%s",
                                       Timer::get_formatted_duration(total).c_str());
                    ImGui::SameLine(180 * text_scale);
                    ImGui::TextDisabled("Avg: %s", Timer::get_formatted_duration(avg).c_str());
                }
            };

            print_stat("Active:", total_work, avg_work);
            print_stat("Pause:", total_pause, avg_pause);
            print_stat("Total:", total_time, avg_time);

            ImGui::Spacing();

            ImVec4 eff_color = (avg_eff > 50.0) ? ImVec4(0.2f, 1.0f, 0.2f, 1.0f) : ImVec4(1.0f, 0.5f, 0.0f, 1.0f);

            if (compact) {
                ImGui::TextColored(eff_color, "  %.1f%%", avg_eff);
            } else {
                ImGui::Text("Avg Eff:");
                ImGui::SameLine(80 * text_scale);
                ImGui::TextColored(eff_color, "  %.1f%%", avg_eff);
            }

            ImGui::Spacing();

            if (compact) {
                ImGui::TextColored(ImVec4(1.0f, 0.9f, 0.2f, 1.0f), "%d", count_morn);
                ImGui::SameLine(40 * text_scale);
                ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.0f, 1.0f), "%d", count_aft);
                ImGui::SameLine(80 * text_scale);
                ImGui::TextColored(ImVec4(0.4f, 0.6f, 1.0f, 1.0f), "%d", count_night);
            } else {
                ImGui::TextColored(ImVec4(1.0f, 0.9f, 0.2f, 1.0f), "Morn: %d", count_morn);
                ImGui::SameLine(120 * text_scale);

                ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.0f, 1.0f), "Aft: %d", count_aft);
                ImGui::SameLine(240 * text_scale);

                ImGui::TextColored(ImVec4(0.4f, 0.6f, 1.0f, 1.0f), "Night: %d", count_night);
            }

            ImGui::Spacing();
            ImGui::Spacing();
            ImGui::SetCursorPosX(ImGui::GetCursorPosX() - 20.0f);
            ImGui::Separator();

            if (ImGui::Button(" View in History", ImVec2(ImGui::GetWindowWidth() - 40.0f, 30))) {
                ctx.show_history_window = true;
                ctx.add_log("SYS_CALL: ctx.show_history_window = true; History opened!");
                std::string filter_cmd = "group:" + current_group;
                strncpy(ctx.search_buffer, filter_cmd.c_str(), sizeof(ctx.search_buffer) - 1);
                ctx.search_buffer[sizeof(ctx.search_buffer) - 1] = '\0';

                ctx.filtered_sessions = db.search_sessions(ctx.search_buffer, ctx.search_error);
            }
        }
        ImGui::EndGroup();
    } else {
        ImGui::Spacing();
        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 10.0f);
        ImGui::TextDisabled("No group context available.");
    }
    ImGui::EndChild();
}

void RenderSidebar(AppContext &ctx, DatabaseHandler &db) {
    float app_width = ImGui::GetWindowWidth();

    float w = ImGui::GetWindowWidth() * 0.12f;
    ImGui::BeginChild("Sidebar", ImVec2(w, 0), true);

    float text_scale = app_width / 1700.0f;
    if (text_scale < 0.8f)
        text_scale = 0.8f;
    if (text_scale > 2.0f)
        text_scale = 2.0f;

    bool compact = app_width > 1000.0f;
    if (!compact)
        text_scale *= 1.5f;

    ImGui::SetWindowFontScale(text_scale);

    ImGui::Spacing();
    if (!compact)
        ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "COMMAND");
    else
        ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "COMMAND CENTER");

    ImGui::Spacing();
    ImGui::Spacing();

    // Toggle logic for the main dashboard interface overlay.
    if (compact) {
        if (ImGui::Button(ctx.show_dashbord ? "[T] Hide Tracker" : "[T] Open Tracker", ImVec2(-1, 50)) || (
                ImGui::IsKeyPressed(ImGuiKey_T) && !ImGui::GetIO().WantCaptureKeyboard)) {
            ctx.show_dashbord = !ctx.show_dashbord;
            if (ctx.show_dashbord)
                ctx.add_log("SYS_CALL: ctx.show_dashboard = true; Dashboard opened!");
            if (!ctx.show_dashbord)
                ctx.add_log("SYS_CALL: ctx.show_dashboard = false; Dashboard closed!");
        }
    } else {
        if (ImGui::Button("[T]", ImVec2(-1, 50)) || ImGui::IsKeyPressed(ImGuiKey_T)) {
            ctx.show_dashbord = !ctx.show_dashbord;
            if (ctx.show_dashbord)
                ctx.add_log("SYS_CALL: ctx.show_dashboard = true; Dashboard opened!");
            if (!ctx.show_dashbord)
                ctx.add_log("SYS_CALL: ctx.show_dashboard = false; Dashboard closed!");
        }
    }

    if (compact) {
        if (ImGui::Button(ctx.show_history_window ? "[H] Hide History" : "[H] Open History", ImVec2(-1, 50)) || (
                ImGui::IsKeyPressed(ImGuiKey_H) && !ImGui::GetIO().WantCaptureKeyboard && !ctx.open_delete_popup)) {
            if (!ctx.show_history_window) {
                ctx.history_sessions = db.get_all_sessions();
                ctx.add_log("SYS_CALL: ctx.show_history_window = false; History closed!");
            } else {
                ctx.history_sessions.clear();
                ctx.add_log("SYS_CALL: ctx.show_history_window = true; History opened!");
            }
            ctx.show_history_window = !ctx.show_history_window;
        }
    } else {
        if (ImGui::Button("[H]", ImVec2(-1, 50)) || (
                ImGui::IsKeyPressed(ImGuiKey_H) && !ImGui::GetIO().WantCaptureKeyboard && !ctx.open_delete_popup)) {
            if (!ctx.show_history_window) {
                ctx.history_sessions = db.get_all_sessions();
                ctx.add_log("SYS_CALL: ctx.show_history_window = false; History closed!");
            } else {
                ctx.history_sessions.clear();
                ctx.add_log("SYS_CALL: ctx.show_history_window = true; History opened!");
            }
            ctx.show_history_window = !ctx.show_history_window;
        }
    }

    ImGui::Spacing();

    if (compact) {
        if (ImGui::Button(ctx.show_info_window ? "[I] Close Info" : "[I] App Info", ImVec2(-1, 50)) || (
                ImGui::IsKeyPressed(ImGuiKey_I) && !ImGui::GetIO().WantCaptureKeyboard)) {
            ctx.show_info_window = !ctx.show_info_window;
            if (ctx.show_info_window)
                ctx.add_log("SYS_CALL: ctx.show_info_window = true; Info Window opened!");
            if (!ctx.show_info_window)
                ctx.add_log("SYS_CALL: ctx.show_info_window = false; Info Window closed!");
        }
    } else {
        if (ImGui::Button("[I]", ImVec2(-1, 50)) || (
                ImGui::IsKeyPressed(ImGuiKey_I) && !ImGui::GetIO().WantCaptureKeyboard)) {
            ctx.show_info_window = !ctx.show_info_window;
            if (ctx.show_info_window)
                ctx.add_log("SYS_CALL: ctx.show_info_window = true; Info Window opened!");
            if (!ctx.show_info_window)
                ctx.add_log("SYS_CALL: ctx.show_info_window = false; Info Window closed!");
        }
    }

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    if (compact) {
        if (ImGui::Button(ctx.show_pql_guide ? "[P] Close PQL Guide" : "[P] Open PQL Guide", ImVec2(-1, 50)) || (
                ImGui::IsKeyPressed(ImGuiKey_P) && !ImGui::GetIO().WantCaptureKeyboard)) {
            ctx.show_pql_guide = !ctx.show_pql_guide;
            if (ctx.show_pql_guide)
                ctx.add_log("SYS_CALL: ctx.show_pql_guide = true; PQL Guide opened!");
            if (!ctx.show_pql_guide)
                ctx.add_log("SYS_CALL: ctx.show_pql_guide = false; PQL Guide closed!");
        }
    } else {
        if (ImGui::Button("[P]", ImVec2(-1, 50)) || (
                ImGui::IsKeyPressed(ImGuiKey_P) && !ImGui::GetIO().WantCaptureKeyboard)) {
            ctx.show_pql_guide = !ctx.show_pql_guide;
            if (ctx.show_pql_guide)
                ctx.add_log("SYS_CALL: ctx.show_pql_guide = true; PQL Guide opened!");
            if (!ctx.show_pql_guide)
                ctx.add_log("SYS_CALL: ctx.show_pql_guide = false; PQL Guide closed!");
        }
    }

    ImGui::Spacing();
    ImGui::Separator();

#ifndef NDEBUG
    RenderDebugMenu(ctx, db);
#endif

    ImGui::EndChild();
}

void RenderTopBar(AppContext &ctx) {
    float app_width = ImGui::GetWindowWidth();
    float h = ImGui::GetWindowHeight() * 0.07f;
    ImGui::BeginChild("TopBar", ImVec2(0, h), true);

    float text_scale = app_width / 1000.0f;
    if (text_scale < 0.8f)
        text_scale = 0.8f;
    if (text_scale > 2.0f)
        text_scale = 2.0f;

    ImGui::SetWindowFontScale(text_scale);

    std::time_t t = std::time(nullptr);
    std::tm *now_tm = std::localtime(&t);

    char clock_buffer[64];
    snprintf(clock_buffer, sizeof(clock_buffer), "%02d-%02d-%04d | %02d:%02d:%02d",
             now_tm->tm_mday, now_tm->tm_mon + 1, now_tm->tm_year + 1900,
             now_tm->tm_hour, now_tm->tm_min, now_tm->tm_sec);

    std::string clock_text = clock_buffer;

    // Dynamically calculate elements width for perfect right alignment
    float text_width = ImGui::CalcTextSize(clock_text.c_str()).x;
    float spacing = 15.0f * text_scale;

    // Calculate button width including its internal ImGui padding
    std::string btn_label = " X ";
    float button_width = ImGui::CalcTextSize(btn_label.c_str()).x + (ImGui::GetStyle().FramePadding.x * 2.0f);

    float total_width = text_width + spacing + button_width;

    // Position the cursor to anchor both elements to the right edge
    ImGui::SetCursorPosX(ImGui::GetWindowWidth() - total_width - 20.0f);

    // Vertically center the elements inside the top bar
    float cursor_y = (h - ImGui::GetTextLineHeight()) * 0.5f;
    ImGui::SetCursorPosY(cursor_y);

    // Render clock
    ImGui::TextColored(ImVec4(0.0f, 1.0f, 1.0f, 1.0f), "%s", clock_text.c_str());

    ImGui::SameLine(0.0f, spacing);

    // Adjust button Y position slightly upward to account for its frame padding
    ImGui::SetCursorPosY(cursor_y - ImGui::GetStyle().FramePadding.y);

    // Apply a danger color palette to the exit button
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.8f, 0.2f, 0.2f, 0.8f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(1.0f, 0.3f, 0.3f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.6f, 0.1f, 0.1f, 1.0f));

    // Close the app with no confirmation (except when a session is running)
    if (ImGui::Button(btn_label.c_str())) {
        if (ctx.current_state == AppState::RUNNING || ctx.current_state == AppState::PAUSED)
            ctx.show_exit_popup = true;
        else
            ctx.should_close = true;
    }

    ImGui::PopStyleColor(3);

    ImGui::EndChild();
}

void RenderLastActivityPanel(AppContext &ctx) {
    float app_width = ImGui::GetWindowWidth();

    float w = ImGui::GetWindowWidth() * 0.35f;
    float h = ImGui::GetWindowHeight() * 0.25f;
    ImGui::BeginChild("LastActivityPanel", ImVec2(w, h), true);

    float text_scale = app_width / 800.0f;
    if (text_scale < 0.8f)
        text_scale = 0.8f;
    if (text_scale > 2.0f)
        text_scale = 2.0f;

    ImGui::SetWindowFontScale(text_scale);

    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.70f, 0.00f, 1.00f, 1.00f));
    ImGui::Spacing();
    ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 10.0f);
    ImGui::Text("LATEST ACTIVITY");
    ImGui::PopStyleColor();

    ImGui::Separator();

    if (ctx.last_session.get_id() != -1) {
        float line_height = ImGui::GetTextLineHeightWithSpacing();
        float spacing_height = ImGui::GetTextLineHeight();
        float content_height = (7.0f * line_height) + (3.0f * spacing_height);
        float available_space = ImGui::GetWindowHeight() - ImGui::GetCursorPosY() - ImGui::GetStyle().WindowPadding.y;

        if (available_space > content_height) {
            float offset_y = (available_space - content_height) / 2.0f;
            ImGui::SetCursorPosY(ImGui::GetCursorPosY() + offset_y);
        }

        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 20.0f);
        ImGui::BeginGroup();

        ImVec4 color_title = ImVec4(1.0f, 0.84f, 0.0f, 1.0f);
        ImGui::TextColored(color_title, "[%s]", ctx.last_session.get_group_name().c_str());
        ImGui::TextColored(color_title, "%s", ctx.last_session.get_name().c_str());

        ImGui::Spacing();

        ImVec4 color_time = ImVec4(0.0f, 1.0f, 1.0f, 1.0f);
        ImGui::TextColored(color_time, "Work:  %s",
                           Timer::get_formatted_duration(ctx.last_session.get_duration_work()).c_str());
        ImGui::TextColored(color_time, "Pause: %s",
                           Timer::get_formatted_duration(ctx.last_session.get_duration_pause()).c_str());
        ImGui::TextColored(color_time, "Total: %s",
                           Timer::get_formatted_duration(ctx.last_session.get_duration_total()).c_str());

        ImGui::Spacing();

        ImVec4 color_eff = ImVec4(0.2f, 1.0f, 0.2f, 1.0f);
        ImGui::TextColored(color_eff, "Efficiency: %.1f%%", ctx.last_session.get_efficiency());

        ImGui::Spacing();

        ImVec4 color_date = ImVec4(0.6f, 0.6f, 0.6f, 1.0f);
        std::string datetime = ctx.last_session.get_timestamp().get_formatted_date() + " - " + ctx.last_session.
                               get_timestamp().get_formatted_time();
        ImGui::TextColored(color_date, "%s", datetime.c_str());

        ImGui::EndGroup();
    } else {
        ImGui::TextDisabled("No sessions recorded yet.");
    }

    ImGui::EndChild();
}

void RenderPQLGuideWindow(AppContext &ctx) {
    if (!ctx.show_pql_guide)
        return;

    ImVec2 display_size = ImGui::GetIO().DisplaySize;

    static ImVec2 old_display_size = display_size;
    static ImVec2 current_window_size = ImVec2(display_size.x * 0.8f, display_size.y * 0.6f);
    static bool is_first_frame = true;

    // Detect structural changes in the application viewport to maintain proportional sizing.
    if (display_size.x != old_display_size.x || display_size.y != old_display_size.y) {
        current_window_size.x = display_size.x * 0.8f;
        current_window_size.y = display_size.y * 0.6f;
        old_display_size = display_size;
        ctx.force_window_resize = true;
    }

    if (ctx.force_window_resize || is_first_frame) {
        ImGui::SetNextWindowSize(current_window_size, ImGuiCond_Always);
        ctx.force_window_resize = false;
        is_first_frame = false;
    }

    if (ImGui::Begin("  PQL Reference Guide", &ctx.show_pql_guide)) {
        if (!ctx.show_pql_guide)
            ctx.add_log("SYS_CALL: ctx.show_pql_guide = false; PQL Guide closed!");

        if (ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows) && ImGui::IsKeyPressed(ImGuiKey_Escape)) {
            ctx.show_pql_guide = false;
            ctx.add_log("SYS_CALL: ctx.show_pql_guide = false; PQL Guide closed!");
        }

        current_window_size = ImGui::GetWindowSize();

        float app_width = ImGui::GetWindowWidth();
        float text_scale = app_width / 1100.0f;

        if (text_scale < 0.5f)
            text_scale = 0.5f;
        if (text_scale > 2.0f)
            text_scale = 2.0f;
        ImGui::SetWindowFontScale(text_scale);

        ImGui::TextWrapped(
            "PQL (Productivity Query Language) is a powerful tool to filter your sessions. Type multiple filters separated by spaces.");
        ImGui::Separator();
        ImGui::Spacing();
        ImGui::Spacing();

        ImVec4 color_key = ImVec4(0.0f, 1.0f, 0.0f, 1.0f);
        ImVec4 color_ex = ImVec4(1.0f, 0.84f, 0.0f, 1.0f);

        ImGui::TextColored(color_key, "1. BASIC SYNTAX & OPERATORS");
        ImGui::BulletText("Format: key:operator_value");
        ImGui::BulletText("Operators: =, >, <, >=, <=");
        ImGui::Spacing();
        ImGui::Spacing();

        ImGui::TextColored(color_key, "2. TEXT FILTERS");
        ImGui::Text("  group:   Search by group name.   ");
        ImGui::SameLine();
        ImGui::TextColored(color_ex, "ex: group:UNI");
        ImGui::Text("  name:    Search by task name.    ");
        ImGui::SameLine();
        ImGui::TextColored(color_ex, "ex: name:Bug");
        ImGui::Spacing();
        ImGui::Spacing();

        ImGui::TextColored(color_key, "3. NUMERIC & TIME FILTERS");
        ImGui::BulletText("Time format: Use h (hours), m (minutes), s (seconds).");
        ImGui::Text("  work:    Total time.             ");
        ImGui::SameLine();
        ImGui::TextColored(color_ex, "ex: work:>2h30m");
        ImGui::Text("  awork:   Active work time.       ");
        ImGui::SameLine();
        ImGui::TextColored(color_ex, "ex: awork:<45m");
        ImGui::Text("  pwork:   Pause time.             ");
        ImGui::SameLine();
        ImGui::TextColored(color_ex, "ex: pwork:>=10m");
        ImGui::Text("  eff:     Efficiency percentage.  ");
        ImGui::SameLine();
        ImGui::TextColored(color_ex, "ex: eff:>80.5");
        ImGui::Spacing();
        ImGui::Spacing();

        ImGui::TextColored(color_key, "4. DATE FILTERS & INTERVALS");
        ImGui::BulletText("Specific Date: DD-MM-YYYY (or just DD or DD-MM, default for MM-YYYY is current date).");
        ImGui::BulletText(
            "Relative Date: lastXM, lastXh, lastXd, lastXm, lastXy (M=minutes, h=hours, d=days, m=months, y=years).");
        ImGui::BulletText("Intervals (&): Combine two rules for the same key.");
        ImGui::Text("  date:    ");
        ImGui::SameLine();
        ImGui::TextColored(color_ex, "ex: date:15-09-2026 or date:last7d");
        ImGui::Text("  interval:");
        ImGui::SameLine();
        ImGui::TextColored(color_ex, "ex: date:>10&<=20 or eff:>50&<80");

        ImGui::Spacing();
        ImGui::Spacing();
        ImGui::Separator();
        ImGui::TextDisabled("Try this full example in the search bar:");
        ImGui::TextColored(ImVec4(0.0f, 1.0f, 1.0f, 1.0f), "group:PROJECT date:last7d work:>1h eff:>80");
    }
    ImGui::End();
}

void RenderActivityLogPanel(AppContext &ctx) {
    float app_width = ImGui::GetWindowWidth();

    float w = ImGui::GetWindowWidth() * 0.30f;
    float h = ImGui::GetWindowHeight() * 0.25f;
    ImGui::BeginChild("SystemLogPanel", ImVec2(w, h), true);

    float text_scale = app_width / 1000.0f;
    if (text_scale < 0.8f)
        text_scale = 0.8f;
    if (text_scale > 2.0f)
        text_scale = 2.0f;
    ImGui::SetWindowFontScale(text_scale);

    ImGui::Spacing();
    ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 10.0f * text_scale);

    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.2f, 1.0f, 0.4f, 1.0f));
    ImGui::Text(" TERMINAL // SYS_LOGS");
    ImGui::PopStyleColor();
    ImGui::Separator();

    // Secondary child window configured for automatic vertical scrolling to display historic logs.
    ImGui::BeginChild("LogScrollRegion", ImVec2(0, 0), false, ImGuiWindowFlags_AlwaysVerticalScrollbar);
    for (const auto &log: ctx.system_logs) {
        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 3.0f);
        ImGui::PushTextWrapPos(0.0f);

        // Pattern matching to apply semantic coloring for the retro terminal aesthetic.
        if (log.find("HUMAN_SPIRIT") != std::string::npos)
            ImGui::TextColored(ImVec4(1.0f, 0.0f, 1.0f, 1.0f), "%s", log.c_str());
        else if (log.find("ERROR") != std::string::npos || log.find("DELETE") != std::string::npos)
            ImGui::TextColored(ImVec4(1.0f, 0.3f, 0.3f, 1.0f), "%s", log.c_str());
        else if (log.find("DB_WRITE") != std::string::npos)
            ImGui::TextColored(ImVec4(0.0f, 0.8f, 1.0f, 1.0f), "%s", log.c_str());
        else if (log.find("AppState mutated -> RUNNING") != std::string::npos)
            ImGui::TextColored(ImVec4(0.18f, 0.8f, 0.44f, 1.0f), "%s", log.c_str());
        else if (log.find("AppState mutated -> PAUSED") != std::string::npos)
            ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.0f, 1.0f), "%s", log.c_str());
        else if (log.find("opened!") != std::string::npos)
            ImGui::TextColored(ImVec4(0.63f, 0.13f, 0.94f, 1.0f), "%s", log.c_str());
        else if (log.find("closed!") != std::string::npos)
            ImGui::TextColored(ImVec4(0.0f, 0.5f, 1.0f, 1.0f), "%s", log.c_str());
        else if (log.find("TODO_EVENT") != std::string::npos)
            ImGui::TextColored(ImVec4(0.87f, 0.80f, 0.67f, 1.0f), "%s", log.c_str());
        else
            ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "%s", log.c_str());

        ImGui::PopTextWrapPos();
    }

    // Anchor scroll strictly to the bottom unless manually overridden by the user.
    ImGui::Dummy(ImVec2(0.0f, 5.0f));
    if (ImGui::GetScrollY() >= ImGui::GetScrollMaxY()) {
        ImGui::SetScrollHereY(0.9f);
    }

    ImGui::EndChild();
    ImGui::EndChild();
}

void RenderTopGroupsPanel(AppContext &ctx) {
    float app_width = ImGui::GetWindowWidth();
    float h = ImGui::GetWindowHeight() * 0.25f;
    ImGui::BeginChild("TopGroupsPanel", ImVec2(0, h), true);

    float text_scale = app_width / 1000.0f;
    if (text_scale < 0.8f)
        text_scale = 0.8f;
    if (text_scale > 2.0f)
        text_scale = 2.0f;
    ImGui::SetWindowFontScale(text_scale);

    ImGui::Spacing();
    ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 10.0f * text_scale);

    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.0f, 0.8f, 1.0f, 1.0f));
    ImGui::Text(" TOP 3 GROUPS // STATS");
    ImGui::PopStyleColor();
    ImGui::Separator();
    ImGui::Spacing();

    // Dynamic scaling logic based on the actual number of active project groups returned by the DB.
    float line_height = ImGui::GetTextLineHeightWithSpacing();
    float spacing_height = ImGui::GetTextLineHeight();
    float content_height = 0.0f;

    if (ctx.top_groups.empty()) {
        content_height = line_height;
    } else {
        content_height = ctx.top_groups.size() * (2.0f * line_height + spacing_height);
    }

    float available_space = ImGui::GetWindowHeight() - ImGui::GetCursorPosY() - ImGui::GetStyle().WindowPadding.y;
    bool compact = app_width < 600.0f;

    if (available_space > content_height) {
        float offset_y = (available_space - content_height) / 2.0f;
        ImGui::SetCursorPosY(ImGui::GetCursorPosY() + offset_y);
    }

    if (ctx.top_groups.empty()) {
        ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), " No data available yet.");
    } else {
        for (size_t i = 0; i < ctx.top_groups.size(); ++i) {
            const auto &stat = ctx.top_groups[i];
            float avg_eff = stat.sum_efficiency / stat.session_count;
            std::string time_str = Timer::get_formatted_duration(stat.total_work);

            ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.0f, 1.0f), " %d. [%s]", (int) (i + 1), stat.name.c_str());

            float indent = 25.0f * text_scale;
            ImGui::SetCursorPosX(ImGui::GetCursorPosX() + indent);

            if (compact)
                ImGui::Text("%s", time_str.c_str());
            else
                ImGui::Text("Time: %s", time_str.c_str());

            ImGui::SameLine();
            if (compact)
                ImGui::Text(" %d", stat.session_count);
            else
                ImGui::Text(" Sess: %d", stat.session_count);

            ImGui::SameLine();
            if (compact)
                ImGui::TextColored(ImVec4(0.2f, 1.0f, 0.4f, 1.0f), " %.1f%%", avg_eff);
            else
                ImGui::TextColored(ImVec4(0.2f, 1.0f, 0.4f, 1.0f), " Eff: %.1f%%", avg_eff);

            ImGui::Spacing();
        }
    }

    ImGui::EndChild();
}

void RenderDailyTimelinePanel(AppContext &ctx, DatabaseHandler &db) {
    float app_width = ImGui::GetWindowWidth();
    ImGui::BeginChild("DailyTimelinePanel", ImVec2(0, 0), true);

    float text_scale = app_width / 1000.0f;
    if (text_scale < 0.8f)
        text_scale = 0.8f;
    if (text_scale > 2.0f)
        text_scale = 2.0f;
    ImGui::SetWindowFontScale(text_scale);

    ImGui::Spacing();
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.00f, 0.84f, 0.00f, 1.00f));
    ImGui::Text(" TODAY // VISUAL TIME BAR");
    ImGui::PopStyleColor();
    ImGui::Separator();

    std::time_t t = std::time(nullptr);
    std::tm *now = std::localtime(&t);
    char today_date[16];
    snprintf(today_date, sizeof(today_date), "%02d-%02d-%04d", now->tm_mday, now->tm_mon + 1, now->tm_year + 1900);
    std::string today_str = today_date;

    static int last_processed_id = -1;
    static std::string last_checked_date = "";
    static std::vector<Session> cached_today_sessions;

    // Cache invalidation mechanism based on DB modification IDs and day rollovers.
    if (last_processed_id != ctx.last_session.get_id() || last_checked_date != today_str) {
        bool dummy_error = false;
        std::string pql_query = "date:" + today_str;

        cached_today_sessions = db.search_sessions(pql_query, dummy_error);
        last_processed_id = ctx.last_session.get_id();
        last_checked_date = today_str;
    }

    ImDrawList *draw_list = ImGui::GetWindowDrawList();
    ImVec2 canvas_pos = ImGui::GetCursorScreenPos();
    ImVec2 canvas_size = ImGui::GetContentRegionAvail();

    // Geometric constants configured to prevent edge-culling on extreme bounds (00:00 and 24:00).
    float padding_x = 35.0f * text_scale;
    float bar_width = canvas_size.x - (padding_x * 2.0f);
    float bar_height = 20.0f * text_scale;
    float max_tick_height = 8.0f;
    float text_offset = 2.0f;
    float text_height = ImGui::GetTextLineHeight();

    float total_visual_height = bar_height + max_tick_height + text_offset + text_height;
    float bar_y = canvas_pos.y + (canvas_size.y - total_visual_height) * 0.5f;

    ImVec2 bar_p0 = ImVec2(canvas_pos.x + padding_x, bar_y);
    ImVec2 bar_p1 = ImVec2(bar_p0.x + bar_width, bar_p0.y + bar_height);

    draw_list->AddRectFilled(bar_p0, bar_p1, IM_COL32(30, 30, 30, 255));
    draw_list->AddRect(bar_p0, bar_p1, IM_COL32(100, 100, 100, 255));

    for (const auto &session: cached_today_sessions) {
        int start_min = (session.get_timestamp().get_hour() * 60) + session.get_timestamp().get_minute();
        int duration_mins = session.get_duration_total() / 60;

        float start_x = bar_p0.x + ((float) start_min / 1440.0f) * bar_width;
        float end_x = bar_p0.x + ((float) (start_min + duration_mins) / 1440.0f) * bar_width;

        // Force a minimum rendering width of 3px to maintain sub-minute visibility.
        if ((end_x - start_x) < 3.0f) {
            end_x = start_x + 3.0f;
        }

        if (end_x > bar_p1.x)
            end_x = bar_p1.x;

        ImVec2 seg_p0 = ImVec2(start_x, bar_p0.y + 1);
        ImVec2 seg_p1 = ImVec2(end_x, bar_p1.y - 1);

        ImVec2 mouse_pos = ImGui::GetMousePos();
        bool is_hovered = (mouse_pos.x >= seg_p0.x && mouse_pos.x <= seg_p1.x &&
                           mouse_pos.y >= seg_p0.y && mouse_pos.y <= seg_p1.y);

        if (is_hovered) {
            draw_list->AddRectFilled(seg_p0, seg_p1, IM_COL32(150, 255, 180, 255));

            // Dynamic pivot prevents tooltip overflow for late-night sessions anchoring it leftwards.
            float screen_width = ImGui::GetIO().DisplaySize.x;
            float pivot_x = (mouse_pos.x > screen_width * 0.65f) ? 1.0f : 0.0f;
            ImGui::SetNextWindowPos(ImVec2(mouse_pos.x, mouse_pos.y - 15.0f), ImGuiCond_Always, ImVec2(pivot_x, 1.0f));

            ImGui::BeginTooltip();
            ImGui::SetWindowFontScale(text_scale);

            ImGui::Spacing();
            ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 10.0f);
            ImGui::TextColored(ImVec4(1.0f, 0.84f, 0.0f, 1.0f), "[%s]", session.get_group_name().c_str());
            ImGui::SameLine();
            ImGui::Text("%s ", session.get_name().c_str());
            ImGui::Separator();

            int end_time_min = start_min + duration_mins;
            int end_h = (end_time_min / 60) % 24;
            int end_m = end_time_min % 60;

            ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 10.0f);
            ImGui::Text("Interval: %02d:%02d - %02d:%02d ", session.get_timestamp().get_hour(),
                        session.get_timestamp().get_minute(), end_h, end_m);

            ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 10.0f);
            ImGui::TextColored(ImVec4(0.0f, 1.0f, 1.0f, 1.0f), "Work Time: %s ",
                               Timer::get_formatted_duration(session.get_duration_work()).c_str());

            ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 10.0f);
            ImGui::TextColored(ImVec4(0.2f, 1.0f, 0.4f, 1.0f), "Efficiency: %.1f%% ", session.get_efficiency());

            ImGui::Spacing();
            ImGui::Spacing();

            ImGui::EndTooltip();
        } else {
            draw_list->AddRectFilled(seg_p0, seg_p1, IM_COL32(50, 255, 100, 255));
        }
    }

    ImU32 text_col = ImGui::GetColorU32(ImGuiCol_TextDisabled);
    for (int i = 0; i <= 24; i++) {
        float x = bar_p0.x + ((float) i / 24.0f) * bar_width;
        float tick_height = (i % 6 == 0) ? max_tick_height : 4.0f;

        draw_list->AddLine(ImVec2(x, bar_p1.y), ImVec2(x, bar_p1.y + tick_height), IM_COL32(150, 150, 150, 255));

        if (i % 4 == 0) {
            char hour_str[8];
            snprintf(hour_str, sizeof(hour_str), "%02d:00", i);
            float txt_w = ImGui::CalcTextSize(hour_str).x;
            draw_list->AddText(ImVec2(x - txt_w * 0.5f, bar_p1.y + tick_height + text_offset), text_col, hour_str);
        }
    }

    ImGui::EndChild();
}

void RenderInfoWindow(AppContext &ctx) {
    if (!ctx.show_info_window)
        return;

    ImVec2 display_size = ImGui::GetIO().DisplaySize;

    static ImVec2 old_display_size = display_size;
    static ImVec2 current_window_size = ImVec2(display_size.x * 0.8f, display_size.y * 0.8f);
    static bool is_first_frame = true;

    // Responsive window resizing logic
    if (display_size.x != old_display_size.x || display_size.y != old_display_size.y) {
        current_window_size.x = display_size.x * 0.8f;
        current_window_size.y = display_size.y * 0.8f;
        old_display_size = display_size;
        ctx.force_window_resize = true;
    }

    if (ctx.force_window_resize || is_first_frame) {
        ImGui::SetNextWindowSize(current_window_size, ImGuiCond_Always);
        ctx.force_window_resize = false;
        is_first_frame = false;
    }

    if (ImGui::Begin("  About Productivity Tracker", &ctx.show_info_window)) {
        if (!ctx.show_info_window)
            ctx.add_log("SYS_CALL: ctx.show_info_window = false; Info closed!");

        if (ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows) && ImGui::IsKeyPressed(ImGuiKey_Escape)) {
            ctx.show_info_window = false;
            ctx.add_log("SYS_CALL: ctx.show_info_window = false; Info closed!");
        }

        float app_width = ImGui::GetWindowWidth();
        float text_scale = app_width / 700.0f;
        if (text_scale < 0.6f)
            text_scale = 0.6f;
        if (text_scale > 1.5f)
            text_scale = 1.5f;
        ImGui::SetWindowFontScale(text_scale);

        // --- APP HEADER ---
        ImGui::TextColored(ImVec4(0.0f, 1.0f, 1.0f, 1.0f), "%s v%s", APP_NAME, APP_VERSION);
        ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "Created by: tanasedarius");
        ImGui::Spacing();
        ImGui::TextWrapped(
            "A high-performance, C++ based productivity dashboard designed to track deep work sessions, monitor efficiency, and prevent procrastination using SQLite and Dear ImGui.");

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        // --- WIDGETS DESCRIPTION ---
        ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0.2f, 0.2f, 0.2f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImVec4(0.3f, 0.3f, 0.3f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_HeaderActive, ImVec4(0.4f, 0.4f, 0.4f, 1.0f));

        if (ImGui::CollapsingHeader(" Widget Breakdown", ImGuiTreeNodeFlags_DefaultOpen)) {
            ImVec4 highlight = ImVec4(1.0f, 0.84f, 0.0f, 1.0f); // Yellow

            ImGui::TextColored(highlight, "Dashboard:");
            ImGui::SameLine(180 * text_scale);
            ImGui::TextWrapped("Core timer interface to start, pause, and save work sessions.");

            ImGui::TextColored(highlight, "History Browser:");
            ImGui::SameLine(180 * text_scale);
            ImGui::TextWrapped("Database table view with PQL (Productivity Query Language) filtering support.");

            ImGui::TextColored(highlight, "Global Heatmap:");
            ImGui::SameLine(180 * text_scale);
            ImGui::TextWrapped("90-day GitHub-style contribution graph based on total work time.");

            ImGui::TextColored(highlight, "Time Slipping:");
            ImGui::SameLine(180 * text_scale);
            ImGui::TextWrapped("Daily, weekly, monthly, and yearly progress bars to enforce time awareness.");

            ImGui::TextColored(highlight, "Daily Timeline:");
            ImGui::SameLine(180 * text_scale);
            ImGui::TextWrapped("Visual chart displaying today's work intervals.");

            ImGui::TextColored(highlight, "Sys Logs:");
            ImGui::SameLine(180 * text_scale);
            ImGui::TextWrapped("Retro-style terminal monitoring internal application events and database queries.");

            ImGui::TextColored(highlight, "Group Stats:");
            ImGui::SameLine(180 * text_scale);
            ImGui::TextWrapped("In-depth metrics (Work, Pause, Efficiency) for the currently active project group.");
        }

        ImGui::Spacing();

        // --- KEYBOARD SHORTCUTS ---
        if (ImGui::CollapsingHeader(" Keyboard Shortcuts")) {
            ImVec4 key_color = ImVec4(0.2f, 1.0f, 0.4f, 1.0f); // Neon Green

            ImGui::TextColored(key_color, "[ T ]");
            ImGui::SameLine(80 * text_scale);
            ImGui::Text("Toggle Main Dashboard");
            ImGui::TextColored(key_color, "[ H ]");
            ImGui::SameLine(80 * text_scale);
            ImGui::Text("Toggle History Browser");
            ImGui::TextColored(key_color, "[ P ]");
            ImGui::SameLine(80 * text_scale);
            ImGui::Text("Toggle PQL Reference Guide");
            ImGui::TextColored(key_color, "[ I ]");
            ImGui::SameLine(80 * text_scale);
            ImGui::Text("Toggle Info/About Window");
            ImGui::TextColored(key_color, "[ C ]");
            ImGui::SameLine(80 * text_scale);
            ImGui::Text("Create a new group (when Dashboard is open)");
            ImGui::TextColored(key_color, "[ S ]");
            ImGui::SameLine(80 * text_scale);
            ImGui::Text("Stop & Save active session");
            ImGui::TextColored(key_color, "[Esc]");
            ImGui::SameLine(80 * text_scale);
            ImGui::Text("Close active modals or Exit application");
        }

        ImGui::PopStyleColor(3);
    }
    ImGui::End();
}

// =====================================================================
// CORE ARCHITECTURE ORCHESTRATOR
// =====================================================================

void RenderBackground(AppContext &ctx, DatabaseHandler &db) {
    const ImGuiViewport *viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(viewport->WorkPos);
    ImGui::SetNextWindowSize(viewport->WorkSize);

    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));

    ImGuiWindowFlags flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse |
                             ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
                             ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus |
                             ImGuiWindowFlags_NoScrollbar;

    ImGui::Begin("DesktopBackground", nullptr, flags);

    RenderSidebar(ctx, db);
    ImGui::SameLine();

    ImGui::BeginChild("MainWorkspace", ImVec2(0, 0), false);

    RenderTopBar(ctx);
    ImGui::Spacing();

    RenderLastActivityPanel(ctx);
    ImGui::SameLine();

    RenderGroupStatsPanel(ctx, db);
    ImGui::SameLine();

    RenderNotepadPanel(ctx);
    RenderHeatmapPanel(ctx, db);
    ImGui::SameLine();

    RenderTimeAwarenessPanel();
    RenderToDoPanel(ctx);
    ImGui::SameLine();

    RenderActivityLogPanel(ctx);
    ImGui::SameLine();

    RenderTopGroupsPanel(ctx);
    RenderDailyTimelinePanel(ctx, db);

    ImGui::EndChild();
    ImGui::End();
    ImGui::PopStyleVar(3);

    RenderPQLGuideWindow(ctx);
    RenderInfoWindow(ctx);
}

void RenderDashboard(AppContext &ctx, DatabaseHandler &db) {
    ImVec2 display_size = ImGui::GetIO().DisplaySize;

    static ImVec2 old_display_size = display_size;
    static ImVec2 current_window_size = ImVec2(display_size.x * 0.5f, display_size.y * 0.3f);
    static bool is_first_frame = true;

    // Responsive adaptation mechanism to force resize the layout when native resolution changes.
    if (display_size.x != old_display_size.x || display_size.y != old_display_size.y) {
        current_window_size.x = display_size.x * 0.5f;
        current_window_size.y = display_size.y * 0.3f;
        old_display_size = display_size;
        ctx.force_window_resize = true;
    }

    if (ctx.force_window_resize || is_first_frame) {
        ImGui::SetNextWindowSize(current_window_size, ImGuiCond_Always);
        ctx.force_window_resize = false;
        is_first_frame = false;
    }

    ImGui::Begin("Dashboard", &ctx.show_dashbord);
    if (!ctx.show_dashbord)
        ctx.add_log("SYS_CALL: ctx.show_dashboard = false; Dashboard closed!");

    if (ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows) && ImGui::IsKeyPressed(ImGuiKey_Escape)) {
        ctx.show_dashbord = false;
        ctx.add_log("SYS_CALL: ctx.show_dashboard = false; Dashboard closed!");
    }

    current_window_size = ImGui::GetWindowSize();

    float app_width = ImGui::GetWindowWidth();
    float text_scale = app_width / 500.0f;
    if (text_scale < 0.8f)
        text_scale = 0.8f;
    if (text_scale > 2.0f)
        text_scale = 2.0f;

    ImGui::SetWindowFontScale(text_scale);

    bool compact = app_width > 1000.0f;

    ImGui::Text("=== PRODUCTIVITY TRACKER ===");
    ImGui::Spacing();

    if (ImGui::IsKeyPressed(ImGuiKey_Enter))
        ctx.focus_textbox = true;

    if (ctx.current_state == AppState::IDLE) {
        if (!ctx.groups_loaded) {
            ctx.available_groups = db.get_unique_groups();
            std::string temp_str = db.get_last_used_group();

            strncpy(ctx.group_name, temp_str.c_str(), sizeof(ctx.group_name) - 1);
            ctx.group_name[sizeof(ctx.group_name) - 1] = '\0';
            ctx.groups_loaded = true;
        }

        if (!ctx.is_creating_new_group) {
            if (ImGui::IsKeyPressed(ImGuiKey_C) && !ImGui::GetIO().WantCaptureKeyboard) {
                ctx.focus_new_group_textbox = true;
                ctx.is_creating_new_group = true;
                memset(ctx.group_name, 0, sizeof(ctx.group_name));
            }
            const char *preview_value = ctx.available_groups.empty() ? "Create Group" : ctx.group_name;

            if (ImGui::BeginCombo("Group", preview_value)) {
                for (int i = 0; i < ctx.available_groups.size(); ++i) {
                    const bool is_selected = (ctx.selected_group_index == i);
                    if (ImGui::Selectable(ctx.available_groups[i].c_str(), is_selected)) {
                        ctx.selected_group_index = i;
                        strncpy(ctx.group_name, ctx.available_groups[i].c_str(), sizeof(ctx.group_name) - 1);
                    }
                    if (is_selected)
                        ImGui::SetItemDefaultFocus();
                }
                ImGui::Separator();
                if (ImGui::Selectable("[C] Create New Group...")) {
                    ctx.focus_new_group_textbox = true;
                    ctx.is_creating_new_group = true;
                    memset(ctx.group_name, 0, sizeof(ctx.group_name));
                }
                ImGui::EndCombo();
            }
        } else {
            if (ctx.focus_new_group_textbox) {
                ImGui::SetKeyboardFocusHere();
                ctx.focus_new_group_textbox = false;
            }
            if (ImGui::InputText("New Group", ctx.group_name, IM_ARRAYSIZE(ctx.group_name),
                                 ImGuiInputTextFlags_EnterReturnsTrue)) {
                if (strlen(ctx.group_name) > 0) {
                    ctx.focus_textbox = true;
                }
                ImGui::GetIO().ClearInputKeys();
            }
            ImGui::SameLine();
            if (ImGui::Button("Cancel") || (ImGui::IsKeyPressed(ImGuiKey_Delete) && !ImGui::GetIO().
                                            WantCaptureKeyboard)) {
                ctx.is_creating_new_group = false;
                ctx.focus_textbox = true;
                if (!ctx.available_groups.empty()) {
                    strncpy(ctx.group_name, ctx.available_groups[ctx.selected_group_index].c_str(),
                            sizeof(ctx.group_name) - 1);
                }
            }
        }

        if (ctx.focus_textbox) {
            ImGui::SetKeyboardFocusHere();
            ctx.focus_textbox = false;
        }
        ImGui::InputText("Activity Name", ctx.activity_name, IM_ARRAYSIZE(ctx.activity_name));

        if ((ImGui::Button("Start Session", ImVec2(150 * text_scale, 40 * text_scale))) || (
                ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows) && ImGui::IsKeyPressed(ImGuiKey_Enter))) {
            if (strlen(ctx.activity_name) > 0 && strlen(ctx.group_name) > 0) {
                ctx.session_timer.start();
                ctx.pause_timer.start();
                ctx.pause_timer.pause();

                ctx.session_start_time = Timestamp::get_current_timestamp();

                ctx.current_state = AppState::RUNNING;
                ctx.status_message = "Tracking in progress...";
                ctx.add_log(
                    "SYS_CALL: AppState mutated -> RUNNING. Thread attached to [" + std::string(ctx.group_name) + "]");
            } else {
                ctx.status_message = "Error: Input, both, group name and activity name!";
            }
        }
    } else {
        ImGui::Text("Currently tracking: %s", ctx.activity_name);
        ImGui::Spacing();

        long long work_sec = ctx.session_timer.get_elapsed_seconds();
        long long pause_sec = ctx.pause_timer.get_elapsed_seconds();

        float efficiency = 100.0f;
        if (work_sec + pause_sec > 0) {
            efficiency = (static_cast<float>(work_sec) * 100.0f) / static_cast<float>(work_sec + pause_sec);
        }

        ImGui::Text("Work Time: %s", Timer::get_formatted_duration(work_sec).c_str());
        ImGui::Text("Pause Time: %s", Timer::get_formatted_duration(pause_sec).c_str());
        ImGui::TextColored(ImVec4(0.0f, 1.0f, 1.0f, 1.0f), "Efficiency: %.1f%%", efficiency);
        ImGui::Spacing();

        const char *label = (ctx.current_state == AppState::RUNNING) ? "Pause" : "Resume";
        if ((ImGui::Button(label, ImVec2(150 * text_scale, 40 * text_scale))) || (
                ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows) && ImGui::IsKeyPressed(ImGuiKey_Enter))) {
            if (ctx.current_state == AppState::RUNNING) {
                ctx.session_timer.pause();
                ctx.pause_timer.resume();
                ctx.current_state = AppState::PAUSED;
                ctx.status_message = "Session paused.";
                ctx.add_log("SYS_CALL: AppState mutated -> PAUSED. Timers halted.");
            } else {
                ctx.session_timer.resume();
                ctx.pause_timer.pause();
                ctx.current_state = AppState::RUNNING;
                ctx.status_message = "Tracking in progress...";
                ctx.add_log("SYS_CALL: AppState mutated -> RUNNING. Timers resumed.");
            }
        }

        ImGui::SameLine();
        if (ImGui::Button("Stop & Save", ImVec2(150 * text_scale, 40 * text_scale)) || (
                ImGui::IsKeyPressed(ImGuiKey_S) && !ImGui::GetIO().WantCaptureKeyboard)) {
            ctx.is_creating_new_group = false;

            ctx.session_timer.stop();
            ctx.pause_timer.stop();

            int final_work = static_cast<int>(ctx.session_timer.get_elapsed_seconds());
            int final_pause = static_cast<int>(ctx.pause_timer.get_elapsed_seconds());

            Session current_session(std::string(ctx.group_name), std::string(ctx.activity_name), final_work,
                                    final_pause, ctx.session_start_time);

            if (db.save_session(current_session)) {
                ctx.last_session = db.get_last_session();
                ctx.status_message = "Saved successfully! ID: " + std::to_string(current_session.get_id());
                if (ctx.show_history_window)
                    ctx.history_sessions = db.get_all_sessions();
                ctx.available_groups.clear();
                ctx.groups_loaded = false;
                ctx.add_log(
                    "DB_WRITE: Session flushed to persistent storage. Assigned ID: " + std::to_string(
                        current_session.get_id()));
                ctx.add_log("SYS_CALL: AppState mutated -> IDLE.");
                db.update_top_groups(db, ctx);
            } else {
                ctx.status_message = "Failed to save session to database.";
            }

            ctx.current_state = AppState::IDLE;
            memset(ctx.activity_name, 0, sizeof(ctx.activity_name));
        }
    }

    ImGui::SameLine();
    const char *label_history = (!ctx.show_history_window) ? "View History" : "Close History";
    if (ImGui::Button(label_history, ImVec2(150 * text_scale, 40 * text_scale))) {
        if (!ctx.show_history_window) {
            ctx.history_sessions = db.get_all_sessions();
            ctx.show_history_window = true;
        } else {
            ctx.show_history_window = false;
            ctx.history_sessions.clear();
        }
    }

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "%s", ctx.status_message.c_str());
    ImGui::End();
}

void RenderHistoryWindow(AppContext &ctx, DatabaseHandler &db) {
    ImVec2 display_size = ImGui::GetIO().DisplaySize;

    static ImVec2 old_display_size = display_size;
    static ImVec2 current_window_size = ImVec2(display_size.x * 0.7f, display_size.y * 0.4f);
    static bool is_first_frame = true;

    if (display_size.x != old_display_size.x || display_size.y != old_display_size.y) {
        current_window_size.x = display_size.x * 0.7f;
        current_window_size.y = display_size.y * 0.4f;
        old_display_size = display_size;
        ctx.force_window_resize = true;
    }

    if (ctx.force_window_resize || is_first_frame) {
        ImGui::SetNextWindowSize(current_window_size, ImGuiCond_Always);
        ctx.force_window_resize = false;
        is_first_frame = false;
    }

    if (ImGui::Begin("Session History", &ctx.show_history_window)) {
        if (!ctx.show_history_window)
            ctx.add_log("SYS_CALL: ctx.show_history_window = false; History closed!");

        if (ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows) && ImGui::IsKeyPressed(ImGuiKey_Escape) && !
            ctx.open_delete_popup) {
            ctx.show_history_window = false;
            ctx.add_log("SYS_CALL: ctx.show_history_window = false; History closed!");
        }

        current_window_size = ImGui::GetWindowSize();
        float app_width = ImGui::GetWindowWidth();
        float text_scale = app_width / 1000.0f;
        if (text_scale < 0.5f)
            text_scale = 0.5f;
        if (text_scale > 2.0f)
            text_scale = 2.0f;
        ImGui::SetWindowFontScale(text_scale);

        ImGui::Text(" Filter (PQL):");
        ImGui::SameLine();

        bool color_pushed = ctx.search_error;
        if (color_pushed)
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.2f, 0.2f, 1.0f));

        if (ctx.focus_textbox) {
            ImGui::SetKeyboardFocusHere();
            ctx.focus_textbox = false;
        }

        if (ImGui::IsKeyPressed(ImGuiKey_Enter) && !ctx.open_delete_popup)
            ctx.focus_textbox = true;

        if (ImGui::InputText("##HistorySearchBox", ctx.search_buffer, IM_ARRAYSIZE(ctx.search_buffer))) {
            if (strlen(ctx.search_buffer) > 0) {
                ctx.filtered_sessions = db.search_sessions(ctx.search_buffer, ctx.search_error);
            } else {
                ctx.filtered_sessions.clear();
                ctx.search_error = false;
            }
        }

        ImGui::SameLine();

        if (ImGui::Button("Clear", ImVec2(130 * text_scale, 0)) || ImGui::IsKeyPressed(ImGuiKey_Delete)) {
            ctx.search_buffer[0] = '\0';
        }

        if (color_pushed)
            ImGui::PopStyleColor();

        ImGui::Separator();

        const std::vector<Session> &display_list = (strlen(ctx.search_buffer) > 0)
                                                       ? ctx.filtered_sessions
                                                       : ctx.history_sessions;

        ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, ImVec2(2.0f, 6.0f));
        if (ImGui::BeginTable("HistoryTable", 8,
                              ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_Resizable)) {
            ImGui::TableSetupColumn("ID");
            ImGui::TableSetupColumn("Group Name");
            ImGui::TableSetupColumn("Activity Name");
            ImGui::TableSetupColumn("Work Time");
            ImGui::TableSetupColumn("Pause Time");
            ImGui::TableSetupColumn("Total Time");
            ImGui::TableSetupColumn("Efficiency");
            ImGui::TableSetupColumn("Date & Time");
            ImGui::TableHeadersRow();

            for (const auto &session: display_list) {
                int index = 0;
                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(index++);

                std::string selectable_label = std::to_string(session.get_id()) + "##" + std::to_string(
                                                   session.get_id());

                if (ImGui::Selectable(selectable_label.c_str(), false, ImGuiSelectableFlags_SpanAllColumns)) {
                    ctx.session_id_to_delete = session.get_id();
                    ctx.open_delete_popup = true;
                }

                ImGui::TableSetColumnIndex(index++);
                ImGui::Text("%s", session.get_group_name().c_str());

                ImGui::TableSetColumnIndex(index++);
                ImGui::Text("%s", session.get_name().c_str());

                ImGui::TableSetColumnIndex(index++);
                ImGui::Text("%s", Timer::get_formatted_duration(session.get_duration_work()).c_str());

                ImGui::TableSetColumnIndex(index++);
                ImGui::Text("%s", Timer::get_formatted_duration(session.get_duration_pause()).c_str());

                ImGui::TableSetColumnIndex(index++);
                ImGui::Text("%s", Timer::get_formatted_duration(session.get_duration_total()).c_str());

                ImGui::TableSetColumnIndex(index++);
                ImGui::Text("%.1f%%", session.get_efficiency());

                ImGui::TableSetColumnIndex(index++);
                std::string formatted_datetime =
                        session.get_timestamp().get_formatted_date() + " " + session.get_timestamp().
                        get_formatted_time();
                ImGui::Text("%s", formatted_datetime.c_str());
            }
            ImGui::EndTable();
        }
        ImGui::PopStyleVar();

        if (ctx.open_delete_popup)
            ImGui::OpenPopup("Delete Session?");

        if (ImGui::IsPopupOpen("Delete Session?")) {
            float popup_width = display_size.x * 0.5f;
            if (popup_width < 350.0f)
                popup_width = 350.0f;

            ImVec2 popup_pos = ImVec2((display_size.x - popup_width) * 0.5f, display_size.y * 0.4f);
            ImGui::SetNextWindowPos(popup_pos, ImGuiCond_Always);
            ImGui::SetNextWindowSize(ImVec2(popup_width, 0.0f), ImGuiCond_Always);
        }

        if (ImGui::BeginPopupModal("Delete Session?", NULL)) {
            ImGui::SetWindowFontScale(text_scale * 1.2f);
            ImGui::Text("Are you sure you want to delete session ID %d?", ctx.session_id_to_delete);
            ImGui::Separator();

            float button_width = (ImGui::GetContentRegionAvail().x - ImGui::GetStyle().ItemSpacing.x) * 0.5f;

            if (ImGui::Button("[Enter] Yes", ImVec2(button_width, 0)) || ImGui::IsKeyPressed(ImGuiKey_Enter)) {
                if (db.delete_session(ctx.session_id_to_delete)) {
                    // Comprehensive state invalidation to synchronize all UI components.
                    db.update_top_groups(db, ctx);
                    ctx.last_session = db.get_last_session();
                    ctx.groups_loaded = false;
                    ctx.history_sessions = db.get_all_sessions();
                    ctx.available_groups = db.get_unique_groups();

                    if (strlen(ctx.search_buffer) > 0) {
                        ctx.filtered_sessions = db.search_sessions(ctx.search_buffer, ctx.search_error);
                    }
                    ctx.add_log(
                        "DB_QUERY: Executed PQL DELETE on Record ID: " + std::to_string(ctx.session_id_to_delete));
                }
                ctx.open_delete_popup = false;
                ImGui::CloseCurrentPopup();
                ImGui::GetIO().ClearInputKeys();
            }
            ImGui::SameLine();

            if (ImGui::Button("[Esc] No", ImVec2(button_width, 0)) || ImGui::IsKeyPressed(ImGuiKey_Escape)) {
                ctx.open_delete_popup = false;
                ImGui::CloseCurrentPopup();
                ImGui::GetIO().ClearInputKeys();
            }
            ImGui::EndPopup();
        }
    }
    ImGui::End();
}

void RenderExitModal(AppContext &ctx) {
    // Await global trigger flag to intercept the application kill signal.
    if (ctx.show_exit_popup) {
        ImGui::OpenPopup("Exit Application?");
    }

    if (ImGui::BeginPopupModal("Exit Application?", NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImVec2 center = ImGui::GetMainViewport()->GetCenter();
        ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));

        float app_width = ImGui::GetMainViewport()->Size.x;
        float text_scale = app_width / 800.0f;
        if (text_scale < 0.8f)
            text_scale = 0.8f;
        if (text_scale > 2.0f)
            text_scale = 2.0f;
        ImGui::SetWindowFontScale(text_scale);

        ImGui::Spacing();
        ImGui::Text("Are you sure you want to close the application?");
        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        if (ImGui::IsKeyPressed(ImGuiKey_Enter)) {
            ctx.should_close = true; // Signal the main GLFW loop to terminate.
        }

        if (ImGui::IsKeyPressed(ImGuiKey_Escape)) {
            ImGui::CloseCurrentPopup();
            ctx.show_exit_popup = false;
        }

        if (ImGui::Button("Yes (Enter)", ImVec2(140 * text_scale, 0))) {
            ctx.should_close = true;
        }

        ImGui::SameLine();

        if (ImGui::Button("No (Esc)", ImVec2(140 * text_scale, 0))) {
            ImGui::CloseCurrentPopup();
            ctx.show_exit_popup = false;
        }

        ImGui::EndPopup();
    }
}
