#include <RTClib.h> // Already included in main .ino, but good practice here too

// --- Calendar App Specific Variables ---
enum CalendarState {
  CAL_MONTH_VIEW
  // Add other states later if needed, e.g., CAL_DAY_VIEW, CAL_ADD_EVENT
};
CalendarState CurrentCalendarState = CAL_MONTH_VIEW;

// These are declared globally in einkPDA_V2.ino now, but keep comments for clarity
// int displayedYear;
// int displayedMonth;
// int selectedDay = 1; // Initialize selectedDay to 1

// --- Helper Functions ---

// Function to check if a year is a leap year
bool isLeapYear(int year) {
  return (((year % 4 == 0) && (year % 100 != 0)) || (year % 400 == 0));
}

// Function to get the number of days in a month
int daysInMonth(int year, int month) {
  if (month == 2) {
    return isLeapYear(year) ? 29 : 28;
  } else if (month == 4 || month == 6 || month == 9 || month == 11) {
    return 30;
  } else {
    return 31;
  }
}

// Function to get the day of the week (0=Sunday, 6=Saturday) for the first day of the month
// Using a simplified approach based on RTClib's capabilities
int getFirstDayOfMonthWeekday(int year, int month) {
    DateTime firstDay(year, month, 1, 0, 0, 0);
    return firstDay.dayOfTheWeek(); // RTClib dayOfTheWeek() returns 0 for Sunday, ..., 6 for Saturday
}

// --- Keyboard Processing ---
void processKB_CALENDAR() {
  if (OLEDPowerSave) {
    u8g2.setPowerSave(0);
    OLEDPowerSave = false;
  }
  disableTimeout = false; // Allow timeout unless an action requires focus

  int currentMillis = millis();
  if (currentMillis - KBBounceMillis >= KB_COOLDOWN) {
    char inchar = updateKeypress();
    bool dayChanged = false; // Flag to track if selection moved

    switch (CurrentCalendarState) {
      case CAL_MONTH_VIEW:
        // Exit/BKSP Received (Use code 12 like FILEWIZ)
        if (inchar == 12) {
          CurrentAppState = HOME;
          CurrentHOMEState = HOME_HOME;
          currentLine     = ""; // Clear any lingering command line
          CurrentKBState  = NORMAL;
          newState = true; // Trigger redraw of HOME
          break;
        }

        // Handle KB State Toggles First
        if (inchar == 17) { // SHIFT
           if (CurrentKBState == SHIFT) CurrentKBState = NORMAL;
           else CurrentKBState = SHIFT;
           // Potentially update OLED indicator if needed
           break; // Consume the key press
        } else if (inchar == 18) { // FN
           if (CurrentKBState == FUNC) CurrentKBState = NORMAL;
           else CurrentKBState = FUNC;
           // Potentially update OLED indicator if needed
           break; // Consume the key press
        }


        // Handle Navigation based on CurrentKBState
        if (CurrentKBState == FUNC) { // Month Navigation
            if (inchar == 19) { // FN + Left Arrow (Previous Month)
                displayedMonth--;
                if (displayedMonth < 1) {
                    displayedMonth = 12;
                    displayedYear--;
                }
                selectedDay = 1; // Reset selection to 1st on month change
                newState = true;
                CurrentKBState = NORMAL; // Reset KB state after combo
            } else if (inchar == 21) { // FN + Right Arrow (Next Month)
                displayedMonth++;
                if (displayedMonth > 12) {
                    displayedMonth = 1;
                    displayedYear++;
                }
                selectedDay = 1; // Reset selection to 1st on month change
                newState = true;
                CurrentKBState = NORMAL; // Reset KB state after combo
            }
            // Reset FN state if other keys pressed while FN active (unless it's a number if needed later)
            else if (inchar != 0 && !(inchar >= '0' && inchar <= '9')) {
                 CurrentKBState = NORMAL;
            }
        } else { // NORMAL or SHIFT state - Day Navigation
            int maxDays = daysInMonth(displayedYear, displayedMonth);
            if (inchar == 19) { // Left Arrow (Previous Day)
                selectedDay--;
                if (selectedDay < 1) {
                    // Find the last day of the *previous* month
                    int prevMonth = displayedMonth - 1;
                    int prevYear = displayedYear;
                    if (prevMonth < 1) {
                        prevMonth = 12;
                        prevYear--;
                    }
                    selectedDay = daysInMonth(prevYear, prevMonth); // Wrap around to last day of prev month
                }
                dayChanged = true;
            } else if (inchar == 21) { // Right Arrow (Next Day)
                selectedDay++;
                if (selectedDay > maxDays) {
                    selectedDay = 1; // Wrap around to first day of next month
                }
                dayChanged = true;
            }
            // Add other NORMAL state actions here later (e.g., Enter/Select key '20')
            // else if (inchar == 20) { /* Go to Day View or Show Events */ }

            // Reset Shift state if any other key is pressed
            if (CurrentKBState == SHIFT && inchar != 0) {
                CurrentKBState = NORMAL;
            }
        }

        // Trigger redraw only if the selection or view actually changed
        if (dayChanged) {
            newState = true;
        }

        break; // End of CAL_MONTH_VIEW case

        // Add cases for other states (CAL_DAY_VIEW, etc.) here later
    }

    // --- OLED Update ---
    currentMillis = millis();
    if (currentMillis - OLEDFPSMillis >= 16) {
      OLEDFPSMillis = currentMillis;
       String monthStr = String(displayedMonth);
       if (displayedMonth < 10) monthStr = "0" + monthStr;
       String dayStr = String(selectedDay); // Show selected day on OLED
       if (selectedDay < 10) dayStr = "0" + dayStr;
       String oledStr = dayStr + "/" + monthStr + "/" + String(displayedYear);
       oledLine(oledStr, false); // Show date, no progress bar needed here
    }
    KBBounceMillis = currentMillis; // Update bounce time
  }
}


