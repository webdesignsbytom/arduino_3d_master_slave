#include <Wire.h>
#include <SPI.h>
#include <SD.h>

void setup() {
  Serial.begin(115200);

  Serial.print("SERIAL CONNECTING....");

  while (!Serial) {
    ; // wait for serial port to connect. Needed for native USB port only
  }

  pinMode(LED_BUILTIN, OUTPUT); // Initialize the built-in LED pin as an output // Test Function

  Serial.print("Initializing SD card...");

  if (!SD.begin(4)) {
    Serial.println("initialization failed!");
    while (1);
  }
  Serial.println("initialization done.");

  Wire.begin(4); // Join I2C bus with address #4
  Wire.onReceive(receiveEvent); // Register event

  File root = SD.open("/");
  readSdCard(root);
}

void loop() {
  delay(100);
}

// Function that executes whenever data is received from master
void receiveEvent(int howMany) {
  if (Wire.available()) {
    int signal = Wire.read(); // Receive signal as an integer
    if (signal == 1) {
      testSlave1();
    }
    else if (signal == 2) { // Signal for SD card listing
      Serial.print("COMMUNICATION ESTABLISHED");
      delay(100);

      Wire.onRequest(requestEvent); // register event
    }
  }
}

void requestEvent() {
  // Explicitly include the null terminator in the message
  static const char message[] = "Connected!"; // Your message
  static const int messageLength = sizeof(message); // Includes null terminator because of static array declaration

  Wire.write(message, messageLength); // Send "hello" followed by '\0'
}


void readSdCard(File dir) {
  while (true) {
    File entry = dir.openNextFile();
    if (!entry) {
      // No more files
      break;
    }
    if (entry.isDirectory()) {
      // Skip directories or recursively call sendFileNames(entry) if you want to include them
    } else {
      // Send the file name
      Serial.println(entry.name());
    }
    entry.close();
  }
}

void sendFileNamesToMaster() {
  File root = SD.open("/");
  File entry = root.openNextFile();

  while (entry) {
    if (!entry.isDirectory()) {
      sendFileNameToMaster(entry.name());
    }
    entry.close();
    entry = root.openNextFile();
  }
  // Signal the end of transmission with a special message, such as an empty string
  sendFileNameToMaster("");
}

void sendFileNameToMaster(const char* fileName) {
  Wire.beginTransmission(4); // Slave address of the master
  const byte startMarker = 0x02; // Start of text
  const byte endMarker = 0x03; // End of text
  Wire.write(startMarker); // Indicate start of file name
  
  // Send file name in chunks  
  while (*fileName) {
    Wire.write(*fileName++);
  }
  
  Wire.write(endMarker); // Indicate end of file name
  Wire.endTransmission();
  delay(100); // Simple flow control - wait a bit before the next operation
}

void testSlave1() {
  Serial.print("TEST SLAVE 1");
  digitalWrite(LED_BUILTIN, HIGH); // Turn the LED on
  delay(500);                      // Wait for half a second
  digitalWrite(LED_BUILTIN, LOW);  // Turn the LED off
  delay(500);                      // Wait for half a second
}