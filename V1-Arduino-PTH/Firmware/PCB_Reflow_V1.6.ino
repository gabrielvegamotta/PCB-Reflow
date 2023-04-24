/* PCB Reflow 
 * Hardware - V1.0
 * Firmware - V1.6
 * 
 * By Gabriel Vega
 *  
 * 11/03/2023 
 * 
 * Inspired by:
 * Solder Reflow Plate Sketch  by Chris Halsall    
 */


#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <EEPROM.h>
#include <thermistor.h>

//Version Definitions
static const PROGMEM float hw = 1.0;
static const PROGMEM float sw = 1.6;

//Heating Animation
static const uint8_t PROGMEM heat_animate[] = {
  0b00000001, 0b00000000,
  0b00000001, 0b10000000,
  0b00000001, 0b10000000,
  0b00000001, 0b01000000,
  0b00000010, 0b01000000,
  0b00100010, 0b01000100,
  0b00100100, 0b00100100,
  0b01010101, 0b00100110,
  0b01001001, 0b10010110,
  0b10000010, 0b10001001,
  0b10100100, 0b01000001,
  0b10011000, 0b01010010,
  0b01000100, 0b01100010,
  0b00100011, 0b10000100,
  0b00011000, 0b00011000,
  0b00000111, 0b11100000
};
static const uint8_t heat_animate_width = 16;
static const uint8_t heat_animate_height = 16;

//Screen Definitions
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 32
#define SCREEN_ADDRESS 0x3C   //I2C Address
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1); //Create Display

//Pin Definitions
#define mosfet PD3
#define upsw PD5
#define dnsw PD6
#define temp PC2            //A2
#define temp_termistor PC1  //A1
#define vcc PC0             //A0

thermistor therm1(A1,0);

#define ledRed  9
#define ledGreen 10  
#define ledBlue 11

//Variables Definitions
float temperatura_thermistor = 0;
float maxTemp = 200;
float targetTemp = 180;
float safeTemp = 50;
int maxTime = 480;          //8 minutos
int x = 0;  //Display change counter
int y = 200; //Display change max (modulused below)

void setup() {
  
  //Pin Direction control
  pinMode(mosfet,OUTPUT);
  digitalWrite(mosfet,LOW);
  pinMode(upsw,INPUT);
  pinMode(dnsw,INPUT);
  pinMode(temp,INPUT);
  pinMode(temp_termistor,INPUT);
  pinMode(vcc,INPUT);

  pinMode(ledRed, OUTPUT);
  pinMode(ledGreen, OUTPUT);
  pinMode(ledBlue, OUTPUT);

  //Start-up Diplay
  display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS);
  display.clearDisplay();
  display.setTextSize(2);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0,0);
  display.print(F("PCB Reflow"));
  display.setTextSize(1);
  display.setCursor(0,16);
  display.print(F("By:"));
  display.setCursor(0,24);
  display.print(F("Gabriel Vega"));
  display.setCursor(80,16);
  display.print(F("S/W V"));
  display.print(sw, 1);
  display.setCursor(80,24);
  display.print(F("H/W V"));
  display.print(hw, 1);
  display.display();
  delay(3000);
  display.clearDisplay();

}

void loop() {

  temperatura_thermistor = getTemp();

  display.clearDisplay();
  display.setTextSize(1);
  display.setCursor(28,0); 
  display.print(F("Temp: "));
  display.print(temperatura_thermistor);
  display.print(F(" C"));

  display.setCursor(23,10);
  display.print(F("Target: "));
  display.print(targetTemp,1);
  display.print(F(" C"));

  display.setCursor(45,20); 
  display.print(F("Heat OFF"));
  
  if(digitalRead(upsw) == HIGH){
    heatingON();

  }else if(digitalRead(dnsw) == HIGH) {
    digitalWrite(mosfet, LOW);

    display.setTextSize(1);
    display.setCursor(45,20); 
    display.print(F("Heat OFF"));
    display.display();
  }
  display.display();

  if(temperatura_thermistor >= safeTemp){
    ledYELLOW_ON();
  }else {
    ledGREEN_ON();
  }
}

