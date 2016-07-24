
#define HT1632_READ  0x6
#define HT1632_WRITE 0x5
#define HT1632_COMMAND 0x4

#define HT1632_SYS_DIS 0x00
#define HT1632_SYS_EN 0x01
#define HT1632_LED_OFF 0x02
#define HT1632_LED_ON 0x03
#define HT1632_BLINK_OFF 0x08
#define HT1632_BLINK_ON 0x09
#define HT1632_SLAVE_MODE 0x10
#define HT1632_MASTER_MODE 0x14
#define HT1632_INT_RC 0x18
#define HT1632_EXT_CLK 0x1C
#define HT1632_PWM_CONTROL 0xA0

#define HT1632_COMMON_8NMOS  0x20
#define HT1632_COMMON_16NMOS  0x24
#define HT1632_COMMON_8PMOS  0x28
#define HT1632_COMMON_16PMOS  0x2C

#define WIDTH  32
#define HEIGHT  8

// HT1632 interface
#define _data 13
#define _wr   12
#define _cs   11

// Buttons
// The buttons are active low and injactive high impedance
// It is important to activate the internal pullups for proper signals
#define BUTTON_PIN_DOWN 5
#define BUTTON_PIN_UP   6
#define BUTTON_PIN_SET  7

#define BUTTON_DOWN 0b00000001
#define BUTTON_UP   0b00000010
#define BUTTON_SET  0b00000100

// LED Pin
// These are NOT two leds
// LED_PIN1---[R]--->|---LED_PIN2
// I would recommend a bicolor led
#define LED_PIN1    16 // Green 
#define LED_PIN2    17 // Red


#define RELAIS_PIN 3

#define scrollSpeed 15
#define scrollDelay 600
#define dimSpeed 40



byte myFont[] = {
  0x78,0xCC,0xDC,0xFC,0xEC,0xCC,0x78,0x00,  // 0x30 0
  0x30,0xF0,0x30,0x30,0x30,0x30,0xFC,0x00,  // 0x31 1
  0x78,0xCC,0x0C,0x38,0x60,0xCC,0xFC,0x00,  // 0x32 2
  0x78,0xCC,0x0C,0x38,0x0C,0xCC,0x78,0x00,  // 0x33 3
  0x1C,0x3C,0x6C,0xCC,0xFE,0x0C,0x0C,0x00,  // 0x34 4
  0xFC,0xC0,0xF8,0x0C,0x0C,0xCC,0x78,0x00,  // 0x35 5
  0x38,0x60,0xC0,0xF8,0xCC,0xCC,0x78,0x00,  // 0x36 6
  0xFC,0xCC,0x0C,0x18,0x30,0x60,0x60,0x00,  // 0x37 7
  0x78,0xCC,0xCC,0x78,0xCC,0xCC,0x78,0x00,  // 0x38 8
  0x78,0xCC,0xCC,0x7C,0x0C,0x18,0x70,0x00,  // 0x39 9
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,  // 0x00 
};


unsigned long currentMillis   = 0; // time since last reset in ms

// For Scroll Delay
unsigned long previousMillis1 = 0;

// Button Debouncing
unsigned long previousMillis2 = 0;
int buttonDelay      = 200;

byte pressedButton = 0;
byte brightnes = 15;
byte oldBrightnes = 15;

enum e_Mode {eSetup, eSetBrightness, eTextScroller};
int eMode = eTextScroller;

byte frameBuffer[6][8] = {0};

byte ledmatrix[32];


//##########################################################################################

void setup()
{
  pinMode(RELAIS_PIN, OUTPUT);
  digitalWrite(RELAIS_PIN, HIGH);


  HT1632begin();
}

void loop()
{
  byte counter = 6;
  
  while(counter)
  {
    counter--;
    
    loadDigitToFrameBuffer(5, counter);
    loadFrameBufferToMatrix();
    for (byte i = 0; i<27; i++)
    {
      shiftFrameBuffer();
      loadFrameBufferToMatrix();
      delay(scrollSpeed);
    }
    delay(scrollDelay);
  }
  
  
  oldBrightnes = brightnes;
  while(brightnes)
  {
    brightnes--;
    sendcommand(HT1632_PWM_CONTROL | brightnes);
    delay(dimSpeed);
  }
  clearFrameBuffer();
  loadFrameBufferToMatrix();

  brightnes = oldBrightnes;
  sendcommand(HT1632_PWM_CONTROL | brightnes);

  digitalWrite(RELAIS_PIN, LOW);
  delay(1000);
  digitalWrite(RELAIS_PIN, HIGH);

  while(1);  
}

