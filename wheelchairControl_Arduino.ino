// Digital pin #2 is the same as Pin D2 see
// http://arduino.cc/en/Hacking/PinMapping168 for the 'raw' pin mapping

// Using the Adafruit Motor Shield v2:
// http://www.adafruit.com/products/1438

#include <Servo.h>
#include <Wire.h>
#include <Adafruit_MotorShield.h>

#define MODE_NONE 0
#define MODE_DRIVE 1
#define MODE_ARM 2
#define MODE_POWER 3

uint8_t controlMode = MODE_NONE;

#define DATA_PIN_REG PIND
#define DATA_DDR_REG DDRD
#define DATA_PORT_REG PORTD
#define DATA_PIN 2
#define BIT_WIDTH 26 //uS
#define DELAY_CMD 8 //ms

#define POWER1_PIN 5
#define POWER2_PIN 6
#define POWER3_PIN 7

#define NUM_PACKETS 6
#define LEN_PACKET (3+8+1)
#define LEN_CMD (LEN_PACKET*NUM_PACKETS)

#define CMD_INIT   "11001001010111010110101111."

#define RX_TIMEOUT 500

char driveCmdBuf[LEN_CMD];

#define ARM_REACH_MOTOR 1
#define ARM_ELEVATION_MOTOR 2
#define ARM_TURN_MOTOR 3
#define ARM_CLAW_MOTOR 4

Adafruit_MotorShield AFMS = Adafruit_MotorShield(); 
Adafruit_DCMotor *armReachMotor = AFMS.getMotor(ARM_REACH_MOTOR);
Adafruit_DCMotor *armElevationMotor = AFMS.getMotor(ARM_ELEVATION_MOTOR);
Adafruit_DCMotor *armTurnMotor = AFMS.getMotor(ARM_TURN_MOTOR);
Adafruit_DCMotor *armClawMotor = AFMS.getMotor(ARM_CLAW_MOTOR);

const uint8_t driveSpeed = 10;

const uint8_t armClawSpeed = 32;
const uint8_t armTurnSpeed = 64;
const uint8_t armElevationSpeed = 96;
const uint8_t armReachSpeed = 128;

uint32_t lastRx;

int IRsignal_power[] = {
	9076, 4520,
	564, 1676,
	592, 568,
	536, 568,
	564, 556,
	568, 564,
	540, 564,
	568, 560,
	564, 568,
	544, 560,
	564, 1708,
	540, 1680,
	588, 1656,
	592, 1680,
	568, 1680,
	588, 1656,
	592, 1684,
	564, 1680,
	588, 544,
	568, 1676,
	592, 1656,
	592, 1680,
	568, 564,
	572, 556,
	564, 568,
	536, 568,
	564, 1680,
	568, 564,
	568, 560,
	564, 568,
	544, 1676,
	592, 1652,
	596, 1676,
	572, 40220,
	9072, 2236,
	588, 0};

int IRsignal_volUp[] = {
	9076, 4512,
	572, 1676,
	624, 532,
	540, 564,
	568, 536,
	616, 540,
	544, 560,
	572, 532,
	624, 508,
	564, 564,
	568, 1680,
	568, 1676,
	624, 1624,
	592, 1680,
	568, 1676,
	624, 1624,
	592, 1680,
	568, 564,
	572, 1672,
	564, 1684,
	616, 1628,
	596, 564,
	592, 508,
	572, 560,
	564, 540,
	624, 1620,
	600, 560,
	592, 512,
	572, 556,
	564, 1684,
	564, 1680,
	620, 1628,
	588, 1684,
	564, 40228,
	9064, 2240,
	596, 0};

int IRsignal_volDown[] = {
	9072, 4516,
	572, 1676,
	592, 564,
	536, 568,
	568, 560,
	572, 560,
	544, 560,
	572, 560,
	564, 564,
	536, 568,
	568, 1680,
	568, 1676,
	592, 1656,
	596, 1680,
	564, 1680,
	588, 1652,
	588, 1684,
	564, 568,
	568, 560,
	560, 1660,
	588, 1684,
	564, 568,
	568, 560,
	572, 560,
	544, 560,
	572, 1672,
	564, 1684,
	596, 560,
	544, 560,
	572, 1672,
	564, 1684,
	596, 1648,
	600, 1676,
	572, 40220,
	9068, 2236,
	592, 0};

#define IR_FREQ 38 //kHz

//digital pin 12
#define IRledPin_PORT PORTB
#define IRledPin 4

