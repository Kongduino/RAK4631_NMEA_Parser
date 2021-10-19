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

char buffer[256];
uint8_t ix = 0;
vector<string> userStrings;
char UTC[7] = {0};
uint8_t SIV = 0;

struct RawDegrees {
    uint16_t deg;
    uint32_t billionths;
    bool negative;
  public:
    RawDegrees() : deg(0), billionths(0), negative(false) {}
};

RawDegrees latitude, longitude;

void parseDegrees(const char *term, RawDegrees &deg) {
  uint32_t leftOfDecimal = (uint32_t)atol(term);
  uint16_t minutes = (uint16_t)(leftOfDecimal % 100);
  uint32_t multiplier = 10000000UL;
  uint32_t tenMillionthsOfMinutes = minutes * multiplier;
  deg.deg = (int16_t)(leftOfDecimal / 100);
  while (isdigit(*term))
    ++term;
  if (*term == '.')
    while (isdigit(*++term)) {
      multiplier /= 10;
      tenMillionthsOfMinutes += (*term - '0') * multiplier;
    }
  deg.billionths = (5 * tenMillionthsOfMinutes + 1) / 3;
  deg.negative = false;
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
  if (result.at(1) != "") {
    Serial.print("UTC Time: ");
    Serial.print(result.at(1).substr(0, 2).c_str());
    Serial.print(":");
    Serial.print(result.at(1).substr(2, 2).c_str());
    Serial.print(":");
    Serial.println(result.at(1).substr(4, 2).c_str());
  }
  if (result.at(2) == "V") {
    Serial.println("Invalid fix!");
  } else Serial.println("Valid fix!");
}

void parseGPGGA(vector<string>result) {
  if (result.at(1) != "") {
    Serial.print("UTC Time: ");
    Serial.print(result.at(1).substr(0, 2).c_str());
    Serial.print(":");
    Serial.print(result.at(1).substr(2, 2).c_str());
    Serial.print(":");
    Serial.println(result.at(1).substr(4, 2).c_str());
  }
  if (result.at(6) == "0") {
    Serial.println("Invalid fix!");
  } else Serial.println("Valid fix!");
  if (result.at(2) != "") {
    parseDegrees(result.at(2).c_str(), latitude);
    Serial.print("Coordinates: ");
    Serial.print(latitude.deg);
    Serial.write('.');
    Serial.print(latitude.billionths);
    Serial.print(result.at(3).c_str());
    Serial.print(", ");
    parseDegrees(result.at(4).c_str(), longitude);
    Serial.print(longitude.deg);
    Serial.write('.');
    Serial.print(longitude.billionths);
    Serial.println(result.at(5).c_str());
  }
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
  Serial.println("\nGPS Example(I2C)");
  //  pinMode(WB_IO1, OUTPUT); // SLOT_A SLOT_B
  Serial.println("Turning off port");
  pinMode(WB_IO2, OUTPUT); // SLOT_A SLOT_B
  digitalWrite(WB_IO2, 0);
  delay(100);
  Serial.println("Turning on port");
  digitalWrite(WB_IO2, 1);
  delay(100);
  Serial2.begin(9600);
  delay(100);
  Serial.println("Serial2 ready!");
}

bool waitForDollar = true;

void loop() {
  if (Serial2.available()) {
    char c = Serial2.read();
    if (waitForDollar && c == '$') {
      waitForDollar = false;
      buffer[0] = '$';
      ix = 1;
    } else if (waitForDollar == false) {
      if (c == 13) {
        buffer[ix] = 0;
        c = Serial2.read();
        delay(50);
        string nextLine = string(buffer);
        //Serial.print("Appending: {");
        //Serial.print(nextLine.c_str());
        //Serial.println("}");
        userStrings.push_back(nextLine);
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
      Serial.print("Not an NMEA string!\n>> ");
      Serial.println(nextLine.c_str());
    } else {
      vector<string>result = parseNMEA(nextLine);
      Serial.print("Number of tokens: ");
      Serial.println(result.size());
      string verb = result.at(0);
      if (verb == "$GPRMC") {
        Serial.println(nextLine.c_str());
        parseGPRMC(result);
      } else if (verb == "$GPGGA") {
        Serial.println(nextLine.c_str());
        parseGPGGA(result);
      }
    }
  }
}
