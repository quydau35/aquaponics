// include the library code:
#include <LiquidCrystal.h>
#include <DHT.h>
#include <DFR_Key.h>

#define DHTPIN A1
#define DHTTYPE DHT11

// initialize the library with the numbers of the interface pins
LiquidCrystal lcd(8, 9, 4, 5, 6, 7);
DFR_Key keypad;

// define some values used by the panel and buttons
int lcd_key     = 0;
int adc_key_in  = 0;
int init_time = 0;
int lcd_windows = 0;
#define btnRIGHT  0
#define btnUP     1
#define btnDOWN   2
#define btnLEFT   3
#define btnSELECT 4
#define btnNONE   5

// read the buttons
int read_LCD_buttons() {
    adc_key_in = analogRead(0);      // read the value from the sensor 
    // my buttons when read are centered at these valies: 0, 144, 329, 504, 741
    // we add approx 50 to those values and check to see if we are close
    if (adc_key_in > 1000) return btnNONE; // We make this the 1st option for speed reasons since it will be the most likely result
    // For V1.1 us this threshold
    if (adc_key_in < 50)   return btnRIGHT;  
    if (adc_key_in < 250)  return btnUP; 
    if (adc_key_in < 450)  return btnDOWN; 
    if (adc_key_in < 650)  return btnLEFT; 
    if (adc_key_in < 850)  return btnSELECT;  
    Serial.print("analogRead(0): ");
    Serial.println(adc_key_in);
    return btnNONE;  // when all others fail, return this...
}
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


void show_DHT(){
    float h = dht.readHumidity();
    float t = dht.readTemperature();
    /*int humid = */

    if (isnan(t) || isnan(h)) {
        Serial.println("Failed to read from sensor. Reloading...");
        lcd.print("Reloading..");
        /*delay(500);*/
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
        /*delay(500);*/
    }
}
void setup() {
    // set up the LCD's number of columns and rows:
    lcd.begin(16, 2);
    lcd.clear();
    lcd.setCursor(0,0);
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

    keypad.setRate(10);
}
void loop() {
    lcd.setCursor(0,0);
    lcd_key = read_LCD_buttons();
    switch (lcd_key)               // depending on which button was pushed, we perform an action
    {
       case btnRIGHT:
         {
         lcd.print("RIGHT ");
         delay(500);
         break;
         }
       case btnLEFT:
         {
         lcd.print("LEFT   ");
         delay(500);
         break;
         }
       case btnUP: {
        lcd.print("UP    ");
        delay(500);
        lcd.clear();
        lcd_windows --;
        if (lcd_windows < 0) { 
            lcd_windows = 0; 
        } else {
             return;
        }
        break;
        }
       case btnDOWN:
        {
        lcd.print("DOWN  ");
        delay(500);
        lcd.clear();
        lcd_windows ++;
        if (lcd_windows < 0) { 
            lcd_windows = 0; 
        } else {
            return;
        }
        break;
        }
       case btnSELECT:
         {
         lcd.print("SELECT");
         delay(500);
         break;
         }
         case btnNONE:
         {
         lcd.print("NONE  ");
         delay(500);
         break;
         }
    }
}
