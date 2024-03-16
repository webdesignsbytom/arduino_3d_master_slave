#include <Wire.h>
#include <LiquidCrystal.h>

// Button pin assignments
const int buttonUp = 6, buttonDown = 7, buttonEnter = 8, buttonBack = 9;

// LCD pin assignments and initialization
const int rs = 12, en = 11, d4 = 5, d5 = 4, d6 = 3, d7 = 2;
LiquidCrystal lcd(rs, en, d4, d5, d6, d7);

// Menu items
String mainMenuItems[] = {"SD Card", "Motor Control", "Tests", "Shutdown"};
String motorControlMenu[] = {"Speed+", "Speed-", "Release", "Home axis", "Back"};
String testsMenu[] = {"Slave Test", "Coms Test", "Back"};
String shutdownMenu[] = {"Confirm Shutdown", "Back"};

String* currentMenu = mainMenuItems; // Start with main menu

int menuItemCount = 4;
int currentMenuItem = 0; // variable to track the current menu item

// SD Data
String fileNames[10]; // Assuming a max of 10 files
int fileCount = 0;

// Menu state
enum MenuState {MAIN_MENU, SD_CARD_MENU, MOTOR_CONTROL_MENU, TESTS_MENU, SHUTDOWN_MENU};
MenuState currentMenuState = MAIN_MENU;

void setup() {
  Serial.begin(115200);

  Wire.begin(); // Join I2C bus as master

  lcd.begin(16, 2);  
  pinMode(buttonUp, INPUT_PULLUP);
  pinMode(buttonDown, INPUT_PULLUP);
  pinMode(buttonEnter, INPUT_PULLUP);
  pinMode(buttonBack, INPUT_PULLUP);

  lcd.clear();
  lcd.print("> ");
  lcd.print(mainMenuItems[currentMenuItem]); 

  // Let's fix the next part:
  int nextMenuItem = currentMenuItem + 1;
  if(nextMenuItem >= menuItemCount) {
    nextMenuItem = 0; // Wrap around to the first item
  }

  // Display the next menu item on the second row
  lcd.setCursor(2, 1); // Set cursor to the second column of the second row to align visually
  lcd.print(currentMenu[nextMenuItem]); // Corrected to use 'currentMenu' instead of 'menuItems'
}

void loop() {
  static unsigned long lastPress = 0;
  if (millis() - lastPress > 200) { // Basic debouncing

    if (digitalRead(buttonUp) == LOW) {
      navigateMenu(-1); // Navigate up
      lastPress = millis();
    } else if (digitalRead(buttonDown) == LOW) {
      navigateMenu(1); // Navigate down
      lastPress = millis();
    } else if (digitalRead(buttonEnter) == LOW) {
      enterMenu(); // Enter into a submenu or execute an option
      lastPress = millis();
    } else if (digitalRead(buttonBack) == LOW) {

      // When the back button is pressed, return to the main menu
      if (currentMenuState != MAIN_MENU) { // Check to avoid resetting if already in the main menu
        currentMenuState = MAIN_MENU;
        currentMenu = mainMenuItems; // Reset the current menu to the main menu
        menuItemCount = sizeof(mainMenuItems) / sizeof(mainMenuItems[0]); // Reset menu item count to the main menu's count
        currentMenuItem = 0; // Reset the current menu item index to 0
        updateMenuDisplay(); // Update the display to show the main menu
        lastPress = millis();
      }
    }
  }
}


void navigateMenu(int direction) {
  currentMenuItem += direction;
  if (currentMenuItem < 0) {
    currentMenuItem = menuItemCount - 1;
  } else if (currentMenuItem >= menuItemCount) {
    currentMenuItem = 0;
  }
  updateMenuDisplay();
}

