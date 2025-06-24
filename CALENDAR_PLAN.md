# Plan: Integrate Calendar Application into EinkPDA V2

**Goal:** Integrate a Calendar application (`CALENDAR.ino`) into the existing ESP32 E-ink PDA firmware (`Code/V2/`).

**Key Features:**

*   Display a monthly calendar view on the e-ink screen.
*   Navigate between months using FN + Arrow keys.
*   Navigate between days within the current month using Arrow keys.
*   Highlight the selected day using inverse video.
*   Outline the current actual day.
*   Display indicators on days that have associated tasks (from `/tasks.txt`, assuming `YYYY-MM-DD` format).
*   Launch the Calendar app from the HOME screen.
*   Ensure the `/tasks.txt` file exists upon startup.

**Prerequisites:**

1.  A 40x40 monochrome bitmap icon named `calendarIcon` needs to be defined in `Code/V2/assets.h` (PROGMEM format). The current plan uses a placeholder icon.

**Implementation Steps:**

1.  **Create `Code/V2/CALENDAR.ino`:** Implement core calendar logic, keyboard handling (`processKB_CALENDAR`), and e-ink drawing (`einkHandler_CALENDAR`) including day/month navigation, highlighting, and task indicators.
2.  **Modify `Code/V2/TASKS.ino`:** Implement `void ensureTasksFileExists()`: Checks if `/tasks.txt` exists in SPIFFS and creates an empty one if not. Call this function from `updateTaskArray()` and `updateTasksFile()`.
3.  **Modify `Code/V2/einkPDA_V2.ino`:**
    *   Add `CALENDAR` to the `AppState` enum definition.
    *   Add `"calendar"` to the `appStateNames` array.
    *   Add `calendarIcon` (or placeholder) to the `appIcons` array.
    *   In `applicationEinkHandler()`, add `case CALENDAR:` to call `einkHandler_CALENDAR()`.
    *   In `processKB()`, add `case CALENDAR:` to call `processKB_CALENDAR()`.
    *   Add global declarations for `displayedYear`, `displayedMonth`, `selectedDay`.
    *   In `setup()` (after RTC init): Initialize `displayedYear`, `displayedMonth`, `selectedDay` using `rtc.now()`. Set `CurrentCalendarState = CAL_MONTH_VIEW;`.
    *   In `setup()` (after `SPIFFS.begin()`): Call `ensureTasksFileExists();`.
4.  **Modify `Code/V2/HOME.ino`:**
    *   In `commandSelect(String command)`, add an `else if` block for commands like `"calendar"`, `"cal"`, `"date"`, `"6"`:
        *   Set `CurrentAppState = CALENDAR`.
        *   Initialize `displayedYear`, `displayedMonth`, `selectedDay` from `rtc.now()`.
        *   Set `CurrentCalendarState = CAL_MONTH_VIEW`.
        *   Set `newState = true`.

**Visual Plan (Simplified Flow):**

```mermaid
graph TD
    A[HOME Screen] -- "calendar" command --> B{Initialize Calendar};
    B -- Set CurrentAppState=CALENDAR --> C[Calendar App];
    C -- Key Input --> D{processKB_CALENDAR};
    D -- Month Nav (FN+Arrows) --> E{Update displayedMonth/Year};
    D -- Day Nav (Arrows) --> F{Update selectedDay};
    D -- Exit Key --> A;
    E -- Set newState=true --> G{einkHandler_CALENDAR};
    F -- Set newState=true --> G;
    G -- Draws UI --> H[E-ink Display];
    G -- Reads Tasks --> I[/tasks.txt];
    J[setup()] -- Calls --> K{ensureTasksFileExists};
    K -- Creates if needed --> I;

    style I fill:#f9f,stroke:#333,stroke-width:2px