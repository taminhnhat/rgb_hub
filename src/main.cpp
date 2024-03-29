#include <Arduino.h>
#include <FastLED.h>
#include <EEPROM.h>

#define NUM_OF_ROW 6
#define NUM_OF_LED_PER_STRIP 330
#define ENABLE_SERIAL_1 true
#define BASE_COLOR "000000"

CRGB leds[NUM_OF_ROW][NUM_OF_LED_PER_STRIP];
CRGB statusLight[12];

String messageFromSerial0 = "";
String messageFromSerial1 = "";
struct RGB_COLOR
{
  uint8_t red = 0;
  uint8_t green = 0;
  uint8_t blue = 0;
} rgbColor;

String lightColor[8] = {"000000", "00ff00", "0000ff", "ffff00", "ff00ff", "00ffff", "ff0000", "ffffff"};

uint8_t numOfLedPerNode = 0x00;
uint8_t numOfColumnOnWall = 0x00;
uint8_t brightnessOfLed = 0x00;
uint16_t numOfLedPerStrip = 0x00;
uint8_t dashWidth = 1;

int reloadConfigFromEeprom();
void msgProcess(String, Stream &);
int ledStripGenerate(String, int, uint16_t, uint16_t);
int ledStripApply();
int statusLightGenerate(String);
int initEEPROM();
int getValOfCfg(char, String);
int eepromWriteUint16(int, uint16_t);
uint16_t eepromReadUint16(int);
long timeStamp = 0;
long timeExec = 0;

void setup()
{
  Serial.begin(115200);
  if (ENABLE_SERIAL_1)
    Serial1.begin(115200);
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, 1);
  delay(1000);
  digitalWrite(LED_BUILTIN, 0);
  delay(1000);
  digitalWrite(LED_BUILTIN, 1);
  delay(1000);
  digitalWrite(LED_BUILTIN, 0);

  // eepromWriteUint16(0, 60); //
  // EEPROM.update(2, 6);      //
  // EEPROM.update(3, 10);     //
  // EEPROM.update(4, 50);     //
  reloadConfigFromEeprom();

  FastLED.addLeds<NEOPIXEL, 2>(leds[0], NUM_OF_LED_PER_STRIP);
  FastLED.addLeds<NEOPIXEL, 3>(leds[1], NUM_OF_LED_PER_STRIP);
  FastLED.addLeds<NEOPIXEL, 4>(leds[2], NUM_OF_LED_PER_STRIP);
  FastLED.addLeds<NEOPIXEL, 5>(leds[3], NUM_OF_LED_PER_STRIP);
  FastLED.addLeds<NEOPIXEL, 6>(leds[4], NUM_OF_LED_PER_STRIP);
  FastLED.addLeds<NEOPIXEL, 7>(leds[5], 12);
  ledStripGenerate("000000", 0, 0, numOfLedPerStrip - 1);
  ledStripGenerate("000000", 1, 0, numOfLedPerStrip - 1);
  ledStripGenerate("000000", 2, 0, numOfLedPerStrip - 1);
  ledStripGenerate("000000", 3, 0, numOfLedPerStrip - 1);
  ledStripGenerate("000000", 4, 0, numOfLedPerStrip - 1);
  ledStripGenerate("ff0000", 5, 0, 11);
  Serial.println("Start");
  if (ENABLE_SERIAL_1)
    Serial1.println("Start");
  ledStripApply();
}

void loop()
{
  // put your main code here, to run repeatedly:
  // if ((millis() - timeStamp) > 6000)
  // {
  //   ledStripGenerate("ff0000", 5, 0, 11);
  //   ledStripApply();
  // }
}

void serialEvent1()
{
  while (Serial1.available())
  {
    char tempChar = (char)Serial1.read();
    if (tempChar != '\n')
    {
      messageFromSerial1 += tempChar;
    }
    else
    {
      msgProcess(messageFromSerial1, Serial1);
      messageFromSerial1 = "";
    }
  }
}
void serialEvent()
{
  while (Serial.available())
  {
    char tempChar = (char)Serial.read();
    if (tempChar != '\n')
    {
      messageFromSerial0 += tempChar;
    }
    else
    {
      msgProcess(messageFromSerial0, Serial);
      messageFromSerial0 = "";
    }
  }
}

/**
 * @brief Reload configuration from eeprom
 *
 * @return int 1 if complete
 */
