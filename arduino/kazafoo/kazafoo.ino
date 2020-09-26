#include <Adafruit_NeoPixel.h>
#ifdef __AVR__
 #include <avr/power.h> // Required for 16 MHz Adafruit Trinket
#endif

#define LED_PIN_LEFT     13
#define LED_PIN_RIGHT    12
const int analogInPinLeft = A0;  // Analog input pin that the potentiometer is attached to
const int analogInPinRight = A1;  // Analog input pin that the potentiometer is attached to

#define LED_COUNT 30

// Declare our NeoPixel stripRight object:
Adafruit_NeoPixel stripLeft( LED_COUNT, LED_PIN_LEFT,  NEO_GRB + NEO_KHZ800);
Adafruit_NeoPixel stripRight(LED_COUNT, LED_PIN_RIGHT, NEO_GRB + NEO_KHZ800);

// setup() function -- runs once at startup --------------------------------

void setup() {
  // These lines are specifically to support the Adafruit Trinket 5V 16 MHz.
  // Any other board, you can remove this part (but no harm leaving it):
#if defined(__AVR_ATtiny85__) && (F_CPU == 16000000)
  clock_prescale_set(clock_div_1);
#endif
  // END of Trinket-specific code.

  unsigned long timeoutMs = (unsigned long)1000 * 60 * 60 * 24;  // 1日（ほぼ無限にタイムアウトしないように）

  stripLeft.begin();           // INITIALIZE NeoPixel stripRight object (REQUIRED)
  stripLeft.show();            // Turn OFF all pixels ASAP
  stripLeft.setBrightness(50); // Set BRIGHTNESS to about 1/5 (max = 255)
  
  stripRight.begin();           // INITIALIZE NeoPixel stripRight object (REQUIRED)
  stripRight.show();            // Turn OFF all pixels ASAP
  stripRight.setBrightness(50); // Set BRIGHTNESS to about 1/5 (max = 255)
  
  Serial.begin(9600);
  Serial.setTimeout(timeoutMs);
}


// loop() function -- runs repeatedly as long as board is on ---------------

const int FILTER_BUFFER_SIZE = 3;
float filterBufferLeft[FILTER_BUFFER_SIZE] = {};
float filterBufferRight[FILTER_BUFFER_SIZE] = {};

float filter(float rawValue, float* filterBuffer, int bufferSize)
{
  // shift values
  for(int i=0; i<(bufferSize - 1); i++) {
    filterBuffer[i] = filterBuffer[i+1];
  }
  // append value
  filterBuffer[bufferSize-1] = rawValue;

  // average
  float sum = 0;
  for(int i=0; i<bufferSize; i++) {
    sum += filterBuffer[i];
  }

  return sum / bufferSize;
}

float getSensorValue(int sensorPin) {
  int sensorValue = analogRead(sensorPin);
  float dist  = adVoltageToDistMeter(sensorValue);
  float joy  = min(mapFloat(dist,  0.1, 1.0, 1.0, 0), 1.0);
  return joy;
}

void setLED(Adafruit_NeoPixel* strip, uint8_t red, uint8_t green, uint8_t blue, uint8_t num) {
  if (num > LED_COUNT)  num   = LED_COUNT;
  uint32_t color  =  strip->Color(red, green, blue);
    
  strip->clear();
  if(num > 0) {
    strip->fill(color, 0, num);
  }
  strip->show();
}

void processLED(String command, boolean isLeft) {
  if (command.length() != 10) {
    return;
  }
  uint8_t red   = map(command.substring(2, 4).toInt(), 0, 99, 0, 255);
  Serial.println(red);
  uint8_t green = map(command.substring(4, 6).toInt(), 0, 99, 0, 255);
  Serial.println(green);
  uint8_t blue  = map(command.substring(6, 8).toInt(), 0, 99, 0, 255);
  Serial.println(blue);
  uint8_t num   = command.substring(8, 10).toInt();
  Serial.println(num);
  if (num > 30) num = 30;

  if (isLeft) {
    setLED(&stripLeft, red, green, blue, num);
  } else {
    setLED(&stripRight, red, green, blue, num);
  }
}

void loop() {
  String command = Serial.readStringUntil('\n');

  if (command.startsWith("GS")) { // Get Sensor
    float left = getSensorValue(analogInPinLeft);
    float right = getSensorValue(analogInPinRight);
    Serial.println("get sensor");
    Serial.print(left);
    Serial.print(",");
    Serial.println(right);
  } else if (command.startsWith("LL")) { // LED Left
    Serial.println("LED Left");
    processLED(command, true);
  } else if (command.startsWith("LR")) { // LED Right
    Serial.println("LED Right");
    processLED(command, false);
  } else {
    Serial.println("Invalid Command");
  }
}

float mapFloat(float x, float in_min, float in_max, float out_min, float out_max) {
  return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

float adVoltageToDistMeter(uint32_t ad_10bit){
  // GP2Y0A21YK0F の値をMeter単位で返す
  // 
  // Args:
  //    ad_10bit: 10bit AD変換の結果 (5V -> 1023)
  // Return: Meter単位の距離 1.0m以上は1.0mに丸められている
  
  float dist1 = 5.0 * ad_10bit / 1023;
  float dist2 = 26.549*pow(dist1,-1.2091)/100;
  float distMeter = min(dist2, 1.0); // 1.0m 以上の値は丸める
  return distMeter;
}
