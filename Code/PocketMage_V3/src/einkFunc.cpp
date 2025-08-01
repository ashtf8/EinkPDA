//  oooooooooooo         ooooo ooooo      ooo oooo    oooo  //
//  `888'     `8         `888' `888b.     `8' `888   .8P'   //
//   888                  888   8 `88b.    8   888  d8'     //
//   888oooo8    8888888  888   8   `88b.  8   88888[       //
//   888    "             888   8     `88b.8   888`88b.     //
//   888       o          888   8       `888   888  `88b.   //
//  o888ooooood8         o888o o8o        `8  o888o  o888o  //
#include "globals.h"

void refresh() {
  // USE A SLOW FULL UPDATE EVERY N FAST UPDATES OR WHEN SPECIFIED
  if ((partialCounter >= FULL_REFRESH_AFTER) || forceSlowFullUpdate) {
    forceSlowFullUpdate = false;
    partialCounter = 0;
    setFastFullRefresh(false);
  }
  // OTHERWISE USE A FAST FULL UPDATE
  else {
    setFastFullRefresh(true);
    partialCounter++;
  }

  display.display(false);

  display.setFullWindow();
  display.fillScreen(GxEPD_WHITE);
  display.hibernate();
}

void multiPassRefesh(int passes) {
  display.display(false);
  if (passes > 0) {
    for (int i = 0; i < passes; i++) {
      delay(250);
      display.display(true);
    }
  }
  
  delay(100);
  display.setFullWindow();
  display.fillScreen(GxEPD_WHITE);
  display.hibernate();
}

void setFastFullRefresh(bool setting) {
  GxEPD2_310_GDEQ031T10::useFastFullUpdate = setting;
  /*if (GxEPD2_310_GDEQ031T10::useFastFullUpdate != setting) {
    GxEPD2_310_GDEQ031T10::useFastFullUpdate = setting;
  }*/
}

void einkHandler(void* parameter) {
  delay(250);
  /*display.setFullWindow();
  display.fillScreen(GxEPD_WHITE);
  display.display(false);
  display.hibernate();*/

  while (true) {
    applicationEinkHandler();

    vTaskDelay(50 / portTICK_PERIOD_MS);
    yield();
  }
}

void statusBar(String input, bool fullWindow) {
  display.setFont(&FreeMonoBold9pt7b);
  if (!fullWindow) display.setPartialWindow(0, display.height() - 20, display.width(), 20);
  display.fillRect(0, display.height() - 26, display.width(), 26, GxEPD_WHITE);
  display.drawRect(0, display.height() - 20, display.width(), 20, GxEPD_BLACK);
  display.setCursor(4, display.height() - 6);
  display.print(input);

  /*switch (CurrentKBState) {
    case NORMAL:
      //Display battery level
      display.drawBitmap(display.width()-30,display.height()-20, KBStatusallArray[6], 30, 20, GxEPD_BLACK);
      break;
    case SHIFT:
      display.drawBitmap(display.width()-30,display.height()-20, KBStatusallArray[0], 30, 20, GxEPD_BLACK);
      break;
    case FUNC:
      display.drawBitmap(display.width()-30,display.height()-20, KBStatusallArray[1], 30, 20, GxEPD_BLACK);
      break;
  }*/
  display.drawRect(display.width() - 30, display.height() - 20, 30, 20, GxEPD_BLACK);
}

void drawStatusBar(String input) {
  display.fillRect(0, display.height() - 26, display.width(), 26, GxEPD_WHITE);
  display.drawRect(0, display.height() - 20, display.width(), 20, GxEPD_BLACK);
  display.setFont(&FreeMonoBold9pt7b);
  display.setCursor(4, display.height() - 6);
  display.print(input);

  // BATTERY INDICATOR
  //display.drawBitmap(display.width() - 30, display.height() - 20, KBStatusallArray[battState], 30, 20, GxEPD_BLACK);
  //display.drawRect(display.width() - 30, display.height() - 20, 30, 20, GxEPD_BLACK);
}

