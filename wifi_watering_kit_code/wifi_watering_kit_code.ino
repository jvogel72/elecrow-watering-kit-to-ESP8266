/** Code for Elecrow Watering Kit Board **/
#include <Wire.h>
#include "U8glib.h"
U8GLIB_SSD1306_130X64 u8g(U8G_I2C_OPT_NONE);    // I2C
#include "Wire.h"
#include "RTClib.h"
RTC_DS1307 RTC;
#include "WaterData.h"

// number of sensors and relays
const int sensors = 4;

// minimum moisture values below which the relay will open
int min_moisture[ sensors ] = {30, 30, 30, 30};

// maximum moisture values above which the relay will close
int max_moisture[ sensors ] = {55, 55, 55, 55};

// timeframe during which the system may operate
// to always operate, set to identical values 
int startHour = 8;
int startMinute = 0;
int endHour = 20;
int endMinute = 30;

// edit only when calibrating sensors

// resistance reported by sensor when detecting dry soil (higher is more dry)
int sensor_dry[ sensors ] = {600, 600, 600, 600};

// resistance reported by sensor when detecting wet soil (lower is more wet)
int sensor_wet[ sensors ] = {360, 360, 360, 360};

// internal veriables follow, no need to edit

// display text for operating time
char operateTime[13];

// set all moisture sensors PIN ID
int moisture[sensors] = {A0, A1, A2, A3};

// declare moisture values
int moisture_value[ sensors ] = {0, 0, 0, 0};

// set initial water level value
int water_level_value = 0;

// set water relays
int relay[ sensors ] = {6, 8, 9, 10};

// set water pump
int pump = 4;

// set button
int button = 12;

//pump state    1:open   0:close
int pump_state_flag = 0;

// relay state
//   true:  one or more relays are open
//   false: all relays are closed
bool relay_state_flag = false;

// for individual relays
// relay state_flags    1:open   0:close
int relay_state_flags[4];

//enable pump   1 = enabled   0 = disabled
int enable_pump = 1;

//Measuring water level with ultrasonic sensor HC-SR04
int trig = 1;   //pin D7
int echo = A4;  //pin A4

// output frequency for sending data to the ESP
TimeSpan outputFrequency = TimeSpan(0, 0, 1, 0);
DateTime nextOutput;

char daysOfTheWeek[7][12] = {"Sun", "Mon", "Tues", "Wed", "Thur", "Fri", "Sat",};

int t_start = startHour * 60 + startMinute;
int t_end = endHour * 60 + endMinute;

// read line from serial input
char serialRead[30];
int srPos = 0;
char *serialVars[7];
const char delimeters[] = " :-_,\t";

// read line from serial1 input
char serial1Read[50];
int sr1Pos = 0;
int serial1Vars[8];
char ESPcmd[2];

struct ESP_WATERING {
  char command[1];
  char moisture0[3];
  char moisture1[3];
  char moisture2[3];
  char moisture3[3];
  char relay0[1];
  char relay1[1];
  char relay2[1];
  char relay3[1];
  char pump[1];
  char waterlevel[3];
};

// good flower
static const unsigned char bitmap_good[] U8G_PROGMEM = {
  0x00, 0x42, 0x4C, 0x00, 0x00, 0xE6, 0x6E, 0x00, 0x00, 0xAE, 0x7B, 0x00, 0x00, 0x3A, 0x51, 0x00,
  0x00, 0x12, 0x40, 0x00, 0x00, 0x02, 0x40, 0x00, 0x00, 0x06, 0x40, 0x00, 0x00, 0x06, 0x40, 0x00,
  0x00, 0x04, 0x60, 0x00, 0x00, 0x0C, 0x20, 0x00, 0x00, 0x08, 0x30, 0x00, 0x00, 0x18, 0x18, 0x00,
  0x00, 0xE0, 0x0F, 0x00, 0x00, 0x80, 0x01, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x01, 0x00,
  0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x02, 0xC1, 0x00, 0x00, 0x0E, 0x61, 0x00,
  0x00, 0x1C, 0x79, 0x00, 0x00, 0x34, 0x29, 0x00, 0x00, 0x28, 0x35, 0x00, 0x00, 0x48, 0x17, 0x00,
  0x00, 0xD8, 0x1B, 0x00, 0x00, 0x90, 0x1B, 0x00, 0x00, 0xB0, 0x09, 0x00, 0x00, 0xA0, 0x05, 0x00,
  0x00, 0xE0, 0x07, 0x00, 0x00, 0xC0, 0x03, 0x00
};