// This procedure sends a xKHz pulse to the IRledPin 
// for a certain # of microseconds. We'll use this whenever we need to send codes
void pulseIR(long microsecs)
{
  // we'll count down from the number of microseconds we are told to wait
 
  int waveLength = 1000 / IR_FREQ;
  int halfWaveLength = waveLength / 2;
  //Serial.println(waveLength);
 
  cli();  // this turns off any background interrupts

  while (microsecs > 0)
  {
   IRledPin_PORT |= _BV(IRledPin);
   delayMicroseconds(halfWaveLength);
   IRledPin_PORT &= ~_BV(IRledPin);
   delayMicroseconds(halfWaveLength);
   
   microsecs -= waveLength;
  }
 
  sei();  // this turns them back on
}

// 
void SendIRCode(const int *code)
{
  const int *pulse = code;
  while (true)
  {
    int on = *(pulse++);
    int off = *(pulse++);
    pulseIR(on);
    delayMicroseconds(off);
    if (off == 0)
      return;
  }
}

//
char pow2(int p)
{
  int retval = 1;
  for (int i = 0; i < p; i++)
    retval *= 2;
  return retval;
}

//
void charToString(const char val, char *string)
{
 for (int i = 0; i < 8; i++)
   string[i] = (val & pow2(i)) ? '1' : '0';
}

//
void charToStringI(const char val, char *string)
{
 for (int i = 0; i < 8; i++)
   string[i] = (val & pow2(i)) ? '0' : '1';
}

//
void stringToChar(const char *string, char &val)
{
  val = 0;
 for (int i = 0; i < 8; i++)
  if (string[i] == '1')
   val += pow2(i);
}

//example:
//    start          horn           speed[4]       y              x              checksum     
//110 01010010 1 110 10000000 1 110 11100000 1 110 00101011 0 110 01000000 1 110 11101011 0 
void buildDriveCmd(char *buf, char speed, char y, char x)
{
  char tmp[8];
  
  //packet starts
  for (int p = 0; p < NUM_PACKETS; p++)
  {
    buf[p*LEN_PACKET + 0] = '1';
    buf[p*LEN_PACKET + 1] = '1';
    buf[p*LEN_PACKET + 2] = '0';
  }
  
  //start packet
  memcpy(buf + (0*LEN_PACKET + 3), "01010010", 8);
  
  //horn packet
  memcpy(buf + (1*LEN_PACKET + 3), "10000000", 8);
  
  //speed packet
  charToString(speed, tmp);
  memcpy(buf + (2*LEN_PACKET + 3), tmp + 4, 4);
  memcpy(buf + (2*LEN_PACKET + 3) + 4, "0000", 4);
  
  //y packet
  charToString(y, tmp);
  memcpy(buf + (3*LEN_PACKET + 3), tmp, 8);

  //x packet
  charToString(x, tmp);
  memcpy(buf + (4*LEN_PACKET + 3), tmp, 8);
  
  //checksum
  char checkVal = 0;
  for (int p = 0; p < NUM_PACKETS - 1; p++)
  {
    char val;
    stringToChar(buf + (p*LEN_PACKET + 3), val);
    checkVal += val;
  }
  charToStringI(checkVal, buf + ((NUM_PACKETS - 1) * LEN_PACKET + 3));
  
  //packet parity
  for (int p = 0; p < NUM_PACKETS; p++)
  {
    bool even = true;
    for (int b = 0; b < 8; b++)
      if (buf[p*LEN_PACKET + 3 + b] == '1')
        even = !even;
    
    buf[p*LEN_PACKET + LEN_PACKET - 1] = even ? '0' : '1';
  }  
}

//
uint32_t lastTx;
void sendDriveCmd(const char *bits)
{  
  lastTx = millis();
  cli();
  DATA_DDR_REG |= (1 << DATA_PIN);

  for (int b = 0; b < LEN_CMD; b++)
  {
    if (*bits == '0')
      DATA_PORT_REG &= ~(1 << DATA_PIN);//lo
    else if (*bits == '1')
      DATA_PORT_REG |= (1 << DATA_PIN);//hi
    else
      break;
      
    delayMicroseconds(BIT_WIDTH);
    bits++;
  }

  DATA_PORT_REG |= (1 << DATA_PIN);//hi
  delayMicroseconds(BIT_WIDTH * 6);

  DATA_DDR_REG &= ~(1 << DATA_PIN);//release
  DATA_PORT_REG &= ~(1 << DATA_PIN);
  sei();
}

//
void armOff()
{
  armReachMotor->run(RELEASE);
  armElevationMotor->run(RELEASE);
  armTurnMotor->run(RELEASE);
  armClawMotor->run(RELEASE);
}

//
void driveOff()
{
  buildDriveCmd(driveCmdBuf, driveSpeed, 0, 0);

  while (millis() - lastTx < DELAY_CMD);

  //sending many times, just in case
  for (int i = 0; i < 10; i++)
  {
    sendDriveCmd(driveCmdBuf);
    delay(DELAY_CMD);
  }
}

//
void allOff()
{
  driveOff();
  armOff();
}

