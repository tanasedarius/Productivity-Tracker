/**
 * @file main.cpp
 * @brief Entry point of the Productivity Tracker application.
 *
 * This module bootstraps the core engine systems (GLFW, OpenGL, Dear ImGui, SQLite),
 * manages the main application lifecycle/render loop, and ensures graceful teardown
 * upon exit. All localized UI rendering is delegated to the isolated UI module.
 */

#include <iostream>
#include <filesystem>
#include <GLFW/glfw3.h>
#include "imgui_internal.h"
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

#include "database_handler.h"
#include "ui.h"
#include "enums.h"

/**
 * @brief Main application entry point.
 *
 * @return int Exit code (0 on success, non-zero on critical system failure).
 */
int main()
{
    // 1. Initialize Persistent Storage
    // The database connection is established first as the UI relies entirely on its state.
    std::filesystem::create_directories("data");
    DatabaseHandler db("data/productivity.db");
    if (!db.open())
        return 1;
    db.create_table();

    // 2. Initialize GLFW & OpenGL Context
    if (!glfwInit())
        return 1;

    // Force OpenGL 3.0 configuration and enable 4x MSAA for smoother geometry rendering.
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
    glfwWindowHint(GLFW_SAMPLES, 4);

    GLFWwindow *window = glfwCreateWindow(1920, 1080, APP_NAME " v" APP_VERSION, nullptr, nullptr);
    if (!window)
    {
        glfwTerminate();
        return 1;
    }

    glfwMakeContextCurrent(window);
    glfwSwapInterval(1); // Enable V-Sync to prevent screen tearing and reduce CPU/GPU load.

    // 3. Initialize Dear ImGui Subsystem
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO &io = ImGui::GetIO();
    
    // Persist ImGui window layout inside the data folder
    io.IniFilename = "data/imgui.ini";

    // Load primary retro typography.
    // Note: Application will fallback to the default embedded font if the file is missing from the working directory.
    io.Fonts->AddFontFromFileTTF("assets/PressStart2P-Regular.ttf", 10.0f);

    ImGui::StyleColorsDark();
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 130");

    // 4. Instantiate Global Application State
    // AppContext acts as the centralized data hub, passed by reference to all modular UI functions.
    AppContext app_ctx;
    app_ctx.add_log("SYS_EVENT: Workspace initialized. Main rendering loop started.");
    app_ctx.load_logs();
    db.update_top_groups(db, app_ctx);
    app_ctx.last_session = db.get_last_session();

    // 5. Main Render Loop
    while (!glfwWindowShouldClose(window) && !app_ctx.should_close)
    {
        glfwPollEvents();

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        // Global Input Interceptor:
        // Triggers the exit modal if Escape is pressed and no other overlay (Dashboard/History/Guide) is actively consuming it.
        if (ImGui::IsKeyPressed(ImGuiKey_Escape) && !app_ctx.show_exit_popup && !app_ctx.show_dashbord && !app_ctx.show_history_window && !app_ctx.show_pql_guide && !app_ctx.show_info_window)
        {
            app_ctx.show_exit_popup = true;
            ImGui::GetIO().ClearInputKeys();
        }

        // Orchestrate UI layout components.
        RenderBackground(app_ctx, db);
        RenderExitModal(app_ctx);

        if (app_ctx.show_dashbord)
        {
            RenderDashboard(app_ctx, db);
        }

        // Handle History window state and clear the PQL search buffer immediately when the window is closed.
        if (app_ctx.show_history_window)
        {
            app_ctx.clear_search_buffer = true;
            RenderHistoryWindow(app_ctx, db);
        }
        else if (app_ctx.clear_search_buffer)
        {
            app_ctx.clear_search_buffer = false;
            app_ctx.search_buffer[0] = '\0';
        }

        // Execute draw commands and composite the frame.
        ImGui::Render();
        int display_w, display_h;
        glfwGetFramebufferSize(window, &display_w, &display_h);
        glViewport(0, 0, display_w, display_h);

        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glfwSwapBuffers(window);
    }

    app_ctx.add_log("SYS_EVENT: Termination signal received. Shutting down workspace...");

    // 6. Graceful Subsystem Teardown
    // Order is critical to prevent memory access violations during OpenGL context destruction.
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glfwDestroyWindow(window);
    glfwTerminate();
    db.close();

    return 0;
}