uint8_t getMaxCharsPerLine() {
  int16_t x1, y1;
  uint16_t charWidth, charHeight;
  // GET AVERAGE CHAR WIDTH
  display.getTextBounds("abcdefghijklmnopqrstuvwxyz", 0, 0, &x1, &y1, &charWidth, &charHeight);
  charWidth = charWidth / 52;

  return (display.width() / (charWidth));
}

uint8_t getMaxLines() {
  int16_t x1, y1;
  uint16_t charWidth, charHeight;
  // GET MAX CHAR HEIGHT
  display.getTextBounds("H", 0, 0, &x1, &y1, &charWidth, &charHeight);
  fontHeight = charHeight;

  return ((display.height() - 26) / (charHeight + lineSpacing));
}

void setTXTFont(const GFXfont* font) {
  // SET THE FONT
  display.setFont(font);
  currentFont = (GFXfont*)font;

  // UPDATE maxCharsPerLine & maxLines
  maxCharsPerLine = getMaxCharsPerLine();
  maxLines = getMaxLines();
}

void drawThickLine(int x0, int y0, int x1, int y1, int thickness) {
  float dx = x1 - x0;
  float dy = y1 - y0;
  float length = sqrt(dx * dx + dy * dy);
  float stepX = dx / length;
  float stepY = dy / length;

  for (float i = 0; i <= length; i += thickness / 2.0) {
    int cx = round(x0 + i * stepX);
    int cy = round(y0 + i * stepY);
    display.fillCircle(cx, cy, thickness / 2, GxEPD_BLACK);
  }
}

void einkTextPartial(String text, bool noRefresh) {
  bool doFullRefresh = false;

  einkRefresh++;
  if (einkRefresh > FULL_REFRESH_AFTER) {
    doFullRefresh = true;
    einkRefresh = 0;
    display.setFullWindow();
    display.fillScreen(GxEPD_WHITE);
  }

  display.setFont(&FreeMonoBold9pt7b);

  if (splitIntoLines(text.c_str(), scroll)) doFullRefresh = true;

  for (int i = 0; i < 13; i++) {
    if (outLines[i] != "") {  // Print only non-empty lines
      if (doFullRefresh) {
        display.fillRect(0, 16 * i, display.width(), 16, GxEPD_WHITE);
        display.setCursor(0, 10 + (16 * i));
        display.print(outLines[i]);
      } else if (outLines[i] != lines_prev[i]) {  //If the line has changed
        display.setPartialWindow(0, 16 * i, display.width(), 16);
        display.fillRect(0, 16 * i, display.width(), 16, GxEPD_WHITE);
        display.setCursor(0, 10 + (16 * i));
        display.print(outLines[i]);
        if (!noRefresh) refresh();
      }
    }
  }

  if (doFullRefresh && !noRefresh) {
    display.nextPage();
    display.hibernate();
  }

  for (int i = 0; i < 13; i++) {
    lines_prev[i] = outLines[i];  // Copy each line
  }
}

void einkTextDynamic(bool doFull_, bool noRefresh) {
  // SET FONT
  setTXTFont(currentFont);

  // ITERATE AND DISPLAY
  uint8_t size = allLines.size();
  uint8_t displayLines = maxLines;

  if (displayLines > size) displayLines = size;  // PREVENT OUT OF BOUNDS

  // Apply dynamic scroll offset (make sure it's within the bounds)
  int scrollOffset = dynamicScroll;
  if (scrollOffset < 0) scrollOffset = 0;
  if (scrollOffset > size - displayLines) scrollOffset = size - displayLines;

  // FULL REFRESH OPERATION
  if (doFull_) {
    display.fillScreen(GxEPD_WHITE);
    for (uint8_t i = size - displayLines - scrollOffset; i < size - scrollOffset; i++) {
      if ((allLines[i]).length() > 0) {
        display.setFullWindow();
        //display.fillRect(0, (fontHeight + lineSpacing) * (i - (size - displayLines - scrollOffset)), display.width(), (fontHeight + lineSpacing), GxEPD_WHITE);
        display.setCursor(0, fontHeight + ((fontHeight + lineSpacing) * (i - (size - displayLines - scrollOffset))));
        display.print(allLines[i]);
        Serial.println(allLines[i]);
      }
    }
  }
  // PARTIAL REFRESH, ONLY SEND LAST LINE
  else {
    if ((allLines[size - displayLines - scrollOffset]).length() > 0) {
      display.setPartialWindow(0, (fontHeight + lineSpacing) * (size - displayLines - scrollOffset), display.width(), (fontHeight + lineSpacing));
      display.fillRect(0, (fontHeight + lineSpacing) * (size - displayLines - scrollOffset), display.width(), (fontHeight + lineSpacing), GxEPD_WHITE);
      display.setCursor(0, fontHeight + ((fontHeight + lineSpacing) * (size - displayLines - scrollOffset)));
      display.print(allLines[size - displayLines - scrollOffset]);
    }
  }

  drawStatusBar("L:" + String(allLines.size()) + " " + editingFile);
}

