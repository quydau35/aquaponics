// include the library code:
#include <LiquidCrystal.h>
#include <DHT.h>
#include <DFR_Key.h>

#define DHTPIN A1
#define DHTTYPE DHT11

// initialize the library with the numbers of the interface pins
LiquidCrystal lcd(8, 9, 4, 5, 6, 7);
DFR_Key keypad;
int windowCursor[] = {0,0,0};

// Pins in use
#define BUTTON_ADC_PIN           A0  // A0 is the button ADC input
#define LCD_BACKLIGHT_PIN         10  // D10 controls LCD backlight
// ADC readings expected for the 5 buttons on the ADC input
#define RIGHT_10BIT_ADC           0  // right
#define UP_10BIT_ADC             92  //145  // up
#define DOWN_10BIT_ADC          252  //329  // down
#define LEFT_10BIT_ADC          404  //505  // left
#define SELECT_10BIT_ADC        629  //741  // right
#define BUTTONHYSTERESIS         30  //10  // hysteresis for valid button sensing window
//return values for ReadButtons()
#define BUTTON_NONE               0  // 
#define BUTTON_RIGHT              1  // 
#define BUTTON_UP                 2  // 
#define BUTTON_DOWN               3  // 
#define BUTTON_LEFT               4  // 
#define BUTTON_SELECT             5  // 
//some example macros with friendly labels for LCD backlight/pin control, tested and can be swapped into the example code as you like
#define LCD_BACKLIGHT_OFF()     digitalWrite( LCD_BACKLIGHT_PIN, LOW )
#define LCD_BACKLIGHT_ON()      digitalWrite( LCD_BACKLIGHT_PIN, HIGH )
#define LCD_BACKLIGHT(state)    { if( state ){digitalWrite( LCD_BACKLIGHT_PIN, HIGH );}else{digitalWrite( LCD_BACKLIGHT_PIN, LOW );} }
/*--------------------------------------------------------------------------------------
  Variables
--------------------------------------------------------------------------------------*/
byte buttonJustPressed  = false;         //this will be true after a ReadButtons() call if triggered
byte buttonJustReleased = false;         //this will be true after a ReadButtons() call if triggered
byte buttonWas          = BUTTON_NONE;   //used by ReadButtons() for detection of button events

unsigned long debounceDelay = 550;    // the debounce time; increase if the output flickers

// time variables
#define humidSamplingInterval 250
#define pHSamplingInterval 20
#define printInterval 500
#define ArrayLength 40 //times of collection
int hourStopInterval = 3;
int hourPumpInterval = 0;
int minStopInterval = 0;
int minPumpInterval = 15;
unsigned long buttonPressedTime = 0; // Time when press a key.
unsigned long idleTime = 10; // Time to idle in second.

//pH array
#define SensorPin A2 //pH meter Analog output to arduino analog input
#define Offset 0.00
int pHArray[ArrayLength];   //Store the average value of the sensor feedback
int pHArrayIndex=0;    

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
byte ee[8] = {
    0b00100,
    0b01010,
    0b00100,
    0b01010,
    0b11111,
    0b01000,
    0b00110,
    0b00100
};
byte oo[8] = {
    0b00100,
    0b01010,
    0b00000,
    0b01110,
    0b10001,
    0b10001,
    0b01110,
    0b00100
};
byte aa[8] = {
    0b01011,
    0b10101,
    0b00100,
    0b00010,
    0b01110,
    0b10010,
    0b11111,
    0b00000
};
byte ow[8] = {
    0b00001,
    0b00001,
    0b01110,
    0b10010,
    0b10010,
    0b10010,
    0b01100,
    0b00000
};
byte uw[8] = {
    0b00000,
    0b00001,
    0b10011,
    0b10010,
    0b10010,
    0b10010,
    0b01111,
    0b00000
};
byte dd[8] = {
    0b00010,
    0b01110,
    0b00010,
    0b01110,
    0b10010,
    0b10010,
    0b01111,
    0b00000
};
byte DD[8] = {
    0b11100,
    0b10010,
    0b10001,
    0b11001,
    0b10001,
    0b10010,
    0b11100,
    0b00000
};

DHT dht(DHTPIN, DHTTYPE);
// Connect pin 1 (on the left) of the sensor to +5V
// Connect pin 2 of the sensor to whatever your DHTPIN is
// Connect pin 4 (on the right) of the sensor to GROUND
// Connect a 10K resistor from pin 2 (data) to pin 1 (power) of the sensor

