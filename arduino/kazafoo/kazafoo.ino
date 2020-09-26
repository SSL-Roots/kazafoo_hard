// A basic everyday NeoPixel stripRight test program.

// NEOPIXEL BEST PRACTICES for most reliable operation:
// - Add 1000 uF CAPACITOR between NeoPixel stripRight's + and - connections.
// - MINIMIZE WIRING LENGTH between microcontroller board and first pixel.
// - NeoPixel stripRight's DATA-IN should pass through a 300-500 OHM RESISTOR.
// - AVOID connecting NeoPixels on a LIVE CIRCUIT. If you must, ALWAYS
//   connect GROUND (-) first, then +, then data.
// - When using a 3.3V microcontroller with a 5V-powered NeoPixel stripRight,
//   a LOGIC-LEVEL CONVERTER on the data line is STRONGLY RECOMMENDED.
// (Skipping these may work OK on your workbench but can fail in the field)

#include <Adafruit_NeoPixel.h>
#ifdef __AVR__
 #include <avr/power.h> // Required for 16 MHz Adafruit Trinket
#endif

// Which pin on the Arduino is connected to the NeoPixels?
// On a Trinket or Gemma we suggest changing this to 1:
#define LED_PIN_LEFT     13
#define LED_PIN_RIGHT    12
const int analogInPinLeft = A0;  // Analog input pin that the potentiometer is attached to
const int analogInPinRight = A1;  // Analog input pin that the potentiometer is attached to

// How many NeoPixels are attached to the Arduino?
#define LED_COUNT 30

// Declare our NeoPixel stripRight object:
Adafruit_NeoPixel stripLeft( LED_COUNT, LED_PIN_LEFT,  NEO_GRB + NEO_KHZ800);
Adafruit_NeoPixel stripRight(LED_COUNT, LED_PIN_RIGHT, NEO_GRB + NEO_KHZ800);
// Argument 1 = Number of pixels in NeoPixel stripRight
// Argument 2 = Arduino pin number (most are valid)
// Argument 3 = Pixel type flags, add together as needed:
//   NEO_KHZ800  800 KHz bitstream (most NeoPixel products w/WS2812 LEDs)
//   NEO_KHZ400  400 KHz (classic 'v1' (not v2) FLORA pixels, WS2811 drivers)
//   NEO_GRB     Pixels are wired for GRB bitstream (most NeoPixel products)
//   NEO_RGB     Pixels are wired for RGB bitstream (v1 FLORA pixels, not v2)
//   NEO_RGBW    Pixels are wired for RGBW bitstream (NeoPixel RGBW products)


// setup() function -- runs once at startup --------------------------------