int countLines(String input, size_t maxLineLength) {
  size_t inputLength = input.length();
  uint8_t charCounter = 0;
  uint16_t lineCounter = 1;

  for (size_t c = 0; c < inputLength; c++) {
    if (input[c] == '\n') {
      charCounter = 0;
      lineCounter++;
      continue;
    } else if (charCounter > (maxLineLength - 1)) {
      charCounter = 0;
      lineCounter++;
    }
    charCounter++;
  }

  return lineCounter;
}

///////////////////////////// CALC EINK FUNCTIONS
// CALC FRAME
void drawCalc(){
  // SET FONT
  setTXTFont(currentFont);
  display.setFullWindow();
  display.firstPage();
  do {
    // print status bar
    if (CurrentCALCState == CALC4){
      drawStatusBar("<- -> scroll | enter -> EXIT");
    } else {
      drawStatusBar("\"/4\" -> info | \"/6\" -> EXIT");
    }
    // draw calc bitmap
    display.drawBitmap(0, 0, calcAllArray[0], 320, 218, GxEPD_BLACK);
    // print current calc mode
    display.setCursor(25, 20);
    switch (CurrentCALCState) {
      case CALC0:      
        //standard mode
        display.print("Calc: Standard");
        break;
      case CALC1:
        //programming mode
        display.print("Calc: Programming");
        break;
      case CALC2:
        //scientific mode
        display.print("Calc: Scientific");
        break;
      case CALC3:
        //conversions
        display.print("Calc: Conversions");
        break;
      case CALC4:
        //help mode
        display.print("Calc: Help");
        break;  
    }
    // print current trig mode
    display.setCursor(240, 20);
    switch (trigType){
      // 0 = degree mode
      case (0):
        display.print("deg");
      break;
      // 1 = radian mode
      case (1):
        display.print("rad");
      break;
      // 2 = grad mode
      case (2):
        display.print("grad");
      break;
    }
  } while (display.nextPage());

}

