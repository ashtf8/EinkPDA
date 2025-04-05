//   .oooooo..o oooooo   oooo  .oooooo..o ooooooooooooo oooooooooooo ooo        ooooo  //
//  d8P'    `Y8  `888.   .8'  d8P'    `Y8 8'   888   `8 `888'     `8 `88.       .888'  //
//  Y88bo.        `888. .8'   Y88bo.           888       888          888b     d'888   //
//   `"Y8888o.     `888.8'     `"Y8888o.       888       888oooo8     8 Y88. .P  888   //
//       `"Y88b     `888'          `"Y88b      888       888    "     8  `888'   888   //
//  oo     .d8P      888      oo     .d8P      888       888       o  8    Y     888   //
//  8""88888P'      o888o     8""88888P'      o888o     o888ooooood8 o8o        o888o  //

void saveFile() {
  switch (TXT_APP_STYLE) {
    case 0:
      if (editingFile == "" || editingFile == "-") editingFile = "/temp.txt";
      keypad.disableInterrupts();
      oledWord("Saving File");
      writeFile(SPIFFS, editingFile.c_str(), allText.c_str());
      oledWord("Saved"+editingFile);
      delay(1000);
      keypad.enableInterrupts();
      break;
    case 1:
      String textToSave = vectorToString();
      Serial.println("Text to save:");
      Serial.println(textToSave);
      if (editingFile == "" || editingFile == "-") editingFile = "/temp.txt";
      keypad.disableInterrupts();
      if (!editingFile.startsWith("/")) editingFile = "/" + editingFile;
      oledWord("Saving File: "+ editingFile);
      writeFile(SPIFFS, (editingFile).c_str(), textToSave.c_str());
      oledWord("Saved: "+ editingFile);
      delay(1000);
      keypad.enableInterrupts();
      break;
  }
}

void loadFile() {
  switch (TXT_APP_STYLE) {
    case 0:
      keypad.disableInterrupts();
      oledWord("Loading File");
      allText = readFileToString(SPIFFS, ("/" + editingFile).c_str());
      keypad.enableInterrupts();
      break;
    case 1:
      keypad.disableInterrupts();
      oledWord("Loading File");
      if (!editingFile.startsWith("/")) editingFile = "/" + editingFile;
      String textToLoad = readFileToString(SPIFFS, (editingFile).c_str());
      Serial.println("Text to load:");
      Serial.println(textToLoad);
      stringToVector(textToLoad);
      keypad.enableInterrupts();
      break;
  }
}

void delFile(String fileName) {
  keypad.disableInterrupts();
  oledWord("Deleting File: "+ fileName);
  if (!fileName.startsWith("/")) fileName = "/" + fileName;
  deleteFile(SPIFFS, fileName.c_str());
  oledWord("Deleted: "+ fileName);
  delay(1000);
  keypad.enableInterrupts();
}

void renFile(String oldFile, String newFile) {
  keypad.disableInterrupts();
  oledWord("Renaming "+ oldFile + " to " + newFile);
  if (!oldFile.startsWith("/")) oldFile = "/" + oldFile;
  if (!newFile.startsWith("/")) newFile = "/" + newFile;
  renameFile(SPIFFS, oldFile.c_str(), newFile.c_str());
  oledWord("Renamed File");
  delay(1000);
  keypad.enableInterrupts();
}

void copyFile(String oldFile, String newFile) {
  keypad.disableInterrupts();
  oledWord("Loading File");
  if (!oldFile.startsWith("/")) oldFile = "/" + oldFile;
  if (!newFile.startsWith("/")) newFile = "/" + newFile;
  String textToLoad = readFileToString(SPIFFS, (oldFile).c_str());
  writeFile(SPIFFS, (newFile).c_str(), textToLoad.c_str());
  oledWord("Saved: "+ newFile);
  delay(1000);
  keypad.enableInterrupts();
}

void appendToFile(String path, String inText) {
  keypad.disableInterrupts();
  appendFile(SPIFFS, path.c_str(), inText.c_str());
  keypad.enableInterrupts();
}