void setup() {
    // set pin 3 as controlling pin for relay
    pinMode(3, OUTPUT);
    pinMode( LCD_BACKLIGHT_PIN, OUTPUT );     //D3 is an output
    // set up the LCD's number of columns and rows:
    lcd.begin(16, 2);
    // Print a message to the LCD.
    /*lcd.print("hello, world!");*/
    /*lcd.print("Humidity:      %");*/
    /*lcd.setCursor(0,1);*/
    /*lcd.print("Temperature:  *C");*/
    lcd.print("Initializing...");

    // DHT on serial
    Serial.begin(9600);
    Serial.println("DHT11 test!");

    // create temperature character
    lcd.createChar(0, temp);
    lcd.createChar(1, ee);
    lcd.createChar(2, oo);
    lcd.createChar(3, aa);
    lcd.createChar(4, ow);
    lcd.createChar(5, uw);
    lcd.createChar(6, dd);
    lcd.createChar(7, DD);

    dht.begin();
    /*lcd.display();*/
}


bool checkIdle() {
    if (millis() - buttonPressedTime < 0) {
	buttonPressedTime = millis();
	checkIdle();
	Serial.print("Aha! millis()-buttonPressedTime<0");
    } else if (millis() - buttonPressedTime > (idleTime*1000)) {
	Serial.println("checkIdle true, set LCD_BACKLIGHT_PIN to LOW");
	digitalWrite( LCD_BACKLIGHT_PIN, LOW );
	delay(150);
	return true;
    } else {
	Serial.println("checkIdle false, set LCD_BACKLIGHT_PIN to HIGH");
	digitalWrite( LCD_BACKLIGHT_PIN, HIGH );
	return false;
    }
}

void showWindow0() {
    static unsigned long samplingTime = millis();
    static unsigned long printTime = millis();
    static float h,t;
    if (millis()-samplingTime > humidSamplingInterval) {
	h = dht.readHumidity();
	t = dht.readTemperature();
	samplingTime = millis();
    }

    if (millis() - printTime > printInterval) {
	if (isnan(t) || isnan(h)) {
	    Serial.println("Failed to read from sensor. Reloading...");
	    lcd.clear();
	    lcd.print("Reloading..");

	} else if ( t==0 && h==0 ){
	    lcd.clear();
	    lcd.println("Please plug in");
	    lcd.println("DHT11 sensor...");
	} else {
	    // Print Humidity & Temperature value to LCD
	    lcd.clear();
	    lcd.setCursor(0,0);
	    lcd.cursor();
	    lcd.print("    m:        %");
	    lcd.setCursor(0,1);
	    lcd.print("Nhi t   :    *C");
	    lcd.setCursor(0,0); // Write DD character
	    lcd.write(byte(7));
	    lcd.setCursor(1,0); // Write oo character
	    lcd.write(byte(2));
	    lcd.setCursor(3,0); // Write aa character
	    lcd.write(byte(3));
	    lcd.setCursor(3,1); // Write ee character
	    lcd.write(byte(1));
	    lcd.setCursor(6,1); // Write dd character
	    lcd.write(byte(6));
	    lcd.setCursor(7,1); // Write oo character
	    lcd.write(byte(2));
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
	    lcd.setCursor(windowCursor[2],windowCursor[1]);

	    // Print Humidity & Temperature value to Serial
	    Serial.print("Humidity: "); 
	    Serial.print(h);
	    Serial.print(" %\t");
	    Serial.print("Temperature: "); 
	    Serial.print(t);
	    Serial.println(" *C");
	}
	printTime = millis();
    }
}