void enterMenu() {
  // Handle entering submenus or executing options
  if (currentMenuState == MAIN_MENU) {
    switch (currentMenuItem) {
      case 0:
        requestSDCardFileListing();
        break;
      case 1:
        currentMenuState = MOTOR_CONTROL_MENU;
        currentMenu = motorControlMenu;
        menuItemCount = sizeof(motorControlMenu) / sizeof(motorControlMenu[0]);
        break;
      case 2: // "Tests" menu selected
        currentMenuState = TESTS_MENU;
        currentMenu = testsMenu;
        menuItemCount = sizeof(testsMenu) / sizeof(testsMenu[0]);
        break;
      case 3:
        currentMenuState = SHUTDOWN_MENU;
        currentMenu = shutdownMenu;
        menuItemCount = sizeof(shutdownMenu) / sizeof(shutdownMenu[0]);
        break;
    }
    currentMenuItem = 0; // Reset to the top of the submenu
    updateMenuDisplay();
  } else if (currentMenuState == TESTS_MENU) {
    if (currentMenuItem == 0) { // "Slave Test" selected
      Wire.beginTransmission(4); // Begin transmission to device with I2C address 4
      Wire.write(1); // Send a signal (e.g., byte '1') to indicate "Slave Test" action
      Wire.endTransmission();
    } // End transmission
    else if (currentMenuItem == 1) { // "Coms Test" selected
      Wire.beginTransmission(4); 
      Wire.write(2); // Signal to request the "hello" message
      Wire.endTransmission();
      displayHelloMessageFromSlave();
    }
    else if (currentMenu[currentMenuItem] == "Back") {
      currentMenuState = MAIN_MENU;
      currentMenu = mainMenuItems;
      menuItemCount = sizeof(mainMenuItems) / sizeof(mainMenuItems[0]);
      currentMenuItem = 0;
      updateMenuDisplay();
    }
    // No else needed here as we're handling all conditions within TESTS_MENU
  }
  // If not in the MAIN_MENU and a specific action was executed, you might want to update the display or take other actions
}

void displayHelloMessageFromSlave() {
  lcd.clear();
  lcd.setCursor(0, 0);

  Wire.requestFrom(4, 32); // Request up to 32 bytes, but you might not use all.
  char receivedMessage[33]; // Buffer for the message, assuming maximum length + 1 for terminator
  int i = 0;

  while (Wire.available()) {
    char c = Wire.read(); // Read a byte
    if (c == '\0' || i == 32) { // Check for the end of the message or buffer full
      break; // Exit the loop
    }
    receivedMessage[i] = c;
    i++;
  }
  receivedMessage[i] = '\0'; // Ensure the string is properly terminated

  lcd.print(receivedMessage); // Display the message
}

// void requestSDCardFileListing() {
//   Wire.beginTransmission(4); // Slave address
//   Wire.write(2); // Signal for SD card listing
//   Wire.endTransmission();

//   // Immediately start polling for file names
//   displayFileNamesFromSlave();
// }

void requestSDCardFileListing() {
  Wire.beginTransmission(4); // Slave address
  Wire.write(2); // Signal for SD card listing
  Wire.endTransmission();

  // Reset file list
  fileCount = 0;
  bool isReceiving = true;

  while (isReceiving && fileCount < 10) { // Prevent overflow
    Wire.requestFrom(4, 32); // Adjust size as needed
    String fileName = "";
    while (Wire.available()) {
      char c = Wire.read();
      if (c == '\0') { // Check for the end of a file name
        if (fileName.length() == 0) {
          isReceiving = false; // Empty filename signals end of the list
          break;
        }
        fileNames[fileCount++] = fileName; // Add to array
        fileName = ""; // Reset for the next file name
      } else {
        fileName += c;
      }
    }
  }

  // Update the menu to display these file names
  currentMenu = fileNames;
  menuItemCount = fileCount;
  currentMenuItem = 0;
  updateMenuDisplay();
}

void displayFileNamesFromSlave() {
  lcd.clear();
  lcd.setCursor(0, 0);
  bool isReceiving = true; // Assume we have data to receive
  while (isReceiving) {
    Wire.requestFrom(4, 32); // Request up to 32 bytes from slave device #4
    isReceiving = false; // Assume this is the last batch unless we hit the marker
    while (Wire.available()) { // Slave may send less than requested
      char c = Wire.read(); // Receive a byte as character
      if (c == '\0') { // Null char might be used to signal the end of data
        isReceiving = false; // No more data to read
        break;
      } else {
        lcd.write(c); // Print the character on the LCD
        isReceiving = true; // Still receiving data
      }
    }
    delay(100); // Simple delay to allow for reading and display refresh
  }
}


void updateMenuDisplay() {
  lcd.clear();
  lcd.print("> ");
  lcd.print(currentMenu[currentMenuItem]); // Displays the current selected item

  // Correctly calculate and display the next menu item
  int nextMenuItem = (currentMenuItem + 1) % menuItemCount;
  lcd.setCursor(2, 1); // Adjusted for second line
  if (menuItemCount > 1) { // Only try to display a second item if it exists
    lcd.print(currentMenu[nextMenuItem]); // Display the next item
  }
}