String vectorToString() {
  String result;
  setTXTFont(currentFont);

  for (size_t i = 0; i < allLines.size(); i++) {
    result += allLines[i];

    int16_t x1, y1;
    uint16_t charWidth, charHeight;
    display.getTextBounds(allLines[i], 0, 0, &x1, &y1, &charWidth, &charHeight);

    // Add newline only if the line doesn't fully use the available space
    if (charWidth < display.width() && i < allLines.size() - 1) {
      result += '\n';
    }
  }

  return result;
}

void stringToVector(String inputText) {
  setTXTFont(currentFont);
  allLines.clear();
  String currentLine_;

  for (size_t i = 0; i < inputText.length(); i++) {
    char c = inputText[i];

    int16_t x1, y1;
    uint16_t charWidth, charHeight;
    display.getTextBounds(currentLine_, 0, 0, &x1, &y1, &charWidth, &charHeight);

    if ((c == '\n' || charWidth >= display.width() - 5) && !currentLine_.isEmpty()) {
      allLines.push_back(currentLine_);
      currentLine_ = "";
    }
    
    if (c != '\n') {
      currentLine_ += c;
    }
  }

  // PUSH LAST LINE IF IT'S NOT EMPTY
  if (!currentLine_.isEmpty()) {
    allLines.push_back(currentLine_);
  }
}

String removeChar(String str, char character) {
  String result = "";
  for (size_t i = 0; i < str.length(); i++) {
    if (str[i] != character) {
      result += str[i];
    }
  }
  return result;
}