void showWindow1() {
    static unsigned long samplingTime = millis();
    static unsigned long printTime = millis();
    static float pHValue,voltage;
    if(millis()-samplingTime > pHSamplingInterval)
    {
      pHArray[pHArrayIndex++]=analogRead(SensorPin);
      if(pHArrayIndex==ArrayLength)pHArrayIndex=0;
      voltage = avergearray(pHArray, ArrayLength)*5.0/1024;
      pHValue = 3.5*voltage+Offset;
      samplingTime=millis();
    }
    if(millis() - printTime > printInterval)   //Every 800 milliseconds, print a numerical, convert the state of the LED indicator
    {
	if (isnan(pHValue)) {
	    Serial.println("Failed to read from sensor. Reloading...");
	    lcd.clear();
	    lcd.print("Reloading..");

	} else if ( pHValue==0 ){
	    lcd.clear();
	    lcd.setCursor(0,0);
	    lcd.cursor();
	    lcd.print("Please plug in");
	    lcd.setCursor(1,0);
	    lcd.print("pH sensor...");
	    lcd.setCursor(windowCursor[2],windowCursor[1]);
	} else {
	    // Print average pH value to LCD
	    lcd.clear();
	    lcd.cursor();
	    lcd.print("   pH:");
	    lcd.setCursor(0,0); // Write DD character
	    lcd.write(byte(7));
	    lcd.setCursor(1,0); // Write oo character
	    lcd.write(byte(2));
	    lcd.setCursor(12,1);
	    lcd.print(pHValue);
	    lcd.setCursor(windowCursor[2],windowCursor[1]);

	    // Print average pH value to Serial
	    Serial.print("pH of water: "); 
	    Serial.print(pHValue);
	}

	    Serial.print(" Voltage:");
	    Serial.print(voltage,2);
	    Serial.print("    pH value: ");
	    Serial.println(pHValue,2);
	    printTime=millis();
	}
    }
    double avergearray(int* arr, int number){
    int i;
    int max,min;
    double avg;
    long amount=0;
    if(number<=0){
    Serial.println("Error number for the array to avraging!/n");
    return 0;
    }
    if(number<5){   //less than 5, calculated directly statistics
    for(i=0;i<number;i++){
      amount+=arr[i];
    }
    avg = amount/number;
    return avg;
    }else{
    if(arr[0]<arr[1]){
      min = arr[0];max=arr[1];
    }
    else{
      min=arr[1];max=arr[0];
    }
    for(i=2;i<number;i++){
      if(arr[i]<min){
	amount+=min;        //arr<min
	min=arr[i];
      }else {
	if(arr[i]>max){
	  amount+=max;    //arr>max
	  max=arr[i];
	}else{
	  amount+=arr[i]; //min<=arr<=max
	}
      }//if
    }//for
    avg = (double)amount/(number-2);
    }//if
    return avg;
}

void showWindow2 () {
    static unsigned long printTime = millis();
    if ((millis() - printTime) > printInterval) {
	lcd.clear();
	// Print idle time interval
	lcd.setCursor(0,0);
	lcd.cursor();
	lcd.print("Ng ng");
	lcd.setCursor(2,0); // Write uw character
	lcd.write(byte(5));
	if (hourStopInterval < 10) {
	    lcd.setCursor(7,0);
	    lcd.print(hourStopInterval);
	} else {
	    lcd.setCursor(6,0);
	    lcd.print(hourStopInterval);
	}
	lcd.setCursor(8,0);
	lcd.print("hrs");
	if (minStopInterval < 10) {
	    lcd.setCursor(12,0);
	    lcd.print(minStopInterval);
	} else {
	    lcd.setCursor(11,0);
	    lcd.print(minStopInterval);
	}
	lcd.setCursor(13,0);
	lcd.print("min");

	// Print pumping time interval
	lcd.setCursor(0,1);
	lcd.print("B m");
	lcd.setCursor(1,1); // Write ow character
	lcd.write(byte(4));
	if (hourPumpInterval < 10) {
	    lcd.setCursor(7,1);
	    lcd.print(hourPumpInterval);
	} else {
	    lcd.setCursor(6,1);
	    lcd.print(hourPumpInterval);
	}
	lcd.setCursor(8,1);
	lcd.print("hrs");
	if (minPumpInterval < 10) {
	    lcd.setCursor(12,2);
	    lcd.print(minPumpInterval);
	} else {
	    lcd.setCursor(11,1);
	    lcd.print(minPumpInterval);
	}
	lcd.setCursor(13,3);
	lcd.print("min");
	lcd.setCursor(windowCursor[2], windowCursor[1]);
	printTime = millis();
    }
}