// bad flower
static const unsigned char bitmap_bad[] U8G_PROGMEM = {
  0x00, 0x80, 0x00, 0x00, 0x00, 0xC0, 0x00, 0x00, 0x00, 0xE0, 0x0D, 0x00, 0x00, 0xA0, 0x0F, 0x00,
  0x00, 0x20, 0x69, 0x00, 0x00, 0x10, 0x78, 0x02, 0x00, 0x10, 0xC0, 0x03, 0x00, 0x10, 0xC0, 0x03,
  0x00, 0x10, 0x00, 0x01, 0x00, 0x10, 0x80, 0x00, 0x00, 0x10, 0xC0, 0x00, 0x00, 0x30, 0x60, 0x00,
  0x00, 0x60, 0x30, 0x00, 0x00, 0xC0, 0x1F, 0x00, 0x00, 0x60, 0x07, 0x00, 0x00, 0x60, 0x00, 0x00,
  0x00, 0x60, 0x00, 0x00, 0x00, 0x40, 0x00, 0x00, 0x00, 0xC0, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00,
  0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x80, 0x00, 0x00, 0x00, 0xC7, 0x1C, 0x00,
  0x80, 0x68, 0x66, 0x00, 0xC0, 0x33, 0x7B, 0x00, 0x40, 0xB6, 0x4D, 0x00, 0x00, 0xE8, 0x06, 0x00,
  0x00, 0xF0, 0x03, 0x00, 0x00, 0xE0, 0x00, 0x00
};

// Elecrow Logo
static const unsigned char bitmap_logo[] U8G_PROGMEM ={
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
  0x00,0x00,0x0F,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
  0x00,0xE0,0xFF,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
  0x04,0xF8,0xFF,0x03,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
  0x08,0xFE,0xFF,0x07,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
  0x10,0x1F,0xE0,0x0F,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
  0xB0,0x07,0x80,0x1F,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
  0xE0,0x03,0x00,0x3F,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
  0xC0,0x00,0x00,0x3E,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
  0x80,0x01,0x00,0x7E,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
  0x60,0x23,0x00,0x7C,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
  0x70,0xC7,0x00,0x7E,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
  0x70,0x9E,0x0F,0x7F,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
  0x70,0x3C,0xFE,0x7F,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
  0x70,0x78,0xF8,0x7F,0xF0,0x9F,0x07,0xFE,0x83,0x0F,0xFF,0x00,0x77,0x3C,0x18,0x1C,
  0x70,0xF0,0xE1,0x3F,0xF1,0x9F,0x07,0xFE,0xE1,0x1F,0xFF,0xC3,0xF7,0x3C,0x38,0x0C,
  0x70,0xE0,0x87,0x8F,0xF1,0xC0,0x07,0x1E,0x70,0x3C,0xCF,0xE3,0xE1,0x7D,0x3C,0x0E,
  0x70,0xD0,0x1F,0xC0,0xF1,0xC0,0x03,0x1F,0x78,0x3C,0xCF,0xE3,0xE1,0x7D,0x3C,0x06,
  0xF0,0xB0,0xFF,0xF1,0xF0,0xC0,0x03,0x0F,0x78,0x3C,0xCF,0xF3,0xE0,0x7B,0x3E,0x06,
  0xF0,0x60,0xFF,0xFF,0xF0,0xC6,0x03,0xEF,0x3C,0x80,0xEF,0xF1,0xE0,0x7B,0x3E,0x03,
  0xF0,0xE1,0xFC,0xFF,0xF8,0xCF,0x03,0xFF,0x3C,0x80,0xFF,0xF0,0xE0,0x7B,0x7B,0x01,
  0xE0,0xC3,0xF9,0x7F,0x78,0xC0,0x03,0x0F,0x3C,0x80,0xF7,0xF1,0xE0,0xF9,0xF9,0x01,
  0xE0,0x83,0xE3,0x7F,0x78,0xE0,0x03,0x0F,0x3C,0xBC,0xE7,0xF1,0xE0,0xF9,0xF9,0x00,
  0xC0,0x0F,0x8F,0x3F,0x78,0xE0,0x81,0x0F,0x3C,0x9E,0xE7,0xF1,0xE0,0xF1,0xF8,0x00,
  0x80,0x3F,0x1E,0x00,0x78,0xE0,0x81,0x07,0x38,0x9E,0xE7,0xF1,0xF0,0xF0,0x78,0x00,
  0x80,0xFF,0xFF,0x00,0xF8,0xEF,0xBF,0xFF,0xF8,0xCF,0xE7,0xE1,0x7F,0x70,0x70,0x00,
  0x00,0xFF,0xFF,0x0F,0xF8,0xEF,0xBF,0xFF,0xE0,0xC3,0xE3,0x81,0x1F,0x70,0x30,0x00,
  0x00,0xFC,0xFF,0x07,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
  0x00,0xF8,0xFF,0x01,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
  0x00,0xE0,0x7F,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00
};

