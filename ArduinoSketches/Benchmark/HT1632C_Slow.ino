
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

// LED Pin
// These are NOT two leds
// LED_PIN1---[R]--->|---LED_PIN2
// I would recommend a bicolor led
#define LED_PIN1    16 // Green 
#define LED_PIN2    17 // Red

byte ledmatrix[40];

void setup()
{
  pinMode(LED_PIN1, OUTPUT);
  pinMode(LED_PIN2, OUTPUT);
  digitalWrite(LED_PIN1, HIGH);
  delay(100);
  digitalWrite(LED_PIN2, HIGH);
  digitalWrite(LED_PIN1, LOW);

  HT1632begin();
  
  // Write the values 1-31 into the frame buffer array
  for(byte i = 0; i <= 31; i++) { ledmatrix[i] = i; }
}

void loop()
{
  PORTC |= (1<<PC2);
  writeScreen();
  PORTC &= ~(1<<PC2);
  delay(250);
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
  sendcommand(HT1632_PWM_CONTROL | 0xF);
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
  pinMode(_data, OUTPUT);
  for (uint8_t i=bits; i > 0; i--) {
    digitalWrite(_wr, LOW);
    if (d & _BV(i-1)) { digitalWrite(_data, HIGH); }
    else { digitalWrite(_data, LOW); }
    digitalWrite(_wr, HIGH);
  }
  pinMode(_data, INPUT);
}