void checkCursor() {
    if (windowCursor[0] < 0) {
	windowCursor[0] = 0;
    } else if (windowCursor[0] > 2) {
	windowCursor[0] = 2;
    } else {
	windowCursor[0] = windowCursor[0];
    }

    if (windowCursor[1] < 0) {
	windowCursor[1] = 1;
	windowCursor[0]--;
	checkCursor();
    } else if (windowCursor[1] > 1) {
	windowCursor[1] = 0;
	windowCursor[0]++;
	checkCursor();
    } else {
	windowCursor[1] = windowCursor[1];
    }

    if (windowCursor[2] < 0) {
	windowCursor[2] = 0;
    } else if (windowCursor[2] > 15) {
	windowCursor[2] = 15;
    } else {
	windowCursor[2] = windowCursor[2];
    }
}

void changeValue() {
    // Check & change idle interval
    if ((windowCursor[0] == 2 && windowCursor[1] == 0 && windowCursor[2] == 11) || (windowCursor[0] == 2 && windowCursor[1] == 0 && windowCursor[2] == 12)) {
	minStopInterval++;
	if (minStopInterval > 60) {
	    minStopInterval = 0;
	}

	if (minStopInterval < 10) {
	    lcd.setCursor(12,0);
	    lcd.print(minStopInterval);
	} else {
	    lcd.setCursor(11,0);
	    lcd.print(minStopInterval);
	}
    } else if ((windowCursor[0] == 2 && windowCursor[1] == 0 && windowCursor[2] == 6) || (windowCursor[0] == 2 && windowCursor[1] == 0 && windowCursor[2] == 7)) {
	hourStopInterval++;
	if (hourStopInterval > 12) {
	    hourStopInterval = 0;
	}

	if (hourStopInterval < 10) {
	    lcd.setCursor(7,0);
	    lcd.print(hourStopInterval);
	} else {
	    lcd.setCursor(6,0);
	    lcd.print(hourStopInterval);
	}
    } else { 
      return;
    }
    
    // Check & change pumping interval
    if ((windowCursor[0] == 2 && windowCursor[1] == 1 && windowCursor[2] == 11) || (windowCursor[0] == 2 && windowCursor[1] == 1 && windowCursor[2] == 12)) {
	minPumpInterval++;
	if (minPumpInterval > 60) {
	    minPumpInterval = 0;
	}

	if (minPumpInterval < 10) {
	    lcd.setCursor(12,0);
	    lcd.print(minPumpInterval);
	} else {
	    lcd.setCursor(11,0);
	    lcd.print(minPumpInterval);
	}
    } else if ((windowCursor[0] == 2 && windowCursor[1] == 1 && windowCursor[2] == 6) || (windowCursor[0] == 2 && windowCursor[1] == 1 && windowCursor[2] ==7)) {
	hourPumpInterval++;
	if (hourPumpInterval > 12) {
	    hourPumpInterval = 0;
	}

	if (hourPumpInterval < 10) {
	    lcd.setCursor(7,0);
	    lcd.print(hourPumpInterval);
	} else {
	    lcd.setCursor(6,0);
	    lcd.print(hourPumpInterval);
	}
    } else { 
      return;
    }

}

void chooseWindow() {
    if (checkIdle()) {
	digitalWrite(LCD_BACKLIGHT_PIN, LOW);
	if (windowCursor[0] == 0) {
	    showWindow0();
	} else if (windowCursor[0] == 1) {
	    showWindow1();
	} else if (windowCursor[0] == 2) {
	    showWindow2();
	} else {
	    showWindow0();
	}
    } else {
	digitalWrite(LCD_BACKLIGHT_PIN, HIGH);
	if (windowCursor[0] == 0) {
	    showWindow0();
	} else if (windowCursor[0] == 1) {
	    showWindow1();
	} else if (windowCursor[0] == 2) {
	    showWindow2();
	} else {
	    showWindow0();
	}
    }
}