void setup() {
  u8g.firstPage();
  do {
    draw_elecrow();
  } while ( u8g.nextPage() );
  delay(2000);
  Wire.begin();
  RTC.begin();
  Serial.begin(19200);
  while (!Serial) { }
  Serial1.begin(19200); // Serial to ESP8266. Use RX & TX pins of Elecrow relay board
  while (!Serial1) { }
  //Ultrasonic sensor
  pinMode(trig, OUTPUT);
  pinMode(echo, INPUT);
  // declare relay as output
  for (int i = 0; i < sensors; ++i)
    pinMode(relay[i], OUTPUT);
  // declare pump as output
  pinMode(pump, OUTPUT);
  // declare switch as input
  pinMode(button, INPUT);
  setOperateTime();
  nextOutput = RTC.now();
}

void loop() {
  long t = 0, h = 0, hp = 0; //for Measuring water level
   
  /** The pump is automatically disabled if water tank level low **/
  // read water level and shut off pump is too low
  // Transmitting pulse

  digitalWrite(trig, LOW);
  delayMicroseconds(2);
  digitalWrite(trig, HIGH);
  delayMicroseconds(10);
  digitalWrite(trig, LOW);
  
  // Waiting for pulse
  t = pulseIn(echo, HIGH);
  
  // Calculating distance 
  h = t / 58;
  h = h - 6;  // offset correction (adjust as necessary for your water tank)
  h = 50 - h;  // water height, 0 - 50 cm
  hp = 2 * h;  // distance in %, 0-100 %

  //check pump reservoir water level 
  water_level_value = hp; 
  delay(20);
  if (water_level_value < 0) {
    water_level_value = 0;
  }
  if (water_level_value >= 0) {
    enable_pump = 1;
  } else if (water_level_value < 0)  {
    enable_pump = 0;
  }
  read_value();
  water_flower();
  int button_state = digitalRead(button);
  if (button_state == 1) {
    read_value();
    u8g.firstPage();
    do {
      drawTH();
      drawflower();
    } while ( u8g.nextPage() );
  } else {
    u8g.firstPage();
    do {
      drawtime();
    } while (u8g.nextPage());
  }

  // for getting and setting the time through serial console
  while (Serial.available() > 0)
    readSerial();

  // ESP commands
  while (Serial1.available() > 0)
    readSerial1();
}