void HT1632begin()
{
  pinMode(_cs, OUTPUT);
  digitalWrite(_cs, HIGH);
  pinMode(_wr, OUTPUT);
  digitalWrite(_wr, HIGH);
  pinMode(_data, OUTPUT);
  
  sendcommand(HT1632_SYS_EN);
  sendcommand(HT1632_LED_ON);
  sendcommand(HT1632_BLINK_OFF);
  sendcommand(HT1632_MASTER_MODE);
  sendcommand(HT1632_INT_RC);
  sendcommand(HT1632_COMMON_8NMOS);
  sendcommand(HT1632_PWM_CONTROL | brightnes);
}

void sendcommand(uint8_t cmd)
{
  uint16_t data = 0;
  data = HT1632_COMMAND;
  data <<= 8;
  data |= cmd;
  data <<= 1;
  
  digitalWrite(_cs, LOW);
  writedata(data, 12);
  digitalWrite(_cs, HIGH);  
}

void writeScreen()
{
  digitalWrite(_cs, LOW);

  writedata(HT1632_WRITE, 3);
  // send with address 0
  writedata(0, 7);

  for (uint16_t i=0; i<(WIDTH*HEIGHT/8); i+=2) {
    uint16_t d = ledmatrix[i];
    d <<= 8;
    d |= ledmatrix[i+1];

    writedata(d, 16);
  }
  digitalWrite(_cs, HIGH);
}

void writedata(uint16_t d, uint8_t bits)
{
  uint16_t compareBit = 0;
  compareBit = bits == 16 ? 0x8000 : compareBit |= (1<<(bits-1)); // 0x8000 -> set MSB
  
  pinMode(_data, OUTPUT);
  for (uint8_t i=bits; i > 0; i--)
  {
    digitalWrite(_wr, LOW);
    if (d & compareBit) { digitalWrite(_data, HIGH); } // 1
    else { digitalWrite(_data, LOW); } // 0
    compareBit = compareBit >> 1;
    digitalWrite(_wr, HIGH);
  }
  pinMode(_data, INPUT);
}

void loadDigitToFrameBuffer(byte digit, byte character)
// This function loads a single character into the selected framebuffer digit
{
  for (byte row=0; row<8; row++)
  {
    frameBuffer[digit][row] = myFont[(character*8)+row];
  }
}

void HT1632_setBrightness(uint8_t pwm) {
  if (pwm > 15) pwm = 15;
  sendcommand(HT1632_PWM_CONTROL | pwm);
}

void loadFrameBufferToMatrix(void)
{
  for (int i=0; i<4; i++) // Write from the frame buffer to the display
  {
    for (char row=0; row <= 7; row++)
    {
      ledmatrix[row+(8*i)] = frameBuffer[i][row];
    }
  }
  writeScreen();
}

byte shiftFrameBuffer() {return shiftFrameBuffer(0);} // 0 default left shift
byte shiftFrameBuffer(byte dir)
{
  static byte shiftCounter = 1;
  
  for (byte digit = 0; digit <= (sizeof(frameBuffer)/8)-1 ; digit++) // Cycle through the digits of the frame buffer
  {
    for (byte rowCounter = 0; rowCounter <= 7; rowCounter++)
    {
      // roll the 8bits...
      // The information how to roll is from http://arduino.cc/forum/index.php?topic=124188.0 
      // rowBuffer[rowCounter] = ((rowBuffer[rowCounter] & 0x80)?0x01:0x00) | (rowBuffer[rowCounter] << 1);
      
      if (digit == 0) // If the first Digit just shift out the MSB
      {
        frameBuffer[digit][rowCounter] = frameBuffer[digit][rowCounter] << 1;
      }
      else
      {
        // check if MSB is true and set the LSB of previours digit
        if (frameBuffer[digit][rowCounter] & 0x80)
        {
          frameBuffer[digit-1][rowCounter] = (frameBuffer[digit-1][rowCounter] | 1);
        }
        frameBuffer[digit][rowCounter] = frameBuffer[digit][rowCounter] << 1;
      }
    }    
  }
  shiftCounter++;
  if (shiftCounter == 9)
    shiftCounter = 1;
  return shiftCounter;
}

void clearFrameBuffer()
// This Function clears the frame buffer
{
  //Serial.println(sizeof(frameBuffer)/8);
  char fbSize = sizeof(frameBuffer)/8;
  
  for (char i = 0; i<fbSize; i++)
  {
    loadDigitToFrameBuffer(i, 10);
  }
}


