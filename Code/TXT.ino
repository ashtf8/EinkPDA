enum TXTState {TXT_, WIZ0, WIZ1, WIZ2, WIZ3};
TXTState CurrentTXTState = TXT_;

void processKB_TXT() {
  int currentMillis = millis();
  //Make sure oled only updates at 60fps
  if (currentMillis - KBBounceMillis >= KB_COOLDOWN) {  
    char inchar = updateKeypress();
    switch (CurrentTXTState) {
      case TXT_:
        //No char recieved
        if (inchar == 0);                                         
        //SHIFT Recieved
        else if (inchar == 17) {                                  
          if (CurrentKBState == SHIFT) CurrentKBState = NORMAL;
          else CurrentKBState = SHIFT;
          newState = true;
        }
        //FN Recieved
        else if (inchar == 18) {                                  
          if (CurrentKBState == FUNC) CurrentKBState = NORMAL;
          else CurrentKBState = FUNC;
          newState = true;
        }
        //Space Recieved
        else if (inchar == 32) {                                  
          allText += (currentWord + " ");
          currentWord = "";
        }
        //CR Recieved
        else if (inchar == 13) {                          
          allText += (currentWord + "\n");
          currentWord = "";
        }
        //ESC / CLEAR Recieved
        else if (inchar == 20) {                                  
          allText ="";
          currentWord = "";
          oledWord("Clearing...");
          einkRefresh = FULL_REFRESH_AFTER + 1;
          newState = true;
          delay(300);
        }
        else if (inchar == 19) {                                  
          scroll--;
          einkRefresh = FULL_REFRESH_AFTER + 1;
          newState = true;
        }
        else if (inchar == 21) {                                  
          scroll++;
          einkRefresh = FULL_REFRESH_AFTER + 1;
          newState = true;
        }
        //BKSP Recieved
        else if (inchar == 8) {                  
          if (currentWord.length() > 0) {
            currentWord.remove(currentWord.length() - 1);
          }
        }
        //SAVE Recieved
        else if (inchar == 6) {
          //File exists, save normally
          if (editingFile != "" && editingFile != "-") {
            keypad.disableInterrupts();
            oledWord("Saving File");
            writeFile(SPIFFS, editingFile.c_str(), allText.c_str());
            oledWord("Saved");
            delay(200);
            keypad.enableInterrupts();
            CurrentKBState = NORMAL;
            newState = true;
          }
          //File does not exist, make a new one
          else {
            CurrentTXTState = WIZ3;
            currentWord = "";
            CurrentKBState = NORMAL;
            einkRefresh = FULL_REFRESH_AFTER + 1;
            newState = true;
          }
        }
        //LOAD Recieved
        else if (inchar == 5) {
          keypad.disableInterrupts();
          oledWord("Loading File");
          allText = readFileToString(SPIFFS, ("/" + editingFile).c_str());
          keypad.enableInterrupts();
          CurrentKBState = NORMAL;
          newState = true;
        }
        //FILE Recieved
        else if (inchar == 7) {
          CurrentTXTState = WIZ0;
          CurrentKBState = NORMAL;
          newState = true;
        }

        else {
          currentWord += inchar;
          if (inchar >= 48 && inchar <= 57) {}  //Only leave FN on if typing numbers
          else {
            CurrentKBState = NORMAL;
            newState = true;
          }
        }
        currentMillis = millis();
        //Make sure oled only updates at 60fps
        if (currentMillis - OLEDFPSMillis >= 16) {
          OLEDFPSMillis = currentMillis;
          oledWord(currentWord);
        }
        break;
      case WIZ0:
        //No char recieved
        if (inchar == 0);
        //BKSP Recieved
        else if (inchar == 127 || inchar == 8) {                  
          CurrentTXTState = TXT_;
          CurrentKBState = NORMAL;
          einkRefresh = FULL_REFRESH_AFTER + 1;
          newState = true;
          display.fillScreen(GxEPD_WHITE);
        }
        else {
          int fileIndex = (inchar == '0') ? 10 : (inchar - '0');
          //Edit a new file
          if (filesList[fileIndex - 1] != editingFile) {
            //Selected file does not exist, create a new one
            if (filesList[fileIndex - 1] == "-") {
              CurrentTXTState = WIZ3;
              einkRefresh = FULL_REFRESH_AFTER + 1;
              newState = true;
              display.fillScreen(GxEPD_WHITE);
            }
            //Selected file exists, prompt to save current file
            else {      
              prevEditingFile = editingFile;
              editingFile = filesList[fileIndex - 1];      
              CurrentTXTState = WIZ1;
              einkRefresh = FULL_REFRESH_AFTER + 1;
              newState = true;
              display.fillScreen(GxEPD_WHITE);
            }
          }
          //Selected file is current file, return to editor
          else {
            CurrentTXTState = TXT_;
            CurrentKBState = NORMAL;
            einkRefresh = FULL_REFRESH_AFTER + 1;
            newState = true;
            display.fillScreen(GxEPD_WHITE);
          }

        }

        currentMillis = millis();
        //Make sure oled only updates at 60fps
        if (currentMillis - OLEDFPSMillis >= 16) {
          OLEDFPSMillis = currentMillis;
          oledWord(currentWord);
        }
        break;
      case WIZ1:
        //No char recieved
        if (inchar == 0);
        //BKSP Recieved
        else if (inchar == 127 || inchar == 8) {                  
          CurrentTXTState = WIZ0;
          CurrentKBState = FUNC;
          einkRefresh = FULL_REFRESH_AFTER + 1;
          newState = true;
          display.fillScreen(GxEPD_WHITE);
        }
        else {
          int numSelect = (inchar == '0') ? 10 : (inchar - '0');
          //YES (save current file)
          if (numSelect == 1) {
            Serial.println("YES (save current file)");
            //File to be saved does not exist
            if (prevEditingFile == "" || prevEditingFile == "-") {
              CurrentTXTState = WIZ2;
              currentWord = "";
              CurrentKBState = NORMAL;
              einkRefresh = FULL_REFRESH_AFTER + 1;
              newState = true;
              display.fillScreen(GxEPD_WHITE);
            }
            //File to be saved exists
            else {
              //Save current file
              keypad.disableInterrupts();
              oledWord("Saving File");
              writeFile(SPIFFS, prevEditingFile.c_str(), allText.c_str());
              oledWord("Saved");
              delay(200);
              //Load new file
              oledWord("Loading File");
              allText = readFileToString(SPIFFS, editingFile.c_str());
              keypad.enableInterrupts();
              //Return to TXT
              CurrentTXTState = TXT_;
              CurrentKBState = NORMAL;
              einkRefresh = FULL_REFRESH_AFTER + 1;
              newState = true;
              display.fillScreen(GxEPD_WHITE);
            }
          }
          //NO  (don't save current file)
          else if (numSelect == 2) {
            Serial.println("NO  (don't save current file)");
            //Just load new file
            keypad.disableInterrupts();
            oledWord("Loading File");
            allText = readFileToString(SPIFFS, ("/" + editingFile).c_str());
            keypad.enableInterrupts();
            //Return to TXT
            CurrentTXTState = TXT_;
            CurrentKBState = NORMAL;
            einkRefresh = FULL_REFRESH_AFTER + 1;
            newState = true;
            display.fillScreen(GxEPD_WHITE);
          }
        }

        currentMillis = millis();
        //Make sure oled only updates at 60fps
        if (currentMillis - OLEDFPSMillis >= 16) {
          OLEDFPSMillis = currentMillis;
          oledWord(currentWord);
        }
        break;

      case WIZ2:
        //No char recieved
        if (inchar == 0);                                         
        //SHIFT Recieved
        else if (inchar == 17) {                                  
          if (CurrentKBState == SHIFT) CurrentKBState = NORMAL;
          else CurrentKBState = SHIFT;
          newState = true;
        }
        //FN Recieved
        else if (inchar == 18) {                                  
          if (CurrentKBState == FUNC) CurrentKBState = NORMAL;
          else CurrentKBState = FUNC;
          newState = true;
        }
        //Space Recieved
        else if (inchar == 32) {}
        //ESC / CLEAR Recieved
        else if (inchar == 20) {                                  
          currentWord = "";
        }
        //BKSP Recieved
        else if (inchar == 8) {                  
          if (currentWord.length() > 0) {
            currentWord.remove(currentWord.length() - 1);
          }
        }
        //ENTER Recieved
        else if (inchar == 13) {                          
          prevEditingFile = "/" + currentWord + ".txt";

          //Save the file
          keypad.disableInterrupts();
          oledWord("Saving File");
          writeFile(SPIFFS, prevEditingFile.c_str(), allText.c_str());
          oledWord("Saved");
          delay(200);
          //Load new file

          keypad.enableInterrupts();

          //Return to TXT_
          CurrentTXTState = TXT_;
          CurrentKBState = NORMAL;
          newState = true;
          currentWord = "";
        }
        //All other chars
        else {
          //Only allow char to be added if it's an allowed char
          if (isalnum(inchar) || inchar == '_' || inchar == '-' || inchar == '.') currentWord += inchar;
          if (inchar >= 48 && inchar <= 57) {}  //Only leave FN on if typing numbers
          else {CurrentKBState = NORMAL; newState = true;}
        }

        currentMillis = millis();
        //Make sure oled only updates at 60fps
        if (currentMillis - OLEDFPSMillis >= 16) {
          OLEDFPSMillis = currentMillis;
          oledWord(currentWord);
        }
        break;
      case WIZ3:
        //No char recieved
        if (inchar == 0);                                         
        //SHIFT Recieved
        else if (inchar == 17) {                                  
          if (CurrentKBState == SHIFT) CurrentKBState = NORMAL;
          else CurrentKBState = SHIFT;
          newState = true;
        }
        //FN Recieved
        else if (inchar == 18) {                                  
          if (CurrentKBState == FUNC) CurrentKBState = NORMAL;
          else CurrentKBState = FUNC;
          newState = true;
        }
        //Space Recieved
        else if (inchar == 32) {}
        //ESC / CLEAR Recieved
        else if (inchar == 20) {                                  
          currentWord = "";
        }
        //BKSP Recieved
        else if (inchar == 8) {                  
          if (currentWord.length() > 0) {
            currentWord.remove(currentWord.length() - 1);
          }
        }
        //ENTER Recieved
        else if (inchar == 13) {                          
          editingFile = "/" + currentWord + ".txt";

          //Save the file
          keypad.disableInterrupts();
          oledWord("Saving " + editingFile);
          writeFile(SPIFFS, editingFile.c_str(), allText.c_str());
          oledWord("Saved " + editingFile);
          delay(200);
          keypad.enableInterrupts();
          //Ask to save prev file
          
          //Return to TXT_
          CurrentTXTState = TXT_;
          CurrentKBState = NORMAL;
          newState = true;
          currentWord = "";
        }
        //All other chars
        else {
          //Only allow char to be added if it's an allowed char
          if (isalnum(inchar) || inchar == '_' || inchar == '-' || inchar == '.') currentWord += inchar;
          if (inchar >= 48 && inchar <= 57) {}  //Only leave FN on if typing numbers
          else {CurrentKBState = NORMAL; newState = true;}
        }

        currentMillis = millis();
        //Make sure oled only updates at 60fps
        if (currentMillis - OLEDFPSMillis >= 16) {
          OLEDFPSMillis = currentMillis;
          oledWord(currentWord);
        }
        break;

    }
    KBBounceMillis = currentMillis;
  }
}

