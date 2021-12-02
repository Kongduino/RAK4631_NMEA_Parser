#undef max
#undef min
#include <string>
#include <vector>

using namespace std;

template class basic_string<char>; // https://github.com/esp8266/Arduino/issues/1136
// Required or the code won't compile!
namespace std _GLIBCXX_VISIBILITY(default) {
_GLIBCXX_BEGIN_NAMESPACE_VERSION
void __throw_length_error(char const*) {}
void __throw_bad_alloc() {}
void __throw_out_of_range(char const*) {}
void __throw_logic_error(char const*) {}
void __throw_out_of_range_fmt(char const*, ...) {}
}

#include <Arduino.h>
#include <Adafruit_TinyUSB.h>
#include <bluefruit.h>

char buffer[256];
uint8_t ix = 0;
vector<string> userStrings;
char UTC[7] = {0};
uint8_t SIV = 0;
float latitude, longitude;
BLEUart g_BleUart;
/** Flag if BLE UART client is connected */
bool g_BleUartConnected = false;
double lastFix = 0;
double fixInterval = 30000;

float parseDegrees(const char *term) {
  float value = (float)(atof(term) / 100.0);
  uint16_t left = (uint16_t)value;
  value = (value - left) * 1.66666666666666;
  value += left;
  return value;
}

void logLatLong(char *thisLat, char NS, char *thisLon, char EW) {
  float newLatitude, newLongitude;
  newLatitude = parseDegrees(thisLat);
  if (NS == 'S') newLatitude *= -1;
  newLongitude = parseDegrees(thisLon);
  if (EW == 'W') newLongitude *= -1;
  double thisFix = millis();
  int32_t lat0, lat1, lon0, lon1;
  lat0 = (int32_t)(latitude * 10000);
  lat1 = (int32_t)(newLatitude * 10000);
  lon0 = (int32_t)(longitude * 10000);
  lon1 = (int32_t)(newLongitude * 10000);
  // limit precision to 4 digits after the decimal
  if ((thisFix - lastFix > fixInterval) || lon0 != lon1 || lat0 != lat1) {
    // show log if GPS coords are different [enough], of it it's been more than fixInterval milliseconds
    lastFix = thisFix;
    latitude = newLatitude;
    longitude = newLongitude;
    // if we were over fixInterval, reset lastFix
    sprintf(buffer, "Lat/Long: %3.8f %c, %3.8f %c", latitude, NS, longitude, EW);
    Serial.println(buffer);
    if (g_BleUartConnected) g_BleUart.println(buffer);
  }
}

vector<string> parseNMEA(string nmea) {
  vector<string>result;
  if (nmea.at(0) != '$') {
    Serial.println("Not an NMEA sentence!");
    return result;
  }
  size_t lastFound = 0;
  size_t found = nmea.find(",", lastFound);
  while (found < nmea.size() && found != string::npos) {
    string token = nmea.substr(lastFound, found - lastFound);
    result.push_back(token);
    lastFound = found + 1;
    found = nmea.find(",", lastFound);
  }
  string token = nmea.substr(lastFound, found - lastFound);
  result.push_back(token);
  lastFound = found + 1;
  found = nmea.find(",", lastFound);
  return result;
}

void parseGPRMC(vector<string>result) {
  if (result.size() < 6) return;
  if (result.at(1) != "") {
    Serial.print("UTC Time: ");
    sprintf(buffer, "%s:%s:%s UTC", result.at(1).substr(0, 2).c_str(), result.at(1).substr(2, 2).c_str(), result.at(1).substr(4, 2).c_str());
    Serial.println(buffer);
    if (g_BleUartConnected) g_BleUart.println(buffer);
  }
  // if (result.at(2) == "V") Serial.println("Invalid fix!");
  // else Serial.println("Valid fix!");
  if (result.at(3) != "") {
    logLatLong((char*)result.at(3).c_str(), result.at(4).c_str()[0], (char*)result.at(5).c_str(), result.at(6).c_str()[0]);
    // float newLatitude, newLongitude;
    // newLatitude = parseDegrees(result.at(3).c_str());
    // newLongitude = parseDegrees(result.at(5).c_str());
    // if (newLatitude != latitude || newLongitude != longitude) {
    //   latitude = newLatitude;
    //   longitude = newLongitude;
    //   sprintf(buffer, "Lat/Long: %3.8f %c, %3.8f %c\n", latitude, result.at(4).c_str()[0], longitude, result.at(6).c_str()[0]);
    //   Serial.print(buffer);
    //   if (g_BleUartConnected) g_BleUart.println(buffer);
    // }
  }
}