int reloadConfigFromEeprom()
{
  numOfLedPerStrip = eepromReadUint16(0); // first 2 bytes
  numOfColumnOnWall = EEPROM.read(2);     // 3rd byte
  numOfLedPerNode = EEPROM.read(3);       // 4rd byte
  brightnessOfLed = EEPROM.read(4);       // 5th byte
  if (brightnessOfLed > 250)
  {
    Serial.println("reset congiguration to factory!");
    eepromWriteUint16(0, 60); //
    EEPROM.update(2, 6);      //
    EEPROM.update(3, 10);     //
    EEPROM.update(4, 50);     //
    FastLED.setBrightness(50);
  }
  else
    FastLED.setBrightness(brightnessOfLed);
  // Serial.print("leds:");
  // Serial.println(numOfLedPerStrip);
  // Serial.print("cols:");
  // Serial.println(numOfColumnOnWall);
  // Serial.print("leds on node:");
  // Serial.println(numOfLedPerNode);
  // Serial.print("brightness:");
  // Serial.println(brightnessOfLed);
  return 1;
}

/**
 * @brief Processing light command
 *
 * @param lightCmd Input command
 */
void msgProcess(String lightCmd, Stream &stream)
{
  // stream.print("cmd:");
  // stream.println(lightCmd);

  if (lightCmd.startsWith(F("CFG")))
  {
    // stream.println("config mode");
    //  CONFIG MODE
    // CFG:T300.C10.N6.B100.b255
    char cfg[4] = {'T', 'C', 'N', 'B'};
    for (unsigned int x = 0; x < sizeof(cfg); x++)
    {
      if (lightCmd.indexOf(cfg[x], 3) >= 0)
      {
        if (x == 0)
        {
          eepromWriteUint16(0, (uint16_t)getValOfCfg(cfg[x], lightCmd));
        }
        else
        {
          EEPROM.update(x + 1, (byte)getValOfCfg(cfg[x], lightCmd));
        }
      }
    }
    reloadConfigFromEeprom();
    stream.println("OK");
  }
  else if (lightCmd.startsWith("GETINFO"))
  {
    // stream.println("get info mode");
    //  GET INFO MODE
    char cfg[4] = {'T', 'C', 'N', 'B'};
    stream.println(F("NAME:WALL_WS2812_MEGA2560"));
    stream.print(F("CONFIG:"));
    for (unsigned int x = 0; x < sizeof(cfg); x++)
    {
      if (x == 0)
      {
        stream.print(cfg[x]);
        stream.print(numOfLedPerStrip);
      }
      else
      {
        stream.print('.');
        stream.print(cfg[x]);
        stream.print(EEPROM.read(x + 1));
      }
    }
    stream.println();
  }
  else if (lightCmd.startsWith("STT"))
  {
    // timeExec = millis() - timeStamp;
    // timeStamp = millis();
    // stream.println(timeExec);
    stream.println("active");
  }
  else
  {
    if (lightCmd[0] == 'R')
    {
      // Serial.println("R mode");
      //  SET ROW MODE  CMD:"R1:00FF00:0000FF:0000FF:00FF00:00FF00:000000"
      uint8_t temp = (numOfLedPerStrip / numOfColumnOnWall) - numOfLedPerNode;
      int ledStripIndex = lightCmd[1] - '0' - 1;
      ledStripGenerate(BASE_COLOR, ledStripIndex, 0, numOfLedPerStrip - 1);
      if (ledStripIndex == 5)
      {
        String rgbVal = lightCmd.substring(3, 9);
        ledStripGenerate(rgbVal, 5, 0, 12);
      }
      else
      {
        for (int x = 0; x < numOfColumnOnWall; x++)
        {
          byte pos = 3 + x * 7;
          String rgbVal = lightCmd.substring(pos, pos + 6);
          uint16_t startLedIndex = (temp / 2) + (temp + numOfLedPerNode) * x;
          int stopLedIndex = numOfLedPerNode + startLedIndex - 1;
          // if (rgbVal.compareTo("000000"))
          //   rgbVal = BASE_COLOR;
          ledStripGenerate(rgbVal, ledStripIndex, startLedIndex, stopLedIndex);
        }
      }
      ledStripApply();
    }
    else if (lightCmd[0] == 'T')
    {
      // stream.println("T mode");
      //  SET ROW MODE  CMD:"R1:123:345:1:4:12345:35"
      int ledStripIndex = lightCmd[1] - '0' - 1;
      ledStripGenerate(BASE_COLOR, ledStripIndex, 0, numOfLedPerStrip - 1);
      byte startIndex = 2;
      for (int x = 0; x < numOfColumnOnWall; x++)
      {
        byte endIndex = lightCmd.indexOf(":", startIndex + 1);
        String lightIndex = lightCmd.substring(startIndex + 1, endIndex);
        // stream.print(startIndex);
        // stream.print(":");
        // stream.print(endIndex);
        // stream.print(":");
        // stream.println(lightIndex);
        uint8_t numOfLight = lightIndex.length();

        uint8_t numOfLedPerUser;
        switch (numOfLight)
        {
        case 1:
          numOfLedPerUser = numOfLedPerNode / 2;
          break;
        case 2:
          numOfLedPerUser = numOfLedPerNode / 2;
          break;
        case 3:
          numOfLedPerUser = numOfLedPerNode / 3;
          break;
        case 4:
          numOfLedPerUser = numOfLedPerNode / 4;
          break;
        case 5:
          numOfLedPerUser = numOfLedPerNode / 5;
          break;
        default:
          break;
        }

        uint8_t temp = (numOfLedPerStrip / numOfColumnOnWall) - numOfLedPerUser * numOfLight;
        uint16_t startLedIndex = (temp / 2) + (temp + numOfLedPerUser * numOfLight) * x;
        for (int j = 1; j <= numOfLight; j++)
        {
          uint8_t whichLight = lightIndex[j - 1] - '0';
          String rgbVal = lightColor[whichLight];
          int stopLedIndex = numOfLedPerUser + startLedIndex - 1;
          // if (rgbVal.compareTo("000000"))
          //   rgbVal = BASE_COLOR;
          // stream.print(startLedIndex);
          // stream.print('-');
          // stream.print(stopLedIndex);
          // stream.print('-');
          // stream.println(rgbVal);
          ledStripGenerate(rgbVal, ledStripIndex, startLedIndex, stopLedIndex);
          startLedIndex += numOfLedPerUser;
        }

        startIndex = endIndex;
      }
      ledStripApply();
    }
    // else if (lightCmd[0] == 'H')
    // {
    //   // stream.println("H mode");
    //   //  SET ROW MODE  CMD:"H1:00FF00:0000FF:0000FF:00FF00:00FF00:000000"
    //   uint8_t temp = (numOfLedPerStrip / numOfColumnOnWall) - numOfLedPerNode;
    //   int ledStripIndex = lightCmd[1] - '0' - 1;
    //   ledStripGenerate(BASE_COLOR, ledStripIndex, 0, numOfLedPerStrip - 1);
    //   for (int x = 0; x < numOfColumnOnWall; x++)
    //   {
    //     byte pos = 3 + x * 7;
    //     String rgbVal = lightCmd.substring(pos, pos + 6);
    //     uint16_t startLedIndex = (temp / 2) + (temp + numOfLedPerNode) * x;
    //     uint16_t startLedIndex_dash = startLedIndex;
    //     // if (rgbVal.compareTo("000000"))
    //     //   rgbVal = BASE_COLOR;
    //     const int temp = (numOfLedPerNode / 2) % 2;
    //     const int numOfLight = numOfLedPerNode / (2 * dashWidth) + temp;
    //     const int numOfDash = numOfLedPerNode / (2 * dashWidth);
    //     for (unsigned int idx = 1; idx <= numOfLight; idx++)
    //     {
    //       ledStripGenerate(rgbVal, ledStripIndex, startLedIndex, startLedIndex + dashWidth - 1);
    //       startLedIndex += 2 * dashWidth;
    //     }
    //     for (unsigned int idx = 1; idx <= numOfLight; idx++)
    //     {
    //       ledStripGenerate("00ff00", ledStripIndex, startLedIndex_dash + dashWidth, startLedIndex_dash + 2 * dashWidth - 1);
    //       startLedIndex_dash += 2 * dashWidth;
    //     }
    //   }
    //   ledStripApply();
    // }
    else if (lightCmd[0] == 'F')
    {
      // stream.println("F mode");
      //  RESET ROW MODE  CMD:"F1:FFFFFF"
      String rgbVal = lightCmd.substring(3, 9);
      int ledStripIndex = lightCmd[1] - '0' - 1;
      ledStripGenerate(rgbVal, ledStripIndex, 0, numOfLedPerStrip - 1);
      ledStripApply();
    }
    else if (lightCmd[0] == 'W')
    {
      // stream.println("W mode");
      //  RESET ROW MODE  CMD:"W1:12:34:FFFFFF"
      int ledStripIndex = lightCmd[1] - '0' - 1;
      const int endOfStartIndex = lightCmd.indexOf(':', 3);
      const int endOfStopIndex = lightCmd.indexOf(':', endOfStartIndex + 1);
      const String stringOfStartIndex = lightCmd.substring(3, endOfStartIndex);
      const String stringOfStopIndex = lightCmd.substring(endOfStartIndex + 1, endOfStopIndex);
      const String stringOfRgbVal = lightCmd.substring(endOfStopIndex + 1, endOfStopIndex + 7);
      const int startLedIndex = stringOfStartIndex.toInt();
      const int stopLedIndex = stringOfStopIndex.toInt();
      // stream.print("light color ");
      // stream.print(stringOfRgbVal);
      // stream.print(" at ");
      // stream.print(startLedIndex);
      // stream.print(" to ");
      // stream.println(stopLedIndex);
      ledStripGenerate(stringOfRgbVal, ledStripIndex, startLedIndex, stopLedIndex);
      ledStripApply();
    }
    else
    {
      stream.println("Invalid message!");
    }
  }
}

/**
 * @brief Get the Val Of Cfg object
 *
 * @param header header of query string
 * @param lightCommand string to query
 * @return int value
 */
int getValOfCfg(char header, String lightCommand)
{
  byte fIndex = lightCommand.indexOf(header, 3) + 1;
  byte lIndex = lightCommand.indexOf('.', fIndex);
  String val = lightCommand.substring(fIndex, lIndex);
  return val.toInt();
}

/**
 * @brief Generate light value on a row
 *
 * @param strRgb light value, ex: "ff053a"
 * @param ledStripIndex index of led strip (0 to 4)
 * @param startLedIndex index of start led (>=0)
 * @param stopLedIndex index of stop led
 * @return int
 */
int ledStripGenerate(String strRgb, int ledStripIndex, uint16_t startLedIndex, uint16_t stopLedIndex)
{
  String hexColor = "0x" + strRgb;
  long rgbHexFormat = strtol(&hexColor[0], NULL, 16);
  rgbColor.red = rgbHexFormat >> 16;
  rgbColor.green = rgbHexFormat >> 8 & 0xFF;
  rgbColor.blue = rgbHexFormat & 0xFF;
  for (unsigned int idx = startLedIndex; idx <= stopLedIndex; idx++)
  {
    leds[ledStripIndex][idx] = CRGB(rgbColor.red, rgbColor.green, rgbColor.blue);
  }
  return 1;
}

/**
 * @brief Apply led color
 *
 * @return int
 */
int ledStripApply()
{
  FastLED.show();
  return 1;
}

int statusLightGenerate(String strRgb)
{
  String hexColor = "0x" + strRgb;
  long rgbHexFormat = strtol(&hexColor[0], NULL, 16);
  rgbColor.red = rgbHexFormat >> 16;
  rgbColor.green = rgbHexFormat >> 8 & 0xFF;
  rgbColor.blue = rgbHexFormat & 0xFF;
  for (unsigned int idx = 0; idx <= 11; idx++)
  {
    statusLight[idx] = CRGB(rgbColor.red, rgbColor.green, rgbColor.blue);
  }
  return 1;
}

/**
 * @brief Write 2 byte to eeprom
 *
 * @param addr eeprom address
 * @param value value write to eeprom
 * @return int 1 if complete
 */
int eepromWriteUint16(int addr, uint16_t value)
{
  EEPROM.update(addr, value >> 8);
  EEPROM.update(addr + 1, value & 0xFF);
  return 1;
}

/**
 * @brief Read 2 bytes from eeprom
 *
 * @param addr eeprom address
 * @return uint16_t 2 bytes of data
 */
uint16_t eepromReadUint16(int addr)
{
  uint8_t byte1 = EEPROM.read(addr);
  uint8_t byte2 = EEPROM.read(addr + 1);
  return (byte1 << 8) + byte2;
}

/* EEPROM:  0+1: NUM_LED_PER_STRIP
            2: NUM_OF_COLUMN
            3: NUM_LED_PER_NODE
            4: brightnessOfLed 1
*/