//
void sendStartSequence()
{
  for (int i = 0; i < 100; i++)
  {
    sendDriveCmd(CMD_INIT);
    delay(DELAY_CMD);
  }
}

//
boolean processCommonData(char rxData)
{
    switch (rxData)
    {
        case '0':
          allOff();
          controlMode = MODE_NONE;
          return true;   

        case '1':
          allOff();
          controlMode = MODE_DRIVE;
          sendStartSequence();
          return true;   
          
        case '2':
          allOff();
          controlMode = MODE_ARM;
          return true;

        case '3':
          allOff();
          controlMode = MODE_POWER;
          return true;
          
        default:
          return false;
    }
}

//
void updateMode0()
{
  if (Serial.available())
  {    
    char rxData = Serial.read();
    lastRx = millis();
    if (processCommonData(rxData))
      return;
  }
}

//
void updateDriveMode()
{
  if (Serial.available())
  {    
    char rxData = Serial.read();
    lastRx = millis();
    if (processCommonData(rxData))
      return;
      
    switch (rxData)
    {
        //drive commands
        case 'u': buildDriveCmd(driveCmdBuf, driveSpeed, 100, 0); break;
        case 'd': buildDriveCmd(driveCmdBuf, driveSpeed, -100, 0); break;
        case 'l': buildDriveCmd(driveCmdBuf, driveSpeed, 0, -100); break;
        case 'r': buildDriveCmd(driveCmdBuf, driveSpeed, 0, 100); break;
        
        //in case Arduino terminal is used
        case '\r':
        case '\n':
            break;
        
        //cut everything!
        default:
          allOff();
          break;
    }
  }
  
  if (millis() - lastTx > DELAY_CMD)
    sendDriveCmd(driveCmdBuf);
}

//
void updateArmMode()
{
  if (Serial.available())
  {
    char rxData = Serial.read();
    lastRx = millis();
    if (processCommonData(rxData))
      return;
  
    switch (rxData)
    {
        //arm commands
        case 'D': armElevationMotor->run(FORWARD); break;
        case 'U': armElevationMotor->run(BACKWARD); break;
        case 'L': armTurnMotor->run(BACKWARD); break;
        case 'R': armTurnMotor->run(FORWARD); break;
        case 'B': armReachMotor->run(FORWARD); break;
        case 'F': armReachMotor->run(BACKWARD); break;
        case 'C': armClawMotor->run(FORWARD); break;
        case 'O': armClawMotor->run(BACKWARD); break;
            
        //in case Arduino terminal is used
        case '\r':
        case '\n':
            break;
        
        //cut everything!
        default:
          allOff();
          break;
    }
  }
}

//
void pulsePowerPin(uint8_t pin)
{
  digitalWrite(pin, HIGH);
  delay(500);
  digitalWrite(pin, LOW);
}

//
void updatePowerMode()
{
  if (Serial.available())
  {
    char rxData = Serial.read();
    lastRx = millis();
    if (processCommonData(rxData))
      return;
  
    switch (rxData)
    {
        //power commands
        case 'x': pulsePowerPin(POWER1_PIN); break;
        case 'y': pulsePowerPin(POWER2_PIN); break;
        case 'z': pulsePowerPin(POWER3_PIN); break;
            
        //radio commands
        case 'm': SendIRCode(IRsignal_power); break;
        case 'n': SendIRCode(IRsignal_volUp); break;
        case 'o': SendIRCode(IRsignal_volDown); break;

        //in case Arduino terminal is used
        case '\r':
        case '\n':
            break;
        
        //cut everything!
        default:
          allOff();
          break;
    }
  }
}

//
void setup()
{
  //IR setup
  DDRB |= _BV(IRledPin);  
  
  //setup wireless  
  Serial.begin(115200);
  Serial.println("ready");
  
  pinMode(POWER1_PIN, OUTPUT);
  pinMode(POWER2_PIN, OUTPUT);
  pinMode(POWER3_PIN, OUTPUT);
  
  AFMS.begin();
  armReachMotor->run(RELEASE);
  armReachMotor->setSpeed(armReachSpeed);
  armElevationMotor->run(RELEASE);
  armElevationMotor->setSpeed(armElevationSpeed);
  armTurnMotor->run(RELEASE);
  armTurnMotor->setSpeed(armTurnSpeed);
  armClawMotor->run(RELEASE);
  armClawMotor->setSpeed(armTurnSpeed);
      
  buildDriveCmd(driveCmdBuf, driveSpeed, 0, 0);//init
  
  //process commands
  while (1)
  {
    switch (controlMode)
    {
      case 1: updateDriveMode(); break;
      case 2: updateArmMode(); break;
      case 3: updatePowerMode(); break;
      default: updateMode0(); break;
    }
    
    //cut everything
    if (millis() - lastRx > RX_TIMEOUT)
      allOff();
  }
  
  while (1) {}
}

//
void loop()
{
}