void parseGPZDA(vector<string>result) {
  if (result.size() < 7) return;
  if (result.at(1) != "") {
    Serial.print("UTC Time: ");
    sprintf(buffer, "%s:%s:%s UTC", result.at(1).substr(0, 2).c_str(), result.at(1).substr(2, 2).c_str(), result.at(1).substr(4, 2).c_str());
    Serial.println(buffer);
    if (g_BleUartConnected) g_BleUart.println(buffer);
  }
  if (result.at(2) == "" || result.at(3) == "" || result.at(4) == "") return;
  Serial.print("UTC Date: ");
  sprintf(buffer, "%s/%s/%s", result.at(4).c_str(), result.at(3).c_str(), result.at(2).c_str());
  Serial.println(buffer);
  if (g_BleUartConnected) g_BleUart.println(buffer);
  if (result.at(5) == "" || result.at(6) == "" || result.at(4) == "") return;
  Serial.print("Local Time Zone: ");
  sprintf(buffer, "%s.%s hours", result.at(5).c_str(), result.at(6).c_str());
  Serial.println(buffer);
}

void parseGPGGA(vector<string>result) {
  if (result.size() < 14) return;
  if (result.at(1) != "") {
    Serial.print("UTC Time: ");
    Serial.print(result.at(1).substr(0, 2).c_str());
    Serial.print(":");
    Serial.print(result.at(1).substr(2, 2).c_str());
    Serial.print(":");
    Serial.println(result.at(1).substr(4, 2).c_str());
  }
  //  if (result.at(6) == "0") Serial.println("Invalid fix!");
  //  else Serial.println("Valid fix!");
  if (result.at(2) != "") {
    logLatLong((char*)result.at(2).c_str(), result.at(3).c_str()[0], (char*)result.at(4).c_str(), result.at(5).c_str()[0]);
    // latitude = parseDegrees(result.at(2).c_str());
    // longitude = parseDegrees(result.at(4).c_str());
    // sprintf(buffer, "Coordinates: %3.8f %c, %3.8f %c\n", latitude, result.at(3).c_str()[0], longitude, result.at(5).c_str()[0]);
    // Serial.print(buffer);
    // if (g_BleUartConnected) g_BleUart.println(buffer);
  }
}

void parseGPGLL(vector<string>result) {
  if (result.at(1) != "") {
    logLatLong((char*)result.at(1).c_str(), result.at(2).c_str()[0], (char*)result.at(3).c_str(), result.at(4).c_str()[0]);
    // latitude = parseDegrees(result.at(1).c_str());
    // longitude = parseDegrees(result.at(3).c_str());
    // sprintf(buffer, "Coordinates: %3.8f %c, %3.8f %c\n", latitude, result.at(2).c_str()[0], longitude, result.at(4).c_str()[0]);
    // Serial.print(buffer);
    // if (g_BleUartConnected) g_BleUart.println(buffer);
  }
}

void parseGPGSV(vector<string>result) {
  if (result.at(1) != "") {
    uint8_t newSIV = atoi(result.at(3).c_str());
    if (SIV != newSIV) {
      sprintf(buffer, "Message %s / %s. SIV: %s\n", result.at(2).c_str(), result.at(1).c_str(), result.at(3).c_str());
      Serial.print(buffer);
      if (g_BleUartConnected) g_BleUart.println(buffer);
      SIV = newSIV;
    }
  }
}

void parseGPTXT(vector<string>result) {
  //$GPTXT, 01, 01, 02, ANTSTATUS = INIT
  if (result.at(1) != "") {
    sprintf(buffer, " . Message %s / %s. Severity: %s\n . Message text: %s\n",
            result.at(2).c_str(), result.at(1).c_str(), result.at(3).c_str(), result.at(4).c_str(), result.at(5).c_str());
    Serial.print(buffer);
    if (g_BleUartConnected) g_BleUart.println(buffer);
  }
}

void parseGPVTG(vector<string>result) {
  Serial.println("Track Made Good and Ground Speed.");
  if (result.at(1) != "") {
    sprintf(buffer, " . True track made good %s [%s].\n", result.at(1).c_str(), result.at(2).c_str());
    Serial.print(buffer);
  }
  if (result.at(3) != "") {
    sprintf(buffer, " . Magnetic track made good %s [%s].\n", result.at(3).c_str(), result.at(4).c_str());
    Serial.print(buffer);
  }
  if (result.at(5) != "") {
    sprintf(buffer, " . Speed: %s %s.\n", result.at(5).c_str(), result.at(6).c_str());
    Serial.print(buffer);
  }
  if (result.at(7) != "") {
    sprintf(buffer, " . Speed: %s %s.\n", result.at(7).c_str(), result.at(8).c_str());
    Serial.print(buffer);
  }
}

void parseGPGSA(vector<string>result) {
  // $GPGSA,A,3,15,29,23,,,,,,,,,,12.56,11.96,3.81
  Serial.println("GPS DOP and active satellites");
  if (result.at(1) == "A") Serial.println(" . Mode: Automatic");
  else if (result.at(1) == "M") Serial.println(" . Mode: Manual");
  else Serial.println(" . Mode: ???");
  if (result.at(2) == "1") {
    Serial.println(" . Fix not available.");
    return;
  } else if (result.at(2) == "2") Serial.println(" . Fix: 2D");
  else if (result.at(2) == "3") Serial.println(" . Fix: 3D");
  else {
    Serial.println(" . Fix: ???");
    return;
  }
  Serial.print(" . PDOP: "); Serial.println(result.at(result.size() - 3).c_str());
  Serial.print(" . HDOP: "); Serial.println(result.at(result.size() - 2).c_str());
  Serial.print(" . VDOP: "); Serial.println(result.at(result.size() - 1).c_str());
}

