import Cocoa

let addr = //PUT THE HOST IP ADDRESS HERE
let port = //PUT THE HOST PORT HERE

var host :NSHost = NSHost(address: addr)
var inp :NSInputStream?
var out :NSOutputStream?

var prevAvg = 0;
var prevSize = 0;

let bufferLim = 2;

func ThermoTCP() {
    // Open connection with sensor device
    NSStream.getStreamsToHostWithName(addr, port: port, inputStream: &inp, outputStream: &out);
    
    // Stream Init
    let inputStream = inp!;
    let outputStream = out!;
    inputStream.open();
    outputStream.open();
    
    // Send signal to start writing data
    writeData(outputStream, "TempLogs");
    
    // Calculate avg temperature over 60 samples
    getAvgTemperature(inputStream);
    
    // Send signal to stop writing data
    writeData(outputStream, "Stop");
    
    // Close socket connections
    outputStream.close();
    inputStream.close();
}

// Read data from socket connection to find the average reported temperature over 500 readings
func getAvgTemperature(inputStream: NSInputStream) {
    while (prevSize < 200) {
        let temperatureReading = getTemperatureReading(inputStream);
        
        // Calculate the new average temperature
        prevAvg = calculateNewAvg(temperatureReading);
        ++prevSize;
    }
    println("Final Avg Temperature: \(getFahrenheitTemperatureValue())");
}

// Read the current temperature value from the socket connection
func getTemperatureReading(inputStream: NSInputStream) -> Int {
    var readByte :UInt8 = 0
    var byteArray = [UInt8]();
    
    // Read 2 bytes from the input stream
    while (byteArray.count < bufferLim) {
        if (inputStream.hasBytesAvailable) {
            inputStream.read(&readByte, maxLength: 1);
            byteArray.append(readByte);
        }
        else {
            // If no new input wait .25 seconds
            usleep(250);
        }
    }
    
    // Convert the 2 bytes into an int
    let intResp: Int = Int(UnsafePointer<UInt16>(byteArray).memory);
    return intResp;
}

// Calculates what the average temperature will be given a new temperature data point
func calculateNewAvg(newItem: Int) -> Int {
    let prevAvgSum = prevAvg * prevSize;
    let newAvg = (prevAvgSum + newItem) / (Int(prevSize) + 1);
    return newAvg;
}

// Return the current avg temperature value converted to Fahrenheit
func getFahrenheitTemperatureValue() -> Double {
    //The temperature value was originally multiplied by 1000 to store as an int
    let celciusVal =  Double(prevAvg) / 1000;
    return (celciusVal * 1.8) + 32;
}

// Write the given string to the output stream
func writeData(outputStream: NSOutputStream, message: String) {
    outputStream.write(message, maxLength: message.lengthOfBytesUsingEncoding(NSUTF8StringEncoding));
}

ThermoTCP();