int countLines(String input, size_t maxLineLength = 29) {
  size_t    inputLength   = input.length();
  uint8_t   charCounter   = 0;
  uint16_t  lineCounter   = 1;

    for (size_t c = 0; c < inputLength; c++) { 
      if (input[c] == '\n') {
        charCounter = 0;
        lineCounter++;
        continue;
      }
      else if (charCounter > (maxLineLength-1)) {
        charCounter = 0;
        lineCounter++;
      }
      charCounter++;
    }

    return lineCounter;
}

bool splitIntoLines(const char* input, int scroll_) {
  size_t    maxLineLength = 29;                 //Chars per line
  size_t    maxLines      = 100;               //Total lines
  size_t    inputLength   = strlen(input);
  uint8_t   charCounter   = 0;
  uint16_t  lineCounter   = 0;

  //Declare a large local array
  String fullLines[maxLines];

  //Clear fullLines
  for (int i = 0; i < maxLines; i++) {
    fullLines[i] = "";
  }

  //Increment through input one char at a time
  for (size_t c = 0; c < inputLength && lineCounter < maxLines; c++) { 
    if (input[c] == '\n') {
      charCounter = 0;
      lineCounter++;
      continue;
    }
    else if (charCounter > (maxLineLength-1)) {
      charCounter = 0;
      lineCounter++;
    }
    fullLines[lineCounter] += input[c];
    charCounter++;
  }

  for (int i = 0; i < 13; i++) {
    outLines[i] = "";
  }

  /*for (int i = 0; i < maxLines; i++) {
    printf(("fullLines " + String(i) + ": " + fullLines[i] + "\n").c_str());
  }*/

  int lineCounter_ = 0;
  if (lineCounter < 13) {
    scroll_ = 0;
    lineCounter_ = 13;
  }
  else lineCounter_ = lineCounter;
  
  uint8_t j = 0;
  for (uint16_t i = (lineCounter_-13-scroll_); j < 13; i++, j++) { //i: fullLines index, j:outLines index
    if (i >= 0 && i < maxLines) {
      outLines[j] = fullLines[i];  // Copy lines from fullLines to outLines
      //printf(("Line " + String(j) + ": " + outLines[j] + "\n").c_str());
    }
  }

  static String prevTopLine = "";  // Static to keep track of the previous top line
  String topLine = outLines[0];

  if (topLine != prevTopLine && lineCounter > 12) {
    // If the top line has changed, it's a new line
    einkRefresh = FULL_REFRESH_AFTER + 1;
    newState = true;
    prevTopLine = topLine;  // Update the previous top line
    return true;
  }
  else return false;
}

