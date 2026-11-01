/**
 * @file ui.h
 * @brief Declarations for the Productivity Tracker user interface.
 *
 * This header defines the structural components of the UI and declares
 * the rendering functions. It strictly decouples the Dear ImGui rendering
 * logic from the main application state and database logic.
 */

#pragma once

#include <string>
#include <vector>
#include "timer.h"
#include "session.h"
#include "app_context.h"

// Forward declaration to avoid including the full database header here,
// which significantly speeds up compilation times and prevents circular dependencies.
class DatabaseHandler;

struct GLFWwindow;

/**
 * @brief Renders a developer tool for direct database manipulation.
 *
 * Allows manual injection of session records into the SQLite database
 * for testing and debugging purposes.
 *
 * @param ctx Reference to the global application context.
 * @param db  Reference to the active database handler.
 */
#ifndef NDEBUG
void RenderDebugMenu(AppContext &ctx, DatabaseHandler &db);
#endif
/**
 * @brief Renders the left navigation sidebar.
 *
 * Contains quick-action buttons and keyboard shortcut toggles for
 * accessing core application modules (Dashboard, History, PQL Guide).
 *
 * @param ctx Reference to the global application context.
 * @param db  Reference to the active database handler.
 */
void RenderSidebar(AppContext &ctx, DatabaseHandler &db);

/**
 * @brief Renders the top status bar.
 *
 * Displays the current system date and time aligned to the right.
 *
 * @param ctx Reference to the global application context.
 */
void RenderTopBar(AppContext &ctx);

/**
 * @brief Renders the UI panel for the top 3 groups and their statistics.
 *
 * This function calculates the average efficiency and formats the total work 
 * duration for each high-performing project group, rendering the ranked statistical 
 * summary inside a dedicated UI panel.
 *
 * @param ctx Reference to the global application context.
 */
void RenderTopGroupsPanel(AppContext &ctx);

/**
 * @brief Displays detailed metrics of the most recently recorded session.
 *
 * Fetches the last entry from the database and centers its details (work time,
 * pause time, efficiency) within a dedicated dashboard panel.
 *
 * @param ctx Reference to the global application context.
 * @param db  Reference to the active database handler.
 */
void RenderLastActivityPanel(AppContext &ctx);

/**
 * @brief Displays aggregated statistics for the currently active project group.
 *
 * Calculates total work time, session counts, and time-of-day distribution
 * for the group associated with the most recent session.
 *
 * @param ctx Reference to the global application context.
 * @param db  Reference to the active database handler.
 */
void RenderGroupStatsPanel(AppContext &ctx, DatabaseHandler &db);

/**
 * @brief Renders a floating reference window for PQL (Productivity Query Language).
 *
 * Provides syntax documentation, available filters, and examples to assist
 * users in querying their session history.
 *
 * @param ctx Reference to the global application context.
 */
void RenderPQLGuideWindow(AppContext &ctx);

/**
 * @brief Renders a persistent quick-notes editor.
 *
 * A multiline text input widget that automatically flushes its contents
 * to a local file (`quick_notes.txt`) upon modification.
 *
 * @param ctx Reference to the global application context.
 */
void RenderNotepadPanel(AppContext &ctx);

/**
 * @brief Renders a GitHub-style activity heatmap for the last 90 days.
 *
 * Color-codes daily squares based on the total accumulated work time
 * and provides interactive tooltips with detailed daily breakdowns.
 *
 * @param ctx Reference to the global application context.
 * @param db  Reference to the active database handler.
 */
void RenderHeatmapPanel(AppContext &ctx, DatabaseHandler &db);

/**
 * @brief Renders an interactive To-Do list.
 *
 * Allows users to add, check off, and delete tasks. State is persistently
 * saved to a local configuration file (`todos.txt`).
 *
 * @param ctx Reference to the global application context.
 */
void RenderToDoPanel(AppContext &ctx);

/**
 * @brief Visualizes the passage of time using progress bars.
 *
 * Displays how much time has elapsed in the current day, week, month,
 * and year to promote time awareness and urgency.
 *
 * @param ctx Reference to the global application context.
 */
void RenderTimeAwarenessPanel();

/**
 * @brief Renders a retro-styled terminal outputting system events.
 *
 * Displays an auto-scrolling log of UI state mutations, database queries,
 * and user interactions, styled with semantic colors.
 *
 * @param ctx Reference to the global application context.
 */
void RenderActivityLogPanel(AppContext &ctx);

/**
 * @brief Orchestrates the entire workspace layout.
 *
 * This function acts as the root container, setting up a full-screen borderless
 * window and systematically rendering all other modular UI panels within it.
 *
 * @param ctx Reference to the global application context.
 * @param db  Reference to the active database handler.
 */
void RenderBackground(AppContext &ctx, DatabaseHandler &db);

/**
 * @brief Renders the core session tracking interface.
 *
 * Handles the inputs for starting new activities, selecting groups, and
 * manages the live work/pause timers before flushing data to the database.
 *
 * @param ctx Reference to the global application context.
 * @param db  Reference to the active database handler.
 */
void RenderDashboard(AppContext &ctx, DatabaseHandler &db);

/**
 * @brief Renders the session history explorer.
 *
 * Displays a sortable table of past sessions equipped with a PQL search bar
 * for advanced filtering, and handles session deletion requests.
 *
 * @param ctx Reference to the global application context.
 * @param db  Reference to the active database handler.
 */
void RenderHistoryWindow(AppContext &ctx, DatabaseHandler &db);

/**
 * @brief Triggers the exit confirmation modal logic (Legacy/Alternative wrapper).
 *
 * @param ctx Reference to the global application context.
 * @param window Pointer to the active GLFW window context.
 */
void RenderExitConfirmationPopup(AppContext &ctx, GLFWwindow *window);

/**
 * @brief Renders a centered modal asking for confirmation before exiting.
 *
 * Prevents accidental closure of the application, ensuring active timers
 * or unsaved states are explicitly acknowledged by the user.
 *
 * @param ctx Reference to the global application context.
 */
void RenderExitModal(AppContext &ctx);

/**
 * @brief Renders the application information window.
 *
 * Displays app metadata, creator info, widget descriptions, and keyboard shortcuts.
 *
 * @param ctx Reference to the global application context.
 */
void RenderInfoWindow(AppContext &ctx);