void setup() {
  // These lines are specifically to support the Adafruit Trinket 5V 16 MHz.
  // Any other board, you can remove this part (but no harm leaving it):
#if defined(__AVR_ATtiny85__) && (F_CPU == 16000000)
  clock_prescale_set(clock_div_1);
#endif
  // END of Trinket-specific code.

  stripLeft.begin();           // INITIALIZE NeoPixel stripRight object (REQUIRED)
  stripLeft.show();            // Turn OFF all pixels ASAP
  stripLeft.setBrightness(50); // Set BRIGHTNESS to about 1/5 (max = 255)
  
  stripRight.begin();           // INITIALIZE NeoPixel stripRight object (REQUIRED)
  stripRight.show();            // Turn OFF all pixels ASAP
  stripRight.setBrightness(50); // Set BRIGHTNESS to about 1/5 (max = 255)
  
  Serial.begin(9600);
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

void loop() {
  int sensorValueLeft = analogRead(analogInPinLeft);
  int sensorValueRight = analogRead(analogInPinRight);

  float distLeft  = adVoltageToDistMeter(sensorValueLeft);
  float distRight = adVoltageToDistMeter(sensorValueRight);

  float joyLeft  = min(mapFloat(distLeft,  0.1, 1.0, 1.0, 0), 1.0);
  float joyRight = min(mapFloat(distRight, 0.1, 1.0, 1.0, 0), 1.0);

  float filteredLeft = filter(joyLeft, filterBufferLeft, FILTER_BUFFER_SIZE);
  float filteredRight = filter(joyRight, filterBufferRight, FILTER_BUFFER_SIZE);

  dispLevel(joyLeft, joyRight);

  Serial.print(filteredLeft);
  Serial.print(",");
  Serial.print(filteredRight);
  Serial.print("\n");

//  dispLevel(distRight / );

  delay(50);
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

void dispLevel(float levelLeft, float levelRight) {
  //  level: the level of dist meter (0.0~1.0)
  uint32_t colorLeft  =  stripLeft.Color(255,   0,   0);
  uint32_t colorRight = stripRight.Color(255,   0,   0);
  uint32_t numLeft  = uint32_t(levelLeft * 30.0);
  uint32_t numRight = uint32_t(levelRight * 30.0);
  
  stripLeft.clear();
  if(numLeft > 0) {
    stripLeft.fill(colorLeft, 0, numLeft);
  }
  stripLeft.show();

  stripRight.clear();
  if(numRight > 0) {
    stripRight.fill(colorRight, 0, numRight);
  }
  stripRight.show();
}

// Some functions of our own for creating animated effects -----------------

// Fill stripRight pixels one after another with a color. stripRight is NOT cleared
// first; anything there will be covered pixel by pixel. Pass in color
// (as a single 'packed' 32-bit value, which you can get by calling
// stripRight.Color(red, green, blue) as shown in the loop() function above),
// and a delay time (in milliseconds) between pixels.
void colorWipe(uint32_t color, int wait) {
  for(int i=0; i<stripRight.numPixels(); i++) { // For each pixel in stripRight...
    stripRight.setPixelColor(i, color);         //  Set pixel's color (in RAM)
    stripRight.show();                          //  Update stripRight to match
    delay(wait);                           //  Pause for a moment
  }
}

// Theater-marquee-style chasing lights. Pass in a color (32-bit value,
// a la stripRight.Color(r,g,b) as mentioned above), and a delay time (in ms)
// between frames.
void theaterChase(uint32_t color, int wait) {
  for(int a=0; a<10; a++) {  // Repeat 10 times...
    for(int b=0; b<3; b++) { //  'b' counts from 0 to 2...
      stripRight.clear();         //   Set all pixels in RAM to 0 (off)
      // 'c' counts up from 'b' to end of stripRight in steps of 3...
      for(int c=b; c<stripRight.numPixels(); c += 3) {
        stripRight.setPixelColor(c, color); // Set pixel 'c' to value 'color'
      }
      stripRight.show(); // Update stripRight with new contents
      delay(wait);  // Pause for a moment
    }
  }
}

// Rainbow cycle along whole stripRight. Pass delay time (in ms) between frames.
void rainbow(int wait) {
  // Hue of first pixel runs 5 complete loops through the color wheel.
  // Color wheel has a range of 65536 but it's OK if we roll over, so
  // just count from 0 to 5*65536. Adding 256 to firstPixelHue each time
  // means we'll make 5*65536/256 = 1280 passes through this outer loop:
  for(long firstPixelHue = 0; firstPixelHue < 5*65536; firstPixelHue += 256) {
    for(int i=0; i<stripRight.numPixels(); i++) { // For each pixel in stripRight...
      // Offset pixel hue by an amount to make one full revolution of the
      // color wheel (range of 65536) along the length of the stripRight
      // (stripRight.numPixels() steps):
      int pixelHue = firstPixelHue + (i * 65536L / stripRight.numPixels());
      // stripRight.ColorHSV() can take 1 or 3 arguments: a hue (0 to 65535) or
      // optionally add saturation and value (brightness) (each 0 to 255).
      // Here we're using just the single-argument hue variant. The result
      // is passed through stripRight.gamma32() to provide 'truer' colors
      // before assigning to each pixel:
      stripRight.setPixelColor(i, stripRight.gamma32(stripRight.ColorHSV(pixelHue)));
    }
    stripRight.show(); // Update stripRight with new contents
    delay(wait);  // Pause for a moment
  }
}

// Rainbow-enhanced theater marquee. Pass delay time (in ms) between frames.
void theaterChaseRainbow(int wait) {
  int firstPixelHue = 0;     // First pixel starts at red (hue 0)
  for(int a=0; a<30; a++) {  // Repeat 30 times...
    for(int b=0; b<3; b++) { //  'b' counts from 0 to 2...
      stripRight.clear();         //   Set all pixels in RAM to 0 (off)
      // 'c' counts up from 'b' to end of stripRight in increments of 3...
      for(int c=b; c<stripRight.numPixels(); c += 3) {
        // hue of pixel 'c' is offset by an amount to make one full
        // revolution of the color wheel (range 65536) along the length
        // of the stripRight (stripRight.numPixels() steps):
        int      hue   = firstPixelHue + c * 65536L / stripRight.numPixels();
        uint32_t color = stripRight.gamma32(stripRight.ColorHSV(hue)); // hue -> RGB
        stripRight.setPixelColor(c, color); // Set pixel 'c' to value 'color'
      }
      stripRight.show();                // Update stripRight with new contents
      delay(wait);                 // Pause for a moment
      firstPixelHue += 65536 / 90; // One cycle of color wheel over 90 frames
    }
  }
}