// CALC EINK TEXT 
// partial refresh that doesn't write over the calc app bitmap
void einkCalcDynamic(bool doFull_, bool noRefresh) {
  const int reservedTop = 32;
  const int leftMargin = 8;
  const int rightMargin = 20;
  const int maxTextWidth = display.width() - leftMargin - rightMargin;
  setTXTFont(currentFont);
  std::vector<String> wrappedLines;
  for (const String& originalLine : allLinesCalc) {
    if (originalLine.length() == 0) {
      wrappedLines.push_back("");
      continue;
    }   
    String currentLine = "";
    String currentWord = "";
    // Check if line needs wrapping
    int16_t x1, y1;
    uint16_t lineWidth, lineHeight;
    bool rightAlign = originalLine.startsWith("~R~");
    // clip right align marker
    String line = rightAlign ? originalLine.substring(3) : originalLine; 
    display.getTextBounds(line, 0, 0, &x1, &y1, &lineWidth, &lineHeight);
    if (lineWidth <= maxTextWidth) {
      wrappedLines.push_back(originalLine);
      continue;
    }
    for (int i = 0; i < line.length(); i++) {
      char c = line[i];
      currentWord += c;
      // Check word boundaries (space or end of string)
      if (c == ' ' || i == line.length() - 1) {
        String testLine = currentLine + currentWord;
        display.getTextBounds(testLine, 0, 0, &x1, &y1, &lineWidth, &lineHeight);
        if (lineWidth > maxTextWidth) {
          if (currentLine.length() > 0) {
            // remove marker for placing on individual wrapped lines
            wrappedLines.push_back(rightAlign ? "~R~" + currentLine : currentLine);
            currentLine = "";
          }
          // Handle very long words
          while (currentWord.length() > 0) {
            int splitPos = 1;
            String testFragment = "";
            while (splitPos < currentWord.length()) {
              testFragment += currentWord[splitPos];
              display.getTextBounds(testFragment, 0, 0, &x1, &y1, &lineWidth, &lineHeight);
              if (lineWidth > maxTextWidth) break;
              splitPos++;
            }
            wrappedLines.push_back(rightAlign ? "~R~" + currentWord.substring(0, splitPos) : currentWord.substring(0, splitPos));
            currentWord = currentWord.substring(splitPos);
          }
        } else {
          currentLine = testLine;
        }
        currentWord = "";
      }
    }
    if (currentLine.length() > 0) {
      wrappedLines.push_back(rightAlign ? "~R~" + currentLine : currentLine);
    }
  }
  // wrap lines
  uint16_t size = wrappedLines.size();
  uint16_t displayLines = constrain(maxLines - 2, 0, size);
  int scrollOffset = constrain(dynamicScroll, 0, size - displayLines);
  int startLine = size - displayLines - scrollOffset;
  int endLine = size - scrollOffset;
  int totalTextHeight = (fontHeight + lineSpacing) * displayLines;
  totalTextHeight -= totalTextHeight % 8;
  if (doFull_) {
    display.setPartialWindow(leftMargin, reservedTop, maxTextWidth, totalTextHeight);
    display.firstPage();
    do {
      display.fillRect(leftMargin, reservedTop, maxTextWidth, totalTextHeight, GxEPD_WHITE);
      for (int i = startLine; i < endLine; i++) {
        if (wrappedLines[i].length() == 0) continue;
        int lineIndex = i - startLine;
        int y = reservedTop + (fontHeight + lineSpacing) * lineIndex;
        y -= y % 8;
        String line = wrappedLines[i];
        bool rightAlign = line.startsWith("~R~");
        if (rightAlign) line.remove(0, 3);
        int16_t x1, y1;
        uint16_t lineWidth, lineHeight;
        display.getTextBounds(line, 0, 0, &x1, &y1, &lineWidth, &lineHeight);
        const int paddingFix = 2;
        int xPos = rightAlign ? (display.width() - rightMargin - lineWidth - paddingFix) : leftMargin;
        display.setCursor(xPos, y + fontHeight);
        display.print(line);
      }
    } while (display.nextPage());
  } else {
    int i = size - displayLines - scrollOffset;
    if (i >= 0 && allLinesCalc[i].length() > 0) {
      int h = fontHeight + lineSpacing;
      h -= h % 8;
      int y = reservedTop;
      y -= y % 8;
      display.setPartialWindow(leftMargin, y, maxTextWidth, h);
      display.firstPage();
      do {
        display.fillRect(leftMargin, y, maxTextWidth, h, GxEPD_WHITE);
        String line = allLinesCalc[i];
        bool rightAlign = line.startsWith("~R~");
        if (rightAlign) line.remove(0, 3);  // Strip "~R~"
        int16_t x1, y1;
        uint16_t lineWidth, lineHeight;
        display.getTextBounds(line, 0, 0, &x1, &y1, &lineWidth, &lineHeight);
        int xPos = rightAlign ? (display.width() - rightMargin - lineWidth) : leftMargin;
        display.setCursor(xPos, y + fontHeight);
        display.print(line);
      } while (display.nextPage());
    }
  }
}

