// ooooooooooooo       .o.        .oooooo..o oooo    oooo  .oooooo..o //
// 8'   888   `8      .888.      d8P'    `Y8 `888   .8P'  d8P'    `Y8 //
//      888          .8"888.     Y88bo.       888  d8'    Y88bo.      //
//      888         .8' `888.     `"Y8888o.   88888[       `"Y8888o.  //
//      888        .88ooo8888.        `"Y88b  888`88b.         `"Y88b //
//      888       .8'     `888.  oo     .d8P  888  `88b.  oo     .d8P //
//     o888o     o88o     o8888o 8""88888P'  o888o  o888o 8""88888P'  //                                                        

// --- Function to ensure the tasks file exists ---
void ensureTasksFileExists() {
  if (!SPIFFS.exists("/tasks.txt")) {
    Serial.println("'/tasks.txt' not found, creating empty file.");
    File file = SPIFFS.open("/tasks.txt", FILE_WRITE);
    if (!file) {
      Serial.println("Failed to create '/tasks.txt'");
    } else {
      // Optionally write a header or leave it empty
      // file.println("# TaskName|DueDate(YYYY-MM-DD)|Priority|Completed(Y/N)");
      file.close();
      Serial.println("Empty '/tasks.txt' created.");
    }
  }
}

void addTask(String taskName, String dueDate, String priority, String completed) {
  String taskInfo = taskName+"|"+dueDate+"|"+priority+"|"+completed;
  updateTaskArray(); // Load existing tasks first
  tasks.push_back({taskName, dueDate, priority, completed});
  updateTasksFile();
}

void updateTaskArray() {
  ensureTasksFileExists(); // Make sure the file exists before trying to read

  File file = SPIFFS.open("/tasks.txt", "r"); // Open the text file in read mode
  if (!file) {
    Serial.println("Failed to open tasks file for reading"); // Corrected message
    return;
  }

  tasks.clear(); // Clear the existing vector before loading the new data

  // Loop through the file, line by line
  while (file.available()) {
    String line = file.readStringUntil('\n');  // Read a line from the file
    line.trim();  // Remove any extra spaces or newlines
    
    // Skip empty lines
    if (line.length() == 0) {
      continue;
    }

    // Split the line into individual parts using the delimiter '|'
    uint8_t delimiterPos1 = line.indexOf('|');
    uint8_t delimiterPos2 = line.indexOf('|', delimiterPos1 + 1);
    uint8_t delimiterPos3 = line.indexOf('|', delimiterPos2 + 1);

    // Extract task name, due date, priority, and completed status
    String taskName  = line.substring(0, delimiterPos1);
    String dueDate   = line.substring(delimiterPos1 + 1, delimiterPos2);
    String priority  = line.substring(delimiterPos2 + 1, delimiterPos3);
    String completed = line.substring(delimiterPos3 + 1);

    // Add the task to the vector
    tasks.push_back({taskName, dueDate, priority, completed});
  }

  file.close();  // Close the file
}

void updateTasksFile() {
  ensureTasksFileExists(); // Make sure the file exists before trying to write/delete

  // Clear the existing tasks file first
  delFile("/tasks.txt");

  // Iterate through the tasks vector and append each task to the file
  for (size_t i = 0; i < tasks.size(); i++) {
    // Create a string from the task's attributes with "|" delimiter
    String taskInfo = tasks[i][0] + "|" + tasks[i][1] + "|" + tasks[i][2] + "|" + tasks[i][3];
    
    // Append the task info to the file
    appendToFile("/tasks.txt", taskInfo);
  }
}

void processKB_TASKS() {

}

void einkHandler_TASKS() {

}