void checkTimeout() {
  timeoutMillis = millis();

  //Trigger timeout deep sleep
  if (!disableTimeout) {
    if (timeoutMillis - prevTimeMillis >= TIMEOUT*1000) {
        Serial.println("Device Idle... Deep Sleeping");
        //Give a chance to keep device awake
        oledWord("Going to sleep!");
        int i = millis();
        int j = millis();
        while ((j - i) <= 4000) {  //10 sec
          j = millis();
          if (digitalRead(KB_IRQ) == 0) {
            oledWord("My Hero!");
            delay(500);
            prevTimeMillis = millis();
            TCA8418_event = false;
            return;
          }
        }

        //Save current work:
        //Only save if alltext has significant content
        if (allText.length() > 10) {
          //No current file, save in temp.txt
          saveFile();
        }

        //Put OLED to sleep
        u8g2.setPowerSave(1);

        //Stop the einkHandler task
        if (einkHandlerTaskHandle != NULL) {
          vTaskDelete(einkHandlerTaskHandle);
          einkHandlerTaskHandle = NULL;
        }

        switch (CurrentAppState) {
          case HOME:
              display.setFullWindow();
              display.drawBitmap(0, 0, sleep0, 320, 240, GxEPD_BLACK);
            break;
          case TXT:
            if (SLEEPMODE == "TEXT" && editingFile != "") {
              prevAllText = allText;
              einkRefresh = FULL_REFRESH_AFTER + 1;
              display.setFullWindow();
              if (TXT_APP_STYLE == 0) einkTextPartial(allText, true);
              else if (TXT_APP_STYLE == 1) einkTextDynamic(true, true);
                    
              display.setFont(&FreeMonoBold9pt7b);
              
              display.fillRect(0,display.height()-26,display.width(),26,GxEPD_WHITE);
              display.drawRect(0,display.height()-20,display.width(),20,GxEPD_BLACK);
              display.setCursor(4, display.height()-6);
              display.print("W:" + String(countWords(allText)) + " C:" + String(countVisibleChars(allText)) + " L:" + String(countLines(allText)));
              display.drawBitmap(display.width()-30,display.height()-20, KBStatusallArray[6], 30, 20, GxEPD_BLACK);
              statusBar(editingFile, true);
              
              display.fillRect(320-86, 240-52, 87, 52, GxEPD_WHITE);
              display.drawBitmap(320-86, 240-52, sleep1, 87, 52, GxEPD_BLACK);
            }
            else {
              //Display sleep image on E-Ink
              display.setFullWindow();
              display.drawBitmap(0, 0, sleep0, 320, 240, GxEPD_BLACK);
            }
            break;
        }
        
        display.nextPage();
        display.hibernate();
        
        //Sleep the device
        esp_deep_sleep_start();
      }
  }
  else {
    prevTimeMillis = millis();
  }
  
  if (PWR_BTN_event && CurrentHOMEState != NOWLATER) {
    PWR_BTN_event = false;
    
    //Save current work:
    //Only save if alltext has significant content
    if (allText.length() > 10) {
      oledWord("Saving Work");
      saveFile();
    }
    
    if (digitalRead(CHRG_SENS) == HIGH) {
      CurrentAppState = HOME;
      CurrentHOMEState = NOWLATER;
      u8g2.setPowerSave(1);
      OLEDPowerSave  = true;
      disableTimeout = true;
      newState = true;

      display.setFullWindow();
      display.fillScreen(GxEPD_WHITE);
      //display.nextPage();
      //display.hibernate();
      //display.display(true);
      //display.hibernate();
      //delay(500);
    }
    else {
      //Put OLED to sleep
      u8g2.setPowerSave(1);

      //Stop the einkHandler task
      if (einkHandlerTaskHandle != NULL) {
        vTaskDelete(einkHandlerTaskHandle);
        einkHandlerTaskHandle = NULL;
      }

      switch (CurrentAppState) {
        case HOME:
            display.setFullWindow();
            display.drawBitmap(0, 0, sleep0, 320, 240, GxEPD_BLACK);
          break;
        case TXT:
          if (SLEEPMODE == "TEXT" && editingFile != "") {
            prevAllText = allText;
            einkRefresh = FULL_REFRESH_AFTER + 1;
            display.setFullWindow();
            if (TXT_APP_STYLE == 0) einkTextPartial(allText, true);
            else if (TXT_APP_STYLE == 1) einkTextDynamic(true, true);    
            display.setFont(&FreeMonoBold9pt7b);
            
            display.fillRect(0,display.height()-26,display.width(),26,GxEPD_WHITE);
            display.drawRect(0,display.height()-20,display.width(),20,GxEPD_BLACK);
            display.setCursor(4, display.height()-6);
            display.print("W:" + String(countWords(allText)) + " C:" + String(countVisibleChars(allText)) + " L:" + String(countLines(allText)));
            display.drawBitmap(display.width()-30,display.height()-20, KBStatusallArray[6], 30, 20, GxEPD_BLACK);
            statusBar(editingFile, true);
            
            display.fillRect(320-86, 240-52, 87, 52, GxEPD_WHITE);
            display.drawBitmap(320-86, 240-52, sleep1, 87, 52, GxEPD_BLACK);
          }
          else {
            //Display sleep image on E-Ink
            display.setFullWindow();
            display.drawBitmap(0, 0, sleep0, 320, 240, GxEPD_BLACK);
          }
          break;
      }
      
      forceSlowFullUpdate = true;
      refresh();
      display.hibernate();
      
      //Sleep the device
      esp_deep_sleep_start();
    }
    
  }
  else if (PWR_BTN_event && CurrentHOMEState == NOWLATER) {
    CurrentAppState = HOME;
    CurrentHOMEState = HOME_HOME;
    PWR_BTN_event = false;
    if (OLEDPowerSave) {
      u8g2.setPowerSave(0);
      OLEDPowerSave = false;
    }
    display.fillScreen(GxEPD_WHITE);
    forceSlowFullUpdate = true;
    refresh();
    delay(200);
    newState = true;
  }
}

void setCpuSpeed(int newFreq) {
  // Return early if the frequency is already set
  if (getCpuFrequencyMhz() == newFreq) return;

  int validFreqs[] = {240, 160, 80, 40, 20, 10};
  bool isValid = false;

  for (int i = 0; i < sizeof(validFreqs) / sizeof(validFreqs[0]); i++) {
    if (newFreq == validFreqs[i]) {
      isValid = true;
      break;
    }
  }

  if (isValid) {
    setCpuFrequencyMhz(newFreq);
    Serial.print("CPU Speed changed to: ");
    Serial.print(newFreq);
    Serial.println(" MHz");
  } 
}