// --- E-Ink Display Handler ---
void einkHandler_CALENDAR() {
  switch (CurrentCalendarState) {
    case CAL_MONTH_VIEW:
      if (newState) {
        newState = false; // Reset flag
        forceSlowFullUpdate = true; // Force a full refresh for calendar redraw

        display.setFullWindow();
        display.fillScreen(GxEPD_WHITE);
        display.setTextColor(GxEPD_BLACK);

        // --- Load Events/Tasks ---
        updateTaskArray(); // Refresh task data

        // --- Draw Calendar UI ---
        const char* monthNames[] = {"", "January", "February", "March", "April", "May", "June", "July", "August", "September", "October", "November", "December"};
        String header = String(monthNames[displayedMonth]) + " " + String(displayedYear);

        // Draw Header (Month Year)
        display.setFont(&FreeSansBold12pt7b);
        int16_t x1, y1;
        uint16_t w, h;
        display.getTextBounds(header, 0, 0, &x1, &y1, &w, &h);
        display.setCursor((display.width() - w) / 2, h + 5); // Center header
        display.print(header);

        // Draw Day Headers (S M T W T F S)
        display.setFont(&FreeMonoBold9pt7b);
        int dayHeaderY = h + 20;
        int dayCellWidth = display.width() / 7;
        const char* dayNames[] = {"S", "M", "T", "W", "T", "F", "S"};
         int dayNameTextHeight;
         display.getTextBounds("S", 0, 0, &x1, &y1, &w, &h); // Get height of day name text
         dayNameTextHeight = h;
        for (int i = 0; i < 7; i++) {
          display.getTextBounds(dayNames[i], 0, 0, &x1, &y1, &w, &h);
          display.setCursor(i * dayCellWidth + (dayCellWidth / 2) - (w/2), dayHeaderY); // Center day name
          display.print(dayNames[i]);
        }

        // Draw Calendar Grid
        int firstDayWeekday = getFirstDayOfMonthWeekday(displayedYear, displayedMonth);
        int days = daysInMonth(displayedYear, displayedMonth);
        int day = 1;
        int gridStartY = dayHeaderY + 5; // Start grid slightly lower
        int cellHeight = (display.height() - gridStartY - 25) / 6; // Adjust for status bar, leave room for 6 rows
        int cellPadding = 2; // Small padding within cells

        display.setFont(&FreeSans9pt7b); // Font for day numbers
        DateTime now = rtc.now(); // Get current date

        for (int row = 0; row < 6; row++) {
          for (int col = 0; col < 7; col++) {
             int cellX = col * dayCellWidth;
             int cellY = gridStartY + row * cellHeight;

             // --- Draw Cell Border (Optional) ---
             // display.drawRect(cellX, cellY, dayCellWidth, cellHeight, GxEPD_BLACK);

            if (!((row == 0 && col < firstDayWeekday) || day > days)) {
              // --- Draw day number ---
              String dayStr = String(day);
              display.getTextBounds(dayStr, 0, 0, &x1, &y1, &w, &h);
              int dayNumX = cellX + cellPadding + 1; // Position top-left
              int dayNumY = cellY + h + cellPadding; // Position top-left

              // --- Highlighting Logic ---
              bool isCurrentDay = (displayedYear == now.year() && displayedMonth == now.month() && day == now.day());
              bool isSelectedDay = (day == selectedDay);

              if (isSelectedDay) {
                 // Inverse video for selected day
                 display.fillRect(cellX + 1, cellY + 1, dayCellWidth - 2, cellHeight - 2, GxEPD_BLACK); // Fill cell black (leave border)
                 display.setTextColor(GxEPD_WHITE); // Set text to white
                 display.setCursor(dayNumX, dayNumY);
                 display.print(dayStr);
                 display.setTextColor(GxEPD_BLACK); // Reset text color for others
              } else {
                 // Normal drawing
                 display.setCursor(dayNumX, dayNumY);
                 display.print(dayStr);
                 // Simple box for current day (if not selected)
                 if (isCurrentDay) {
                    // Draw rectangle slightly larger than text bounds for better visual
                    display.drawRect(dayNumX - 2, dayNumY - h - 1, w + 4 , h + 4, GxEPD_BLACK); // Box around number
                 }
              }


               // --- Check for Events/Tasks on this day ---
              String currentDayFormatted = String(displayedYear) + "-";
              if (displayedMonth < 10) currentDayFormatted += "0";
              currentDayFormatted += String(displayedMonth) + "-";
              if (day < 10) currentDayFormatted += "0";
              currentDayFormatted += String(day);

              bool hasEvent = false;
              for (const auto& task : tasks) {
                  // Assuming task[1] is the dueDate in YYYY-MM-DD format
                  if (task.size() > 1 && task[1] == currentDayFormatted) {
                      hasEvent = true;
                      break;
                  }
              }

              if (hasEvent) {
                 // Draw an indicator (e.g., small filled circle bottom right)
                 int indicatorX = cellX + dayCellWidth - cellPadding - 5; // Bottom-right
                 int indicatorY = cellY + cellHeight - cellPadding - 5;  // Bottom-right
                 // Adjust color based on selection
                 uint16_t indicatorColor = isSelectedDay ? GxEPD_WHITE : GxEPD_BLACK;
                 display.fillCircle(indicatorX, indicatorY, 2, indicatorColor); // Draw filled circle indicator
                 // Use fillRect if you prefer a square: display.fillRect(indicatorX, indicatorY, 4, 4, indicatorColor);
              }

              day++; // Increment day
            }
          }
        }

        // Draw Status Bar (using existing function)
        drawStatusBar("Calendar (FN+<> Month | <> Day)");

        // Refresh Display
        refresh();
      }
      break;

      // Add cases for other states here later
  }
}