// Set moisture value based on sensor readout
//   600 is dry - value of 600 and above is 0%
//   360 is wet - value of 360 is 100%
void read_value() {
  char ESPString[26];
  for (int i = 0; i < sensors; ++i) {
    float value = analogRead(moisture[i]);
    moisture_value[i] = constrain(map(value, sensor_dry[i], sensor_wet[i], 0, 100), 0, 100);
    delay(20);
  }
  if (RTC.now() >= nextOutput) {
//  if (counter >= 470) {       //output frequency to ESP, 470 = approx 1 minute
    sprintf(ESPString, "%04d,%04d,%04d,%04d,%d,%04d", moisture_value[0], moisture_value[1], moisture_value[2], moisture_value[3], pump_state_flag, water_level_value);
    /*********Output Moisture Sensor values to ESP8266******/
    Serial1.println(ESPString);
//    delay(100);
//    counter = 0;
    nextOutput = RTC.now() + outputFrequency;
  
     /* Optional - to display on Arduino serial monitor */
/*
    char serialDebug[50];
    sprintf(serialDebug, "A0: %d, A1: %d, A2: %d, A3: %d, Pump: %d, Water Level: %d", moisture_value[0], moisture_value[1], moisture_value[2], moisture_value[3], pump_state_flag, water_level_value);
    Serial.println(serialDebug);
    delay(50); 
*/     
  }
}

void pump_on() {
  digitalWrite(pump, HIGH);
  pump_state_flag = 1;
  delay(50);
}

void pump_off() {
  digitalWrite(pump, LOW);
  pump_state_flag = 0;
  delay(50);
}

void pump_switch() {
  if (relay_state_flag) {
    if (pump_state_flag == 0 && enable_pump == 1) {
      pump_on();
    }
  } else {
    pump_off;  
  }
}

void water_flower() {
  relay_state_flag = false;
  for (int i = 0; i < sensors; ++i) {
    if (moisture_value[i] < min_moisture[i] && inTimeFrame()) {
      digitalWrite(relay[i], HIGH);
      relay_state_flags[i] = 1;
      relay_state_flag = true;
      delay(50);
    } else if (moisture_value[i] > max_moisture[i]) {
      digitalWrite(relay[i], LOW);
      relay_state_flags[i] = 0;
      delay(50);
    }
  }
  pump_switch();
}

void draw_elecrow(void) {
  u8g.setFont(u8g_font_gdr9r);
  u8g.setPrintPos(8, 55);
  u8g.print("www.elecrow.com");
  u8g.drawXBMP(0, 5, 128, 32, bitmap_logo);
}

void drawtime(void) {
  DateTime now = RTC.now();
  char t_time[8];
  char t_date[10];
  if (! RTC.isrunning()) {
    u8g.setFont(u8g_font_6x10);
    u8g.setPrintPos(5, 20);
    u8g.print("RTC is NOT running!");
    RTC.adjust(DateTime(__DATE__, __TIME__));
  } else {
    u8g.setFont(u8g_font_7x13);
    sprintf(t_date, "%d/%d/%d", now.year(), now.month(), now.day());
    u8g.setPrintPos(5, 11);
    u8g.print(t_date);
    u8g.setPrintPos(85, 11);
    u8g.print(daysOfTheWeek[now.dayOfTheWeek()]);
    u8g.setFont(u8g_font_8x13);
    sprintf(t_time, "%02d:%02d:%02d", now.hour(), now.minute(), now.second());
    u8g.setPrintPos(35, 33);
    u8g.print(t_time);
    u8g.setPrintPos(12, 55);
    u8g.print(operateTime);
  }
}

//Style the flowers     bitmap_bad: bad flowers     bitmap_good:good  flowers
void drawflower(void) {
  int flowerPosition[ sensors ] = {0, 32, 64, 96};
  for (int i = 0; i < sensors; ++i) {
    if (moisture_value[i] < 30)
      u8g.drawXBMP(flowerPosition[i], 0, 32, 30, bitmap_bad);
    else
      u8g.drawXBMP(flowerPosition[i], 0, 32, 30, bitmap_good);
  }
}

void drawTH(void) {
  char sensorName[2];
  char percentage[4];
  read_value();
  u8g.setFont(u8g_font_7x14);
  for (int i = 0; i < sensors; ++i) {
    sprintf(sensorName, "A%d", i);
    u8g.setPrintPos((i * 32) + 9, 60);
    u8g.print(sensorName);
    sprintf(percentage, "%3d%c", moisture_value[i], '%');
    u8g.setPrintPos((i * 32) + (moisture_value[i] == 100 ? 2 : 0), 45);
    u8g.print(percentage);
  }
}