void heatingON(){
  
  unsigned long startTime = 0;
  unsigned long elapsedTime = 0; 

  temperatura_thermistor = getTemp();
  
  startTime = (millis() / 1000);  
  while(temperatura_thermistor <= maxTemp && digitalRead(dnsw) == LOW && elapsedTime <= maxTime){

    temperatura_thermistor = getTemp();

    if(temperatura_thermistor >= targetTemp && elapsedTime <= maxTime){

      display.clearDisplay();
      display.setCursor(30,0); 
      display.print(F("Temp"));
      display.print(temperatura_thermistor);
      display.print(F(" C"));
      
      display.setCursor(42,10);
      display.print(F("Time:")); 
      display.print(elapsedTime);
      display.print(F(" s"));

      display.setCursor(45,20); 
      display.print(F("Heat OFF"));

      display.display();

      digitalWrite(mosfet, LOW);
    }else if(temperatura_thermistor <= targetTemp && elapsedTime <= maxTime){

      display.clearDisplay();
      display.setCursor(30,0); 
      display.print(F("Temp:"));
      display.print(temperatura_thermistor);
      display.print(F(" C"));

      display.setCursor(42,10);
      display.print(F("Time:"));
      display.print(elapsedTime);
      display.print(F(" s"));

      display.setCursor(45,20); 
      display.print(F("Heat ON"));
      
      //Heat Animate Control
      display.drawBitmap( 0, 10, heat_animate, heat_animate_width, heat_animate_height, SSD1306_WHITE);
      display.drawBitmap( 112, 10, heat_animate, heat_animate_width, heat_animate_height, SSD1306_WHITE);
      display.fillRect( 0, 10, heat_animate_width, heat_animate_height * (y - x) / y, SSD1306_BLACK);
      display.fillRect( 112, 10, heat_animate_width, heat_animate_height * (y - x) / y, SSD1306_BLACK);
      x = ( x + 25 ) % y;

      display.display();

      if(temperatura_thermistor < 100){
        digitalWrite(mosfet, HIGH);     
        delay(200);
        digitalWrite(mosfet, LOW);     
        delay(100);

      }else if(temperatura_thermistor > 100){
        digitalWrite(mosfet, HIGH);     
        delay(350);
        digitalWrite(mosfet, LOW);     
        delay(20);
      }
    }

    ledRED_ON();
    elapsedTime = (millis() / 1000) - startTime;
  }

}

float getTemp(){
  float t = 0;
  for (byte i = 0; i < 10; i++){ //Poll temp reading 10 times
    t = t + therm1.analog2temp();
  }
  return ((t / 10) ); //Average, convert to C, and return
}


void ledRED_ON(){
  int r = 255,g = 0,b = 0;
  digitalWrite(ledRed, HIGH);
  digitalWrite(ledGreen, LOW);
  digitalWrite(ledBlue, LOW);
  
}

void ledGREEN_ON(){
  int r = 0,g = 255,b = 0;
  digitalWrite(ledRed, LOW);
  digitalWrite(ledGreen, HIGH);
  digitalWrite(ledBlue, LOW);
}

void ledBLUE_ON(){
  int r = 0,g = 0,b = 255;
  digitalWrite(ledRed, LOW);
  digitalWrite(ledGreen, LOW);
  digitalWrite(ledBlue, LOW);
}

void ledYELLOW_ON(){
  int r = 255,g = 255,b = 0;
  digitalWrite(ledRed, HIGH);
  digitalWrite(ledGreen, HIGH);
  digitalWrite(ledBlue, LOW);
}

void led_OFF(){
  int r = 0,g = 0,b = 0;
  digitalWrite(ledRed, LOW);
  digitalWrite(ledGreen, LOW);
  digitalWrite(ledBlue, LOW);
}