/*--------------------------------------------------------------------------------------
  ReadButtons()
  Detect the button pressed and return the value
  Uses global values buttonWas, buttonJustPressed, buttonJustReleased.
--------------------------------------------------------------------------------------*/
byte ReadButtons()
{
   unsigned int buttonVoltage;
   byte button = BUTTON_NONE;   // return no button pressed if the below checks don't write to btn
 
   //read the button ADC pin voltage
   buttonVoltage = analogRead( BUTTON_ADC_PIN );
   //sense if the voltage falls within valid voltage windows
   if( buttonVoltage < ( RIGHT_10BIT_ADC + BUTTONHYSTERESIS ) )
   {
      button = BUTTON_RIGHT;
   }
   else if(   buttonVoltage >= ( UP_10BIT_ADC - BUTTONHYSTERESIS )
           && buttonVoltage <= ( UP_10BIT_ADC + BUTTONHYSTERESIS ) )
   {
      button = BUTTON_UP;
   }
   else if(   buttonVoltage >= ( DOWN_10BIT_ADC - BUTTONHYSTERESIS )
           && buttonVoltage <= ( DOWN_10BIT_ADC + BUTTONHYSTERESIS ) )
   {
      button = BUTTON_DOWN;
   }
   else if(   buttonVoltage >= ( LEFT_10BIT_ADC - BUTTONHYSTERESIS )
           && buttonVoltage <= ( LEFT_10BIT_ADC + BUTTONHYSTERESIS ) )
   {
      button = BUTTON_LEFT;
   }
   else if(   buttonVoltage >= ( SELECT_10BIT_ADC - BUTTONHYSTERESIS )
           && buttonVoltage <= ( SELECT_10BIT_ADC + BUTTONHYSTERESIS ) )
   {
      button = BUTTON_SELECT;
   }
   //handle button flags for just pressed and just released events
   if( ( buttonWas == BUTTON_NONE ) && ( button != BUTTON_NONE ) )
   {
      //the button was just pressed, set buttonJustPressed, this can optionally be used to trigger a once-off action for a button press event
      //it's the duty of the receiver to clear these flags if it wants to detect a new button change event
      buttonJustPressed  = true;
      buttonJustReleased = false;
   }
   if( ( buttonWas != BUTTON_NONE ) && ( button == BUTTON_NONE ) )
   {
      buttonJustPressed  = false;
      buttonJustReleased = true;
   }
 
   if (((millis() - buttonPressedTime) < debounceDelay) && buttonWas == button) {
       return BUTTON_NONE;
   } else {
       // Debug bouncing key
       /*Serial.println(millis());*/
       /*Serial.println(buttonPressedTime);*/
     
       //save the latest button value, for change event detection next time round
       buttonWas = button;
       return( button );
   }
}

void showTempHumid() {
    byte button;
    //get the latest button pressed, also the buttonJustPressed, buttonJustReleased flags
    button = ReadButtons();
    switch( button )
    {
	case BUTTON_NONE:
	{
	    break;
	}
	case BUTTON_RIGHT:
	{
	    Serial.println("press RIGHT");
	    buttonPressedTime = millis();
	    windowCursor[2]++;
	    checkCursor();
	    break;
	}
	case BUTTON_UP:
	{
	    Serial.println("press UP");
	    buttonPressedTime = millis();
	    windowCursor[1]--;
	    checkCursor();
	    chooseWindow();
	    break;
	}
	case BUTTON_DOWN:
	{
	    Serial.println("press DOWN");
	    buttonPressedTime = millis();
	    windowCursor[1]++;
	    checkCursor();
	    chooseWindow();
	    break;
	}
	case BUTTON_LEFT:
	{
	    Serial.println("press LEFT");
	    buttonPressedTime = millis();
	    windowCursor[2]--;
	    checkCursor();
	    break;
	}
	case BUTTON_SELECT:
	{
	    Serial.println("press SELECT");
	    buttonPressedTime = millis();
	    checkIdle();
	    changeValue();
	    break;
	}
    default:
       	{
	    break;
	}
    }
   if( buttonJustPressed )
      buttonJustPressed = false;
   if( buttonJustReleased )
      buttonJustReleased = false;
   chooseWindow();
}

void loop() {
    unsigned long startPumpMillis = millis(); // 900000 = 15 mins
    unsigned long t1 = 60000*minPumpInterval + 3600000*hourPumpInterval;
    while ((millis() - startPumpMillis) < t1) {
	if (millis() - startPumpMillis < 0) {
	    startPumpMillis = millis();
	}
	digitalWrite(3, HIGH);
	showTempHumid();
    }
    digitalWrite(3, LOW);
    unsigned long startmillis = millis();
    unsigned long t2 = 60000*minStopInterval + 3600000*hourPumpInterval;
    while (millis() - startmillis < t2) { //10800000 = 3 hrs
	if (millis() - startPumpMillis < 0) {
	    startPumpMillis = millis();
	}
	showTempHumid();
    }
   
}
