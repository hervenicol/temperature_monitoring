#include <OneWire.h>
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define OLED_RESET_PIN 13
#define ONEWIRE_PIN 10

Adafruit_SSD1306 display(OLED_RESET_PIN);

OneWire  ds(ONEWIRE_PIN);  // on pin 10 (a 4.7K resistor is necessary)

void setup(void) {
  Serial.begin(9600);
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);  // initialize with the I2C addr 0x3C (for the 128x32)
}

int addr_cmp(byte* a1, byte* a2, int len) {
  int i;
  for( i = 0; i < len; i++) {
    if ( a1[i] != a2[i] ) {
      return 1;
    } 
  }
  return 0;
}


void loop(void) {
  byte i;
  byte present = 0;
  byte type_s;
  byte data[12];
  byte addr[8];
  float celsius, fahrenheit;
  char* probe_name;
  
  byte addr_salon[] = {0x28, 0xFF, 0x18, 0x41, 0x65, 0x15, 0x03, 0x04};
  byte addr_arduino[] = {0x28, 0xFF, 0x4B, 0xBD, 0x64, 0x15, 0x01, 0xAB};
  byte addr_lit[] = {0x28, 0xFF, 0x5F, 0x7F, 0x65, 0x15, 0x02, 0xBF};
  byte addr_cave[] = {0x28, 0xFF, 0xED, 0xBB, 0x64, 0x15, 0x01, 0x0D};
  
  if ( !ds.search(addr)) {
    Serial.println("No more addresses.");
    Serial.println();
    ds.reset_search();
    delay(250);
    return;
  }
  
  Serial.print("ROM =");
  for( i = 0; i < 8; i++) {
    Serial.write(' ');
    Serial.print(addr[i], HEX);
  }
/*  Serial.print(" - ardu =");
  for( i = 0; i < 8; i++) {
    Serial.write(' ');
    Serial.print(addr_arduino[i], HEX);
  }*/
  Serial.print(" = ");
  if (addr_cmp(addr, addr_salon, 8) == 0) {
      probe_name = "salon";
      Serial.print(probe_name);
  } else if (addr_cmp(addr, addr_arduino, 8) == 0) {
      probe_name = "arduino";
      Serial.print(probe_name);
  } else if (addr_cmp(addr, addr_lit, 8) == 0) {
      probe_name = "lit";
      Serial.print(probe_name);
  } else if (addr_cmp(addr, addr_cave, 8) == 0) {
      probe_name = "cave";
      Serial.print(probe_name);
  } else {
      probe_name = "unknown";
      Serial.print(probe_name);
  } 





  if (OneWire::crc8(addr, 7) != addr[7]) {
      Serial.println("CRC is not valid!");
      return;
  }
  Serial.println();
 
  // the first ROM byte indicates which chip
  switch (addr[0]) {
    case 0x10:
//    Chip = DS18S20 or old DS1820
      type_s = 1;
      break;
    case 0x28:
//    Chip = DS18B20
      type_s = 0;
      break;
    case 0x22:
//    Chip = DS1822
      type_s = 0;
      break;
    default:
      Serial.println("Device is not a DS18x20 family device.");
      return;
  } 

  ds.reset();
  ds.select(addr);
  ds.write(0x44, 1);        // start conversion, with parasite power on at the end
  
  delay(1000);     // maybe 750ms is enough, maybe not
  // we might do a ds.depower() here, but the reset will take care of it.
  
  present = ds.reset();
  ds.select(addr);    
  ds.write(0xBE);         // Read Scratchpad

  for ( i = 0; i < 9; i++) {           // we need 9 bytes
    data[i] = ds.read();
  }

  // Convert the data to actual temperature
  // because the result is a 16 bit signed integer, it should
  // be stored to an "int16_t" type, which is always 16 bits
  // even when compiled on a 32 bit processor.
  int16_t raw = (data[1] << 8) | data[0];
  if (type_s) {
    raw = raw << 3; // 9 bit resolution default
    if (data[7] == 0x10) {
      // "count remain" gives full 12 bit resolution
      raw = (raw & 0xFFF0) + 12 - data[6];
    }
  } else {
    byte cfg = (data[4] & 0x60);
    // at lower res, the low bits are undefined, so let's zero them
    if (cfg == 0x00) raw = raw & ~7;  // 9 bit resolution, 93.75 ms
    else if (cfg == 0x20) raw = raw & ~3; // 10 bit res, 187.5 ms
    else if (cfg == 0x40) raw = raw & ~1; // 11 bit res, 375 ms
    //// default is 12 bit resolution, 750 ms conversion time
  }
  celsius = (float)raw / 16.0;
  Serial.print("  Temperature = ");
  Serial.print(celsius);
  Serial.println(" Celsius");

// OLED display  
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(0,0);
  display.clearDisplay();
  display.println(probe_name);
  display.println(celsius);
  display.display();

  delay(2000);

}