bool inTimeFrame() {
  DateTime now = RTC.now();
  int t_now = now.hour() * 60 + now.minute();
  if (t_start == t_end) {
    return true;
  } else if (t_start < t_end) {
    if (t_start <= t_now && t_now < t_end)
      return true;
  } else {
    if (t_start <= t_now || t_now < t_end)
      return true;
  }
  return false;
}

void printTime() {
  DateTime now = RTC.now();
  char nowText[32];
  sprintf(nowText, "RTC reports %d-%d-%d %02d:%02d:%02d", now.year(), now.month(), now.day(), now.hour(), now.minute(), now.second());
  Serial.println(nowText);
}

void setOperateTime() {
  sprintf(operateTime, "%02d:%02d - %02d:%02d", startHour, startMinute, endHour, endMinute);
}

void serialToVars() {
  int i = 0;
  serialVars[i] = strtok(serialRead, delimeters);
  while (serialVars[i] != NULL)
    serialVars[++i] = strtok(NULL, delimeters);
}

void commandRTC() {
  serialToVars();
  if (serialVars[0]) {
    if (serialVars[1] && serialVars[2] && serialVars[3] && serialVars[4] && serialVars[5]) {
      Serial.println("Adjusting RTC");
      RTC.adjust(DateTime(atoi(serialVars[0]), atoi(serialVars[1]), atoi(serialVars[2]), atoi(serialVars[3]), atoi(serialVars[4]), atoi(serialVars[5])));
      printTime();
    }
  } else
    printTime();
}

/*
 * Definition of communication between the Elecrow Watering Kit
 * an Arduino Leonardo and boards using an ESP8266 or ESP32
 * 
 * All elements in the communication have a 4 byte length
 * The first element defines the requested action
 *   0001 - get minimum and maxmum moisture levels
 *          response is 0001 appended with
 *            first the four minumum levels
 *            then the four maximum levels
 *          both in the sequence of A0 through A3
 *   0002 - set the minimum and maximum moisture levels
 *          sequence is identical to command 1
 *            first the four minumum levels
 *            then the four maximum levels
 *          both in the sequence of A0 through A3
 *   0003 - get the timeframe in which the device operates 
 *          response is 0003 appended with
 *            start hour
 *            start minute
 *            end hour
 *            end minute
 *   0004 - set the timeframce in which the device operates
 *          sequence is identical to command 0003
 */
void commandESP() {
  char serial1Reply[36];
  switch (serial1Vars[0]) {
    case 1:
      sprintf(serial1Reply, "0001%04d%04d%04d%04d%04d%04d%04d%04d", min_moisture[0], min_moisture[1], min_moisture[2], min_moisture[3], max_moisture[0], max_moisture[1], max_moisture[2], max_moisture[3]);
      Serial1.println(serial1Reply);
      break;
    case 2:
      for (int i = 0; i < sensors; ++i) {
        min_moisture[ i ] = serial1Vars[ i + 1 ];
        max_moisture[ i ] = serial1Vars[ i + 5 ];
      }
      break;
    case 3:
      sprintf(serial1Reply, "0003%04d%04d%04d%04d", startHour, startMinute, endHour, endMinute);
      Serial1.println(serial1Reply);
      break;
    case 4:
      startHour = serial1Vars[ 1 ];
      startMinute = serial1Vars[ 2 ];
      endHour = serial1Vars[ 3 ];
      endMinute = serial1Vars[ 4 ];
      setOperateTime();
      break;
  }
}

void readSerial() {
  int c = Serial.read();
  switch (c) {
    case '\r': break; // ignore
    case '\n': 
      srPos = 0;
      commandRTC();
      break;
    default:
      if (srPos < 29) {
        serialRead[srPos++] = c;
        serialRead[srPos] = 0;
      }
  }
}

void readSerial1() {
  int c = Serial1.read();
  switch (c) {
    case '\r': break; // ignore
    case '\n': 
      sr1Pos = 0;
      commandESP();
      break;
    default:
      if (sr1Pos % 4 == 0 && sr1Pos / 4 > 0) {
        serial1Vars[ sr1Pos / 4 - 1 ] = atoi(serial1Read);
      }
      if (sr1Pos < 49) {
        serial1Read[ sr1Pos % 4 ] = c;
        sr1Pos += 1;
      }
  }
}
