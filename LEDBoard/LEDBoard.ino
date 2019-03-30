// Use the MD_MAX72XX library to Display a Daft Punk LED Helmet
//
// If RUN_DEMO is set to zero the display cycles changes triggered by a switch on
// the MODE_SWITCH pin. This can be substituted for any trigger as implemented
// by the helmet wearer.
// If RUN_DEMO is set to 1 the sketch will cycle each element of the display every
// DEMO_DELAY seconds, without the need for a switch.
//
// Uses the MD_UISwitch library found at https://github.com/MajicDesigns/MD_UISwitch

#define RUN_DEMO  1

#include <MD_MAX72xx.h>
#include <SPI.h>

#if RUN_DEMO
#define DEMO_DELAY  15  // time to show each demo element in seconds
#else
#include <MD_UISwitch.h>
#endif

#define DEBUG 1   // Enable or disable (default) debugging output

#if DEBUG
#define PRINT(s, v)   { Serial.print(F(s)); Serial.print(v); }        // Print a string followed by a value (decimal)
#define PRINTX(s, v)  { Serial.print(F(s)); Serial.print(v, HEX); }   // Print a string followed by a value (hex)
#define PRINTB(s, v)  { Serial.print(F(s)); Serial.print(v, BIN); }   // Print a string followed by a value (binary)
#define PRINTC(s, v)  { Serial.print(F(s)); Serial.print((char)v); }  // Print a string followed by a value (char)
#define PRINTS(s)     { Serial.print(F(s)); }                         // Print a string
#else
#define PRINT(s, v)   // Print a string followed by a value (decimal)
#define PRINTX(s, v)  // Print a string followed by a value (hex)
#define PRINTB(s, v)  // Print a string followed by a value (binary)
#define PRINTC(s, v)  // Print a string followed by a value (char)
#define PRINTS(s)     // Print a string
#endif

// --------------------
// MD_MAX72xx hardware definitions and object
// Define the number of devices we have in the chain and the hardware interface
// NOTE: These pin numbers will probably not work with your hardware and may
// need to be adapted
//
#define HARDWARE_TYPE MD_MAX72XX::FC16_HW
#define MAX_DEVICES 4
#define CLK_PIN   13  // or SCK
#define DATA_PIN  11  // or MOSI
#define CS_PIN    10  // or SS

MD_MAX72XX mx = MD_MAX72XX(HARDWARE_TYPE, CS_PIN, MAX_DEVICES);                      // SPI hardware interface
//MD_MAX72XX mx = MD_MAX72XX(HARDWARE_TYPE, DATA_PIN, CLK_PIN, CS_PIN, MAX_DEVICES); // Arbitrary pins

#if !RUN_DEMO
// --------------------
// Mode keyswitch parameters and object
//
#define MODE_SWITCH 9 // Digital Pin

MD_UISwitch_Digital  ks = MD_UISwitch_Digital(MODE_SWITCH, LOW);
#endif

// --------------------
// Constant parameters
//
// Various delays in milliseconds
#define UNIT_DELAY      25
#define SCROLL_DELAY    (4 * UNIT_DELAY)

#define CHAR_SPACING     1  // pixels between characters
#define BUF_SIZE        75  // character buffer size

// ========== General Variables ===========
//
uint32_t prevTimeAnim = 0;    // Used for remembering the millis() value in animations
#if RUN_DEMO
uint32_t prevTimeDemo = 0;      //  Used for remembering the millis() time in demo loop
uint8_t  timeDemo = DEMO_DELAY; // number of seconds left in this demo loop
#endif

// ========== Text routines ===========
//
// Text Message Table
// To change messages simply reorder, add to, or delete from, this table
char *msgTab[] =
{
  "Hello, World!"
};

bool scrollText(bool bInit, char *pmsg)
// Callback function for data that is required for scrolling into the display
{
  static char   curMessage[BUF_SIZE];
  static char   *p = curMessage;
  static uint8_t  state = 0;
  static uint8_t  curLen, showLen;
  static uint8_t  cBuf[8];
  uint8_t         colData;

  // are we initializing?
  if (bInit)
  {
    PRINTS("\n--- Initializing ScrollText");
    resetMatrix();
    strcpy(curMessage, pmsg);
    state = 0;
    p = curMessage;
    bInit = false;
  }

  // Is it time to scroll the text?
  if (millis()-prevTimeAnim < SCROLL_DELAY)
    return(bInit);

  // scroll the display
  mx.transform(MD_MAX72XX::TSL);  // scroll along
  prevTimeAnim = millis();        // starting point for next time

  // now run the finite state machine to control what we do
  PRINT("\nScroll FSM S:", state);
  switch (state)
  {
    case 0: // Load the next character from the font table
      PRINTC("\nLoading ", *p);
      showLen = mx.getChar(*p++, sizeof(cBuf)/sizeof(cBuf[0]), cBuf);
      curLen = 0;
      state = 1;

    // !! deliberately fall through to next state to start displaying

    case 1: // display the next part of the character
      colData = cBuf[curLen++];
      mx.setColumn(0, colData);
      if (curLen == showLen)
      {
        showLen = ((*p != '\0') ? CHAR_SPACING : mx.getColumnCount()-1);
        curLen = 0;
        state = 2;
      }
      break;

    case 2: // display inter-character spacing (blank column) or scroll off the display
      mx.setColumn(0, 0);
      if (++curLen == showLen)
      {
        state = 0;
        bInit = (*p == '\0');
      }
      break;

    default:
      state = 0;
  }

  return(bInit);
}

// ========== Control routines ===========
//
void resetMatrix(void)
{
  mx.control(MD_MAX72XX::INTENSITY, MAX_INTENSITY/8);
  mx.control(MD_MAX72XX::UPDATE, MD_MAX72XX::ON);
  mx.clear();
  prevTimeAnim = 0;
}

void runMatrixAnimation(void)
// Schedule the animations, switching to the next one when the
// the mode switch is pressed.
{
  static  uint8_t state = 0;
  static  uint8_t mesg = 0;
  static  boolean bRestart = true;
	static boolean	bInMessages = false;
  boolean changeState = false;

#if RUN_DEMO
  // check if one second has passed and then count down the demo timer. Once this
  // gets to zero, change the state.
  if (millis()-prevTimeDemo >= 1000)
  {
    prevTimeDemo = millis();
    if (--timeDemo == 0)
    {
      timeDemo = DEMO_DELAY;
      changeState = true;
    }
  }
#else
  // check if the switch is pressed and handle that first
  changeState = (ks.read() == MD_UISwitch::KEY_PRESS);
#endif
  if (changeState)
  {
    if (bInMessages) // the message display state
    {
      mesg++;
      if (mesg >= sizeof(msgTab)/sizeof(msgTab[0]))
      {
        mesg = 0;
    	bInMessages = false;
        state++;
      }
    }
    else
      state++;

    bRestart = true;
  };

  // now do whatever we do in the current state
  switch(state)
  {
    case  0: bInMessages = true; bRestart = scrollText(bRestart, msgTab[mesg]); break;

    default: state = 0;
  }
}

void setup()
{
  mx.begin();
  prevTimeAnim = millis();
#if RUN_DEMO
  prevTimeDemo = millis();
#else
  ks.begin();
#endif
#if DEBUG
  Serial.begin(57600);
#endif
  PRINTS("\n[MD_MAX72XX DaftPunk]");
}

void loop()
{
  runMatrixAnimation();
  // other code to run the helmet goes here
}