void updateBattState() {
  //mapping settings 
  static float batteryMaxVoltage = 4.4;  // min and max voltages (mapping function will still work if out of these bounds; ratios will just be off)
  static float batteryMinVoltage = 3.6;
  static int spriteMax = 6; // bitmap full charge
  static int spriteMin = 2; // bitmap min charge 
  float batteryVoltage = (analogRead(BAT_SENS) * (3.3 / 4095.0) * 2) + 0.2;
  float batteryDifference = batteryVoltage - batteryMinVoltage;
  

  static float prevVoltage = 0.0;
  float threshold = 0.05; // Hysteresis threshold (adjustable)

  if (digitalRead(CHRG_SENS) == 1) { // Forces batt state 7 if charging 
    battState = 7;
  } 
  else if (abs(prevVoltage-batteryVoltage)>threshold){ // Checks if Voltage has changed enough to avoid exessive polling
    battState =
    constrain(round( // Rounds the mapping function and sets limits for proper array indexing 
      ((batteryDifference / (batteryMaxVoltage-batteryMinVoltage)) * (spriteMax+spriteMin/spriteMin)) + spriteMin // Maps voltage to index 
    ),spriteMin,spriteMax); // Hail Mary for if voltage is ever in unexpected state 
    
    prevVoltage = batteryVoltage; // Updates new voltage reference 
  }

  if (battState != prevBattState) {
    prevBattState = battState;
    newState = true;
  }

}

void TCA8418_irq() {
  TCA8418_event = true;
}

void PWR_BTN_irq() {
  PWR_BTN_event = true;
}

char updateKeypress() {
  if (TCA8418_event == true) {
    int k = keypad.getEvent();
    
    //  try to clear the IRQ flag
    //  if there are pending events it is not cleared
    keypad.writeRegister(TCA8418_REG_INT_STAT, 1);
    int intstat = keypad.readRegister(TCA8418_REG_INT_STAT);
    if ((intstat & 0x01) == 0) TCA8418_event = false;

    if (k & 0x80) {   //Key pressed, not released
      k &= 0x7F;
      k--;
      //return currentKB[k/10][k%10];
      if ((k/10) < 4) {
        //Key was pressed, reset timeout counter
        prevTimeMillis = millis();

        //Return Key
        switch (CurrentKBState) {
          case NORMAL:
            return keysArray[k/10][k%10];
            break;
          case SHIFT:
            return keysArraySHFT[k/10][k%10];
            break;
          case FUNC:
            return keysArrayFN[k/10][k%10];
            break;
        }
      }
      else return 0;
    }
    else return 0;
  }
  else return 0;
}

void printDebug() {
  DateTime now = rtc.now();
  if (now.second() != prevSec) {
    prevSec = now.second();

    // DIVIDER
    Serial.print("//////////////////////////////////////_DEBUG_//////////////////////////////////////"); 
    Serial.println();

    // DISPLAY GPIO STATES
    Serial.print("PWR_BTN: "); Serial.print(digitalRead(PWR_BTN));
    Serial.print(", KB_INT: "); Serial.print(digitalRead(KB_IRQ));
    Serial.print(", CHRG: "); Serial.print(digitalRead(CHRG_SENS));
    Serial.print(", RTC_INT: "); Serial.print(digitalRead(RTC_INT));

    // READ AND DISPLAY BATTERY VOLTAGE
    float batteryVoltage = (analogRead(BAT_SENS) * (3.3 / 4095.0) * 2) + 0.2;
    Serial.print(", BAT: "); Serial.print(batteryVoltage, 2); // Print with 2 decimal places
    
    // DISPLAY CLOCK SPEED
    Serial.print(", CPU_FRQ: "); Serial.print(getCpuFrequencyMhz(), 1);

    // FAST FULL UPDATE MODE
    Serial.print(", FFU: "); Serial.println(GxEPD2_310_GDEQ031T10::useFastFullUpdate);

    // DISPLAY SYSTEM TIME
    Serial.print("SYSTEM_CLOCK: ");
    Serial.print(now.month(), DEC);
    Serial.print('/');
    Serial.print(now.day(), DEC);
    Serial.print('/');
    Serial.print(now.year(), DEC);
    Serial.print(" (");
    Serial.print(daysOfTheWeek[now.dayOfTheWeek()]);
    Serial.print(") ");
    Serial.print(now.hour(), DEC);
    Serial.print(':');
    Serial.print(now.minute(), DEC);
    Serial.print(':');
    Serial.print(now.second(), DEC);
    
    Serial.println();
  }
}

