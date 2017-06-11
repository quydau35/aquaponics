// include the library code:
#include <LiquidCrystal.h>
#include <DHT.h>

#define DHTPIN A1
#define DHTTYPE DHT11

// initialize the library with the numbers of the interface pins
LiquidCrystal lcd(8, 9, 4, 5, 6, 7);

// Custom character
byte temp[8] = {
  0b00100,
  0b01010,
  0b00100,
  0b00000,
  0b00000,
  0b00000,
  0b00000,
  0b00000
};

DHT dht(DHTPIN, DHTTYPE);
// Connect pin 1 (on the left) of the sensor to +5V
// Connect pin 2 of the sensor to whatever your DHTPIN is
// Connect pin 4 (on the right) of the sensor to GROUND
// Connect a 10K resistor from pin 2 (data) to pin 1 (power) of the sensor

void setup() {
    // set up the LCD's number of columns and rows:
    lcd.begin(16, 2);
    // Print a message to the LCD.
    /*lcd.print("hello, world!");*/
    lcd.print("Humidity:      %");
    lcd.setCursor(0,1);
    lcd.print("Temperature:  *C");

    // DHT on serial
    Serial.begin(9600);
    Serial.println("DHT11 test!");

    // create temperature character
    lcd.createChar(0, temp);

    dht.begin();
    /*lcd.display();*/
}

void loop() {
    float h = dht.readHumidity();
    float t = dht.readTemperature();
    /*int humid = */

    if (isnan(t) || isnan(h)) {
	Serial.println("Failed to read from sensor. Reloading...");
	lcd.print("Reloading..");
	delay(500);
    } else {
	// Print Humidity & Temperature value to LCD
	lcd.setCursor(13,0);
	lcd.print(h);
	lcd.setCursor(15,0);
	lcd.print("%");
	lcd.setCursor(12,1);
	lcd.print(t);
	lcd.setCursor(14,1);
	lcd.write(byte(0));
	lcd.setCursor(15,1);
	lcd.write("C");

	// Print Humidity & Temperature value to Serial
	Serial.print("Humidity: "); 
	Serial.print(h);
	Serial.print(" %\t");
	Serial.print("Temperature: "); 
	Serial.print(t);
	Serial.println(" *C");
	delay(500);
    }
}
