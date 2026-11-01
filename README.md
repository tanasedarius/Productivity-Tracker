# Productivity Tracker

A desktop productivity timer written in modern C++20. Track focused work sessions,
measure your efficiency (work vs. pause time), organize activities into groups, and
search your full history with a small custom query language (PQL).

The UI is built with [Dear ImGui](https://github.com/ocornut/imgui) on an OpenGL 3 /
GLFW backend, styled with a retro pixel-font terminal aesthetic. All data is stored
locally in a SQLite database — no network, no accounts, no telemetry.

> Built and tested on Linux (Arch). The CMake build is scaffolded for Windows and
> macOS as well; cross-platform builds are planned but not yet verified. See
> [Roadmap](#roadmap).

<!-- A screenshot or short GIF of the UI goes a long way here. Add one when you can:
![screenshot](docs/screenshot.png) -->

## Features

- **Session timer** with pause/resume. A high-resolution stopwatch accumulates work
  time across multiple pause/resume cycles (`Timer`, based on `std::chrono`).
- **Efficiency metric** — each session automatically computes work time as a
  percentage of total time (work + pause).
- **Activity groups** — organize sessions by group, with a live "Top Groups" panel
  summarizing total time, session counts, and average efficiency.
- **Persistent history** — every session is saved to SQLite and browsable in a
  history window.
- **PQL — Productivity Query Language** — a compact `key:value` search syntax for
  filtering history (see below).
- **Auxiliary widgets** — a to-do list and a scratchpad notepad, each persisted to
  disk, plus an in-app system log terminal.

## PQL — Productivity Query Language

PQL is a small query language that compiles down to a parameterized SQL `WHERE`
clause. String values are bound as parameters (not string-concatenated), so queries
are safe against SQL injection.

Examples:

```
group:study                 # all sessions in the "study" group
date:last7d                 # sessions from the last 7 days
work:>2h                    # sessions with more than 2 hours of work time
group:study work:>1h30m     # combine filters
work:>10m&<2h               # range query on a single field
date:26-09-2026             # absolute date (DD-MM-YYYY)
```

Supported pieces:
- **Operators:** `>`, `<`, `>=`, `<=`, `=` (defaults to `=`), and `&` to chain
  conditions on one field.
- **Durations:** `1h30m`, `45m`, `120s` — converted to seconds for numeric comparison.
- **Dates:** relative (`last7d`, `last2h`) or absolute (`DD-MM-YYYY`).

## Tech stack

| Area        | Choice                                   |
|-------------|------------------------------------------|
| Language    | C++20                                    |
| Build       | CMake (>= 3.16)                          |
| UI          | Dear ImGui (vendored in `extern/imgui/`) |
| Rendering   | OpenGL 3 + GLFW 3                         |
| Storage     | SQLite 3                                  |

## Building (Linux)

Requires a C++20 compiler, CMake, and the GLFW and SQLite development packages.
If GLFW or SQLite aren't found on the system, CMake falls back to fetching them
automatically (`FetchContent`).

On Arch Linux:

```bash
sudo pacman -S --needed base-devel cmake glfw sqlite
```

Then configure and build:

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
````

The executable is produced at `build/productivity_tracker`.

## Running

The app loads its font from `assets/` and stores its database and config in `data/`,
using paths **relative to the current working directory**. Run it from the project
root so those folders are found:

```bash
cd /path/to/TimeTracker
./build/productivity_tracker
```

Tip: to launch from anywhere, make a shell alias that changes into the project root
first:

```bash
alias tracker='(cd /path/to/TimeTracker && ./build/productivity_tracker)'
```

Press `Esc` to bring up the exit confirmation.

## Project layout

```
src/                 Application source
  main.cpp           Entry point, window + render loop lifecycle
  ui.cpp / ui.h      All ImGui panels and windows
  app_context.h      Central application state (AppContext) + logging
  database_handler.* SQLite access layer
  session.*          Session model (work/pause/efficiency)
  timer.*            Pause/resume stopwatch
  timestamp.*        Date/time helper
  query_parser.*     PQL -> parameterized SQL WHERE clause
  enums.h            Application state enum
extern/imgui/        Vendored Dear ImGui + GLFW/OpenGL backends
assets/              Fonts (PressStart2P)
data/                Local runtime data (DB, logs, notes) — gitignored
```

## Design notes

A few decisions worth calling out:

- **Centralized state (`AppContext`).** Rather than scattering UI/session state as
  locals and statics in `main()`, everything lives in one struct passed by reference
  to each render function. This keeps the render loop thin and makes the UI functions
  easy to reason about and test in isolation.

- **PQL compiles to *parameterized* SQL.** The parser separates structure (operators,
  columns, date math) from user-supplied string values. Strings are returned as bind
  parameters and passed to SQLite via `?` placeholders, so a query like
  `group:'; DROP TABLE sessions; --` is treated as data, not SQL.

- **Efficiency is derived at construction.** The `Session` model computes total time
  and efficiency from work/pause inputs, keeping the derived numbers consistent
  everywhere they're read.

- **Dependencies degrade gracefully.** GLFW and SQLite are used from the system when
  present, and fetched/built from source when not — so a fresh machine can build
  without manual dependency setup.

## Roadmap

- [ ] Verified Windows build (MSVC).
- [ ] Verified macOS build.
- [ ] Resolve asset/data paths relative to the executable so the app can run from
      any working directory (and as a double-clicked binary).
- [ ] Packaging / installable artifacts per platform.

## License

See [LICENSE](LICENSE). This project vendors and links third-party libraries
(Dear ImGui, GLFW, SQLite), each under its own license.