int countWords(String str) {
    int count = 0;
    bool inWord = false;

    // Loop through each character in the String
    for (int i = 0; i < str.length(); i++) {
        char c = str.charAt(i);

        // If the character is a space, mark the end of a word
        if (c == ' ') {
            if (inWord) {
                count++;  // We've encountered the end of a word
                inWord = false;
            }
        } else {
            // If the character is not a space, we're in a word
            inWord = true;
        }
    }

    // If the last character is not a space, we have one more word
    if (inWord) {
        count++;
    }

    return count;
}

int countVisibleChars(String input) {
  int count = 0;

  for (size_t i = 0; i < input.length(); i++) {
    char c = input[i];
    // Check if the character is a visible character or space
    if (c >= 32 && c <= 126) { // ASCII range for printable characters and space
      count++;
    }
  }

  return count;
}

void einkTextPartial(String text, bool noRefresh = false) {
  bool doFullRefresh = false;

  einkRefresh++;
  if (einkRefresh > FULL_REFRESH_AFTER) {
    doFullRefresh = true;
    einkRefresh = 0;
    display.setFullWindow();
    display.fillScreen(GxEPD_WHITE);
  }

  display.setFont(&FreeMonoBold9pt7b);

  if(splitIntoLines(text.c_str(), scroll)) doFullRefresh = true;

  for (int i = 0; i < 13; i++) {
    if (outLines[i] != "") { // Print only non-empty lines
      if (doFullRefresh) {
        display.fillRect(0,16*i,display.width(),16,GxEPD_WHITE);
        display.setCursor(0, 10+(16*i));
        display.print(outLines[i]);
      }
      else if (outLines[i] != lines_prev[i]) {   //If the line has changed
        display.setPartialWindow(0,16*i,display.width(),16);
        display.fillRect(0,16*i,display.width(),16,GxEPD_WHITE);
        display.setCursor(0, 10+(16*i));
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
    lines_prev[i] = outLines[i]; // Copy each line
  }
}

void einkHandler_TXT() {
  if ((prevAllText != allText) || newState) {
    newState = false;
    switch (CurrentTXTState) {
      case TXT_:
        prevAllText = allText;
        einkTextPartial(allText);

        bottomText("C:" + String(countVisibleChars(allText)) + ",L:" + String(countLines(allText)) + "," + editingFile);
        
        refresh();
        break;
      case WIZ0:
        prevAllText = allText;
        einkRefresh = FULL_REFRESH_AFTER + 1;
        display.setFullWindow();
        einkTextPartial(allText, true);      
        display.setFont(&FreeMonoBold9pt7b);
        
        display.fillRect(0,display.height()-26,display.width(),26,GxEPD_WHITE);
        display.drawRect(0,display.height()-20,display.width(),20,GxEPD_BLACK);
        display.setCursor(4, display.height()-6);
        display.print("W:" + String(countWords(allText)) + " C:" + String(countVisibleChars(allText)) + " L:" + String(countLines(allText)));
        display.drawBitmap(display.width()-30,display.height()-20, KBStatusallArray[6], 30, 20, GxEPD_BLACK);

        display.fillRect(60,0,200,218,GxEPD_WHITE);
        display.drawBitmap(60,0,fileWizLiteallArray[0],200,218, GxEPD_BLACK);

        keypad.disableInterrupts();
        listDir(SPIFFS, "/");
        keypad.enableInterrupts();

        for (int i = 0; i < MAX_FILES; i++) {
          display.setCursor(88, 54+(17*i));
          display.print(filesList[i]);
        }

        display.nextPage();
        display.hibernate();
        CurrentKBState = FUNC;
        break;
      case WIZ1:
        display.setFont(&FreeMonoBold9pt7b);
        
        display.fillRect(0,display.height()-26,display.width(),26,GxEPD_WHITE);
        display.drawRect(0,display.height()-20,display.width(),20,GxEPD_BLACK);
        display.setCursor(4, display.height()-6);
        display.print("W:" + String(countWords(allText)) + " C:" + String(countVisibleChars(allText)) + " L:" + String(countLines(allText)));
        display.drawBitmap(display.width()-30,display.height()-20, KBStatusallArray[6], 30, 20, GxEPD_BLACK);

        display.fillRect(60,0,200,218,GxEPD_WHITE);
        display.drawBitmap(60,0,fileWizLiteallArray[1],200,218, GxEPD_BLACK);

        display.nextPage();
        display.hibernate();
        CurrentKBState = FUNC;
        break;
      case WIZ2:
        display.setFont(&FreeMonoBold9pt7b);
        
        display.fillRect(0,display.height()-26,display.width(),26,GxEPD_WHITE);
        display.drawRect(0,display.height()-20,display.width(),20,GxEPD_BLACK);
        display.setCursor(4, display.height()-6);
        display.print("W:" + String(countWords(allText)) + " C:" + String(countVisibleChars(allText)) + " L:" + String(countLines(allText)));
        display.drawBitmap(display.width()-30,display.height()-20, KBStatusallArray[6], 30, 20, GxEPD_BLACK);

        display.fillRect(60,0,200,218,GxEPD_WHITE);
        display.drawBitmap(60,0,fileWizLiteallArray[2],200,218, GxEPD_BLACK);

        display.nextPage();
        display.hibernate();
        CurrentKBState = NORMAL;
        break;
      case WIZ3:
        prevAllText = allText;
        einkRefresh = FULL_REFRESH_AFTER + 1;
        display.setFullWindow();
        einkTextPartial(allText, true);      
        display.setFont(&FreeMonoBold9pt7b);
        
        display.fillRect(0,display.height()-26,display.width(),26,GxEPD_WHITE);
        display.drawRect(0,display.height()-20,display.width(),20,GxEPD_BLACK);
        display.setCursor(4, display.height()-6);
        display.print("W:" + String(countWords(allText)) + " C:" + String(countVisibleChars(allText)) + " L:" + String(countLines(allText)));
        display.drawBitmap(display.width()-30,display.height()-20, KBStatusallArray[6], 30, 20, GxEPD_BLACK);

        display.fillRect(60,0,200,218,GxEPD_WHITE);
        display.drawBitmap(60,0,fileWizLiteallArray[3],200,218, GxEPD_BLACK);

        display.nextPage();
        display.hibernate();
        CurrentKBState = NORMAL;
        break;
    }

  }
}