/**
   @brief  Callback when client connects
   @param  conn_handle: Connection handle id
*/
void ble_connect_callback(uint16_t conn_handle) {
  (void)conn_handle;
  g_BleUartConnected = true;
  Serial.println("BLE client connected");
}

/**
   @brief  Callback invoked when a connection is dropped
   @param  conn_handle: connection handle id
   @param  reason: disconnect reason
*/
void ble_disconnect_callback(uint16_t conn_handle, uint8_t reason) {
  (void)conn_handle;
  (void)reason;
  g_BleUartConnected = false;
  Serial.println("BLE client disconnected");
}

void setup() {
  Serial.begin(115200);
  time_t timeout = millis();
  while (!Serial) {
    if ((millis() - timeout) < 5000) {
      delay(100);
    } else {
      break;
    }
  }
  delay(2500);
  Serial.println("\nGPS Example (NMEA Parser)");
  Serial.println("Turning off port");
  pinMode(WB_IO2, OUTPUT); // SLOT_A SLOT_B
  digitalWrite(WB_IO2, 0);
  delay(100);
  Serial.println("Turning on port");
  digitalWrite(WB_IO2, 1);
  delay(100);
  Serial1.begin(9600);
  delay(100);
  Serial.println("Serial1 ready!");
  Bluefruit.configPrphBandwidth(BANDWIDTH_MAX);
  Bluefruit.configPrphConn(92, BLE_GAP_EVENT_LENGTH_MIN, 16, 16);
  Bluefruit.begin(1, 0);
  // Set max power. Accepted values are: -40, -30, -20, -16, -12, -8, -4, 0, 4
  Bluefruit.setTxPower(4);
  // Set the BLE device name
  Bluefruit.setName("RAK_GPS_BLE");
  Bluefruit.Periph.setConnectCallback(ble_connect_callback);
  Bluefruit.Periph.setDisconnectCallback(ble_disconnect_callback);
  // Configure and Start BLE Uart Service
  g_BleUart.begin();
  // Set up and start advertising
  // Advertising packet
  Bluefruit.Advertising.addFlags(BLE_GAP_ADV_FLAGS_LE_ONLY_GENERAL_DISC_MODE);
  Bluefruit.Advertising.addTxPower();
  Bluefruit.Advertising.addName();
  Bluefruit.Advertising.restartOnDisconnect(true);
  Bluefruit.Advertising.setInterval(32, 244); // in unit of 0.625 ms
  Bluefruit.Advertising.setFastTimeout(30); // number of seconds in fast mode
  Bluefruit.Advertising.start(0); // 0 = Don't stop advertising after n seconds
}

bool waitForDollar = true;

void loop() {
  if (Serial1.available()) {
    char c = Serial1.read();
    if (waitForDollar && c == '$') {
      waitForDollar = false;
      buffer[0] = '$';
      ix = 1;
    } else if (waitForDollar == false) {
      if (c == 13) {
        buffer[ix] = 0;
        c = Serial1.read();
        delay(50);
        string nextLine = string(buffer);
        userStrings.push_back(nextLine.substr(0, nextLine.size() - 3));
        waitForDollar = true;
      } else {
        buffer[ix++] = c;
      }
    }
  }
  if (userStrings.size() > 0) {
    string nextLine = userStrings[0];
    userStrings.erase(userStrings.begin());
    if (nextLine.substr(0, 1) != "$") {
      // Serial.print("Not an NMEA string!\n>> ");
      // Serial.println(nextLine.c_str());
      return;
    } else {
      vector<string>result = parseNMEA(nextLine);
      if (result.size() == 0) return;
      string verb = result.at(0);
      if (verb.substr(3, 3) == "RMC") {
        parseGPRMC(result);
      } else if (verb.substr(3, 3) == "GSV") {
        parseGPGSV(result);
      } else if (verb.substr(3, 3) == "GGA") {
        parseGPGGA(result);
      } else if (verb.substr(3, 3) == "ZDA") {
        parseGPZDA(result);
      } else if (verb.substr(3, 3) == "GLL") {
        parseGPGLL(result);
      } else if (verb.substr(3, 3) == "GSA") {
        parseGPGSA(result);
      } else if (verb.substr(3, 3) == "VTG") {
        parseGPVTG(result);
      } else if (verb.substr(3, 3) == "TXT") {
        parseGPTXT(result);
      } else {
        Serial.println(nextLine.c_str());
      }
    }
  }
}
