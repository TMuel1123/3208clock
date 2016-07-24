
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
#define scrollDelay 500
#define dimSpeed 40

byte myFont[11][8] = {
  {0x78,0xCC,0xDC,0xFC,0xEC,0xCC,0x78,0x00},	// 0
  {0x30,0xF0,0x30,0x30,0x30,0x30,0xFC,0x00},	// 1
  {0x78,0xCC,0x0C,0x38,0x60,0xCC,0xFC,0x00},	// 2
  {0x78,0xCC,0x0C,0x38,0x0C,0xCC,0x78,0x00},	// 3
  {0x1C,0x3C,0x6C,0xCC,0xFE,0x0C,0x0C,0x00},	// 4
  {0xFC,0xC0,0xF8,0x0C,0x0C,0xCC,0x78,0x00},	// 5
  {0x38,0x60,0xC0,0xF8,0xCC,0xCC,0x78,0x00},	// 6
  {0xFC,0xCC,0x0C,0x18,0x30,0x60,0x60,0x00},	// 7
  {0x78,0xCC,0xCC,0x78,0xCC,0xCC,0x78,0x00},	// 8
  {0x78,0xCC,0xCC,0x7C,0x0C,0x18,0x70,0x00},	// 9
  {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}	//  
};

byte brightnes = 15;
byte oldBrightnes = 0;

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
  // Start des Countdowns setzen
  byte counter = 3;
  
  // Zuvor eingestellte helligkeit merken und helligkeit dann auf minimum setzen
  oldBrightnes = brightnes;
  sendcommand(HT1632_PWM_CONTROL | 0);

  // Erste zahl in den FrameBuffer schreiben und dann 3 schieben, damit es schön in der Mitte ist
  loadDigitToFrameBuffer(2, counter);
  for (byte i = 0; i<3; i++)
  {
    shiftFrameBuffer();
  }
  loadFrameBufferToMatrix();

  // Zahl einblenden
  for(brightnes = 0; oldBrightnes >= brightnes; brightnes++)
  {
    sendcommand(HT1632_PWM_CONTROL | brightnes);
    delay(dimSpeed);
  }
  
  // Runter zählen bis 0 und die Zahlen dabei immer schön durchschieben
  while(counter)
  {
    counter--;
    
    loadDigitToFrameBuffer(5, counter);
    loadFrameBufferToMatrix();
    for (byte i = 0; i<27; i++) // 27 schieben weils dann schön in die Mitte passt
    {
      shiftFrameBuffer();
      loadFrameBufferToMatrix();
      delay(scrollSpeed);
    }
    delay(scrollDelay);
  }
  
  // Helligkeit merken...
  oldBrightnes = brightnes;
  // ...und die 0 langsam ausblenden
  while(brightnes)
  {
    brightnes--;
    sendcommand(HT1632_PWM_CONTROL | brightnes);
    delay(dimSpeed);
  }
  // Nach dem ausblenden Matrix löschen //
  clearFrameBuffer();
  loadFrameBufferToMatrix();
  
  // und alte Helligkeit wieder herstellen
  brightnes = oldBrightnes;
  sendcommand(HT1632_PWM_CONTROL | brightnes);

  // Relais für eine Sekunde anziehen lassen
  digitalWrite(RELAIS_PIN, LOW);
  delay(1000);
  digitalWrite(RELAIS_PIN, HIGH);

  // Warten bis zum nächsten Reset
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
//  character = character*8;
  
  for (byte row=0; row<8; row++)
  {
    frameBuffer[digit][row] = myFont[character][row];
  }
}

void HT1632_setBrightness(uint8_t pwm)
{
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

byte shiftFrameBuffer()
{
  for (byte digit = 0; digit <= (sizeof(frameBuffer)/8)-1 ; digit++) // Cycle through the digits of the frame buffer
  {
    for (byte rowCounter = 0; rowCounter <= 7; rowCounter++)
    {
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
}

void clearFrameBuffer()
// This Function clears the frame buffer
{
  char fbSize = sizeof(frameBuffer)/8;
  
  for (char i = 0; i<fbSize; i++)
  {
    loadDigitToFrameBuffer(i, 10);
  }
}


