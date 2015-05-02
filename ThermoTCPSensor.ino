#define RECONNECT_DELAY 200

// telnet defaults to port 23
TCPServer server = TCPServer(23);
TCPClient client;

char myIpString[24];

int bufMax = 12;
int count = 0;
double temperature = 0.0;

bool printLogs = false;
int numLogs;

int iter = 0;

unsigned long lastTime = millis();

void setup()
{
  // Start listening for clients
  server.begin();

  // Make IP address accessable over cloud functions
  IPAddress myIp = WiFi.localIP();
  sprintf(myIpString, "%d.%d.%d.%d", myIp[0], myIp[1], myIp[2], myIp[3]);
  Spark.variable("ipAddress", myIpString, STRING);

  // Assign temp sensor pin
  pinMode(A7, INPUT);

}

void loop()
{
    while (client.connected()) {
        // Read current temperature
        readTempSensor();

        // Check if there is input to consume
        if (int count = client.available()) {
            String reqStr = readSocket(count);
            consumeInput(reqStr);
        }

        // Write the temperature data if TCP logging is enabled
        if (printLogs && numLogs <= 201) {
            sendTempData(temperature);
            numLogs++;// Automatically stop after 201 readings
        }

        // Touch base with wireless connection, or else the CC3000 tends to lose contact during TCP operations
        SPARK_WLAN_Loop();
    }

    if (!client.connected()) {
        client = server.available();
        if (printLogs) {
            cleanUp();
        }
    }

    readTempSensor();
    delay(RECONNECT_DELAY);
}

// Read the message from the socket connection
String readSocket(int count) {
    // Cut off the input at 12 bytes
    if (count > bufMax) {
         count = bufMax;
    }

    char charBuf[count+1];
    int i;
    // Form a char array from the input
    for (i = 0; i < count; ++i) {
        charBuf[i] = client.read();
    }
    // Format charBuf to be a valid string
    charBuf[i] = '\0';
    client.flush();
    return charBuf;
}

// Updates the current temperatue value, in Celcius
void readTempSensor() {
    int reading = 0;
    double voltage = 0.0;

    // Take the raw sensor reading
    reading = analogRead(A7);

    // Calculate the voltage from the sensor reading
    voltage = (reading * 3.3) / 4095;

    // Calculate the temperature
    temperature = (voltage - 0.5) * 100;
}

// Switch on and off the real-time temperature logging
void consumeInput(String reqStr) {
    if (reqStr == "TempLogs") {
        printLogs = true;
        numLogs = 0;
    }
    else {
        //Disable temperature logging if any other input is recieved
        writeSocket("Logging Disabled");
        cleanUp();
    }
}

// Write a string to the socket connection
void writeSocket(String resp) {
    char respArray[resp.length() + 1];
    resp.toCharArray(respArray, resp.length() + 1);
    server.write(respArray);
}

// Reset variables governing printing data to sockets
void cleanUp() {
    printLogs = false;
    numLogs = -1;
}

// Send temperature data over the socket connection
void sendTempData(double temperature) {
    // Convert the temp value to an int for more effiecient transmition
    int tempVal = (temperature * 1000);

     byte intBytes[2];
     intBytes[0] = tempVal;
     intBytes[1] = tempVal >> 8;
     server.write(intBytes, 2);
}