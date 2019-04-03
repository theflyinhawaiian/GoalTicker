#include <MD_MAX72xx.h>
#include <SPI.h>

#define DEBUG 1

#if DEBUG
#define PRINT(s, v)   { Serial.print(F(s)); Serial.print(v); }
#define PRINTX(s, v)  { Serial.print(F(s)); Serial.print(v, HEX); }
#define PRINTB(s, v)  { Serial.print(F(s)); Serial.print(v, BIN); }
#define PRINTC(s, v)  { Serial.print(F(s)); Serial.print((char)v); }
#define PRINTS(s)     { Serial.print(F(s)); }
#else
#define PRINT(s, v)
#define PRINTX(s, v)
#define PRINTB(s, v)
#define PRINTC(s, v)
#define PRINTS(s)
#endif

#define HARDWARE_TYPE MD_MAX72XX::FC16_HW
#define MAX_DEVICES 4
#define CLK_PIN   13
#define DATA_PIN  11
#define CS_PIN    10

MD_MAX72XX mx = MD_MAX72XX(HARDWARE_TYPE, CS_PIN, MAX_DEVICES);

#define DELAY  10
#define UNIT_DELAY      25
#define SCROLL_DELAY    (2 * UNIT_DELAY)
#define ONE_SECOND 1000

#define CHAR_SPACING     1
#define BUF_SIZE        75

uint32_t lastAnimationTime = 0;
uint32_t prevCycleTime = 0;
uint8_t  cycleCountdown = DELAY;

char *msgTab[] =
{
  "this is a drill", "testing, one, two, three", "Hello, World!"
};

bool scrollText(bool shouldRestart, char *pmsg)
{
  const uint8_t LOAD_CHAR = 0;
  const uint8_t PRINT_CHAR = 1;
  const uint8_t ADD_SPACE = 2;
  
  static char   curMessage[BUF_SIZE];
  static char   *p = curMessage;
  static uint8_t  state = LOAD_CHAR;
  static uint8_t  curLen, showLen;
  static uint8_t  characterBuffer[8];
  uint8_t         colData;

  if (shouldRestart)
  {
    PRINTS("\n--- Initializing ScrollText");
    resetMatrix();
    strcpy(curMessage, pmsg);
    state = LOAD_CHAR;
    p = curMessage;
    shouldRestart = false;
  }

  if (millis()-lastAnimationTime < SCROLL_DELAY)
    return(shouldRestart);

  mx.transform(MD_MAX72XX::TSL);
  lastAnimationTime = millis();

  switch (state)
  {
    case LOAD_CHAR:
      PRINTC("\nLoading ", *p);
      showLen = mx.getChar(*p++, sizeof(characterBuffer)/sizeof(characterBuffer[0]), characterBuffer);
      curLen = 0;
      state = PRINT_CHAR;

    // !! break intentionally omitted
    case PRINT_CHAR: // display the next part of the character
      colData = characterBuffer[curLen++];
      mx.setColumn(0, colData);
      if (curLen == showLen)
      {
        showLen = ((*p != '\0') ? CHAR_SPACING : mx.getColumnCount()-1);
        curLen = 0;
        state = ADD_SPACE;
      }
      break;

    case ADD_SPACE:
      mx.setColumn(0, 0);
      if (++curLen == showLen)
      {
        state = LOAD_CHAR;
        shouldRestart = (*p == '\0');
      }
      break;

    default:
      state = LOAD_CHAR;
  }

  return(shouldRestart);
}

void resetMatrix(void)
{
  mx.control(MD_MAX72XX::INTENSITY, MAX_INTENSITY/8);
  mx.control(MD_MAX72XX::UPDATE, MD_MAX72XX::ON);
  mx.clear();
  lastAnimationTime = 0;
}

void animateMatrix()
{
  static  uint8_t mesg = 0;
  static  boolean shouldRestart = true;
  boolean changeMessage = false;

  if (millis()-prevCycleTime >= ONE_SECOND)
  {
    prevCycleTime = millis();
    if (--cycleCountdown == 0)
    {
      cycleCountdown = DELAY;
      changeMessage = true;
    }
  }
  
  if (changeMessage)
  {
      mesg++;
      if (mesg >= sizeof(msgTab)/sizeof(msgTab[0]))
      {
        mesg = 0;
      }

    shouldRestart = true;
  }


  shouldRestart = scrollText(shouldRestart, msgTab[mesg]);
}

void setup()
{
  mx.begin();
  lastAnimationTime = millis();
  prevCycleTime = millis();
#if DEBUG
  Serial.begin(57600);
#endif
}

void loop()
{
  animateMatrix();
}