//   .oooooo..o ooooooooo.   ooooo oooooooooooo oooooooooooo  .oooooo..o  //
//  d8P'    `Y8 `888   `Y88. `888' `888'     `8 `888'     `8 d8P'    `Y8  //
//  Y88bo.       888   .d88'  888   888          888         Y88bo.       //
//   `"Y8888o.   888ooo88P'   888   888oooo8     888oooo8     `"Y8888o.   //
//       `"Y88b  888          888   888    "     888    "         `"Y88b  //
//  oo     .d8P  888          888   888          888         oo     .d8P  //
//  8""88888P'  o888o        o888o o888o        o888o        8""88888P'   //

void listDir(fs::FS &fs, const char *dirname) {
  Serial.printf("Listing directory: %s\r\n", dirname);

  File root = fs.open(dirname);
  if (!root) {
    Serial.println("- failed to open directory");
    return;
  }
  if (!root.isDirectory()) {
    Serial.println(" - not a directory");
    return;
  }

  // Reset fileIndex and initialize filesList with "-"
  fileIndex = 0; // Reset fileIndex
  for (int i = 0; i < MAX_FILES; i++) {
    filesList[i] = "-";
  }

  File file = root.openNextFile();
  while (file && fileIndex < MAX_FILES) {
    if (!file.isDirectory()) {
      String fileName = String(file.name());
      
      // Check if file is in the exclusion list
      bool excluded = false;
      for (const String &excludedFile : excludedFiles) {
        if (fileName.equals(excludedFile)) {
          excluded = true;
          break;
        }
      }

      if (!excluded) {
        filesList[fileIndex++] = fileName; // Store file name if not excluded
      }
    }
    file = root.openNextFile();
  }

  for (int i = 0; i < fileIndex; i++) { // Only print valid entries
    Serial.println(filesList[i]);
  }
}

void readFile(fs::FS &fs, const char *path) {
  Serial.printf("Reading file: %s\r\n", path);

  File file = fs.open(path);
  if (!file || file.isDirectory()) {
    Serial.println("- failed to open file for reading");
    return;
  }

  Serial.println("- read from file:");
  while (file.available()) {
    Serial.write(file.read());
  }
  file.close();
}

String readFileToString(fs::FS &fs, const char *path) {
  Serial.printf("Reading file: %s\r\n", path);

  File file = fs.open(path);
  if (!file || file.isDirectory()) {
    Serial.println("- failed to open file for reading");
    oledWord("Load Failed");
    delay(500);
    return "";  // Return an empty string on failure
  }

  Serial.println("- reading from file:");
  String content = "";  // Initialize an empty String to hold the content

  while (file.available()) {
    content += (char)file.read();  // Read each character and append to the String
  }

  file.close();
  oledWord("File Loaded");
  delay(200);
  einkRefresh = FULL_REFRESH_AFTER; //Force a full refresh
  return content;  // Return the complete String
}

void writeFile(fs::FS &fs, const char *path, const char *message) {
  Serial.printf("Writing file: %s\r\n", path);
  delay(200);

  File file = fs.open(path, FILE_WRITE);
  if (!file) {
    Serial.println("- failed to open file for writing");
    return;
  }
  if (file.print(message)) {
    Serial.println("- file written");
  } else {
    Serial.println("- write failed");
  }
  file.close();
}

void appendFile(fs::FS &fs, const char *path, const char *message) {
  Serial.printf("Appending to file: %s\r\n", path);

  File file = fs.open(path, FILE_APPEND);
  if (!file) {
    Serial.println("- failed to open file for appending");
    return;
  }
  if (file.println(message)) {
    Serial.println("- message appended");
  } else {
    Serial.println("- append failed");
  }
  file.close();
}

void renameFile(fs::FS &fs, const char *path1, const char *path2) {
  Serial.printf("Renaming file %s to %s\r\n", path1, path2);
  if (fs.rename(path1, path2)) {
    Serial.println("- file renamed");
  } else {
    Serial.println("- rename failed");
  }
}

void deleteFile(fs::FS &fs, const char *path) {
  Serial.printf("Deleting file: %s\r\n", path);
  if (fs.remove(path)) {
    Serial.println("- file deleted");
  } else {
    Serial.println("- delete failed");
  }
}