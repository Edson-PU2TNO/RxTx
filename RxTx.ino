//******************************
//*Author: Edson Santos
//*Email: pu2tno@pilotbeacon.com
//******************************

#include <EEPROM.h>
#include <Adafruit_RGBLCDShield.h>
#include "morse.h"

/*  
 1 - dah
 0 - dit
*/

// Koch Char Sequence
const byte CHARS[][3] = {
  {' ',0b0,0}, // Space
  
 
  {'K',0b101,3},
  {'M',0b11,2},
  {'U',0b001,3},
  {'R',0b010,3},  
  {'E',0b0,1},
  {'S',0b000,3}, 
  {'N',0b10,2},
  {'A',0b01,2},
  {'P',0b0110,4},
  {'L',0b0100,4},
  {'W',0b011,3},   
  {'I',0b00,2},
  
  {'T',0b1,1},
  {'O',0b111,3},
  
  {'D',0b100,3},
  {'F',0b0010,4},
  
  {'J',0b0111,4},
  {'L',0b0100,4},
  {'Q',0b1101,4},
  {'X',0b1001,4},
  {'B',0b1000,4},
  {'Y',0b1011,4},
  
  
  {'H',0b0000,4},
  {'Z',0b1100,4},
  {'G',0b110,3},
 
  {'V',0b0001,4},
  {'C',0b1010,4}, //27 (idx=26) 
  
  {'1',0b01111,5},  {'2',0b00111,5},  {'3',0b00011,5},
  {'4',0b00001,5},  {'5',0b00000,5},  {'6',0b10000,5},
  {'7',0b11000,5},  {'8',0b11100,5},  {'9',0b11110,5}, // 36 (idx=35)
  {'0',0b11111,5}, // 37 (idx=36)
  
  {'/',0b10010,5}, {'?',0b001100,6}, {',',0b110011,6}, {'.',0b010101,6}, // 41 (idx=40)  

  {2,0b10001,5}, // BT
  {3,0b1000101,7}, // BK 
  {4,0b10110,5}, // KN
  {5,0b000101,6} // SK 44 (idx=43)

};

//byte PARIS[] = {12,4,20,2,3};


const char *const menuOptions[] = { "Words p/ minute","Words p/ min (F)", "Mode", "Word len min", "Word len max", "Buzz (KHz)", "Qty. of letters", "Qty. of numbers", "Pause" };
const char *const modeOptions[] = {"Letters","Numbers","Mixed","Special","ProSign"};
const char *const pause[] = {"No", "Yes"};
// savedData[] = {WPM, WPM(F),Mode, MinWord, MaxWord, Buzz, Qty Letters , Qty Numbers}
int savedData[] = {20, 13, 1, 5, 5, 700, 27, 10, 0};
int savedDataMax[] = {40, 40, 5, 15, 15, 800, 27, 10, 1};
int savedDataMin[]= {5, 5, 1, 1, 1, 550, 1, 1, 0};

byte icons[6][8] = { { 0x04,0x0e,0x15,0x04,0x04,0x04,0x04 }, // UP
                     { 0x04,0x04,0x04,0x04,0x15,0x0e,0x04 }, // DOWN
                     { 0x10,0x18,0x18,0x00,0x07,0x02,0x02 }, // BT
                     { 0x10,0x18,0x18,0x00,0x05,0x06,0x05 }, // BK
                     { 0x14,0x18,0x14,0x00,0x19,0x15,0x13 }, // KN
                     { 0x18,0x10,0x18,0x08,0x1d,0x06,0x05 } // SK
};

//! Enum of backlight colors.
enum Icons {UP=0x00, DOWN, BT, BK, KN, SK};
enum BackLightColor { RED=0x1, GREEN, YELLOW, BLUE, VIOLET, TEAL, WHITE };
enum Mode { Letters=0x01, Numbers, Mixed, Special, ProSign }; //Letter, Numbers, Mix, Special, ProSign


byte strPos = 0;

byte mult = 0;
byte colPos = 0;
byte clicked_buttons;
byte menu_idx = 0;

// Time unit for common method 
unsigned long timeUnit; // = (1000*1.2/savedData[0]);

// Time Unit for Farnworth method
unsigned long timeUnitf;

// 1st Line string
char firstLinestr[17]; 
//String firstLinestr, enteredStr;
// Entered string
char enteredStr[17];
// Flag to indicated that saved data is valid
boolean saved = true;

//! The LCD display object.
Adafruit_RGBLCDShield lcd = Adafruit_RGBLCDShield();

//! The current state.
void (*state)() = NULL;

// Iambic Key 
#define P_DOT    2   // Connects to the dot lever of the paddle
#define P_DASH   3   // Connects to the dash lever of the paddle

int speed = 60; //20 WPM
//    int speed = 120; // 10 WPM
unsigned long t0,t1,t2,t3,t4=0;
boolean flag, flag2 = false;
char* lookupString = ".EISH5..4..V...3..UF........?_2..ARL.......+.*...WP....@.J...1'.TNDB6.-=.%X/.....KC....;!Y%.....MGZ7....,Q......O.8:.....9..0..";
byte currentDecoderIndex = 0;
byte currentDashJump = 128;
char currentAssumedChar='\0';
byte curPos = 0;


//*****************************************
void setup() {
  int savedDataVal[(sizeof(savedData)/sizeof(savedData[0]))];
  for (int i=0; i<16; i++) firstLinestr[i] = ' ';
  
  randomSeed(analogRead(0));
  
#if debug
  Serial.begin(9600);  
#endif

  lcd.begin(16, 2);
  if (lcd.readButtons() & (BUTTON_UP | BUTTON_DOWN)) state = config_Menu;
  else state = splashScreen;

// Creating LCD chars
  lcd.createChar(UP, icons[UP]);
  lcd.createChar(DOWN, icons[DOWN]);  
  lcd.createChar(BT, icons[BT]); 
  lcd.createChar(BK, icons[BK]); 
  lcd.createChar(KN, icons[KN]); 
  lcd.createChar(SK, icons[SK]);
 
// Validating savedData
  EEPROM.get (0,savedDataVal);
  for (int i=0; i<(sizeof(savedDataVal)/sizeof(savedDataVal[0])); i++) {
    if ((savedDataVal[i]<savedDataMin[i]) | (savedDataVal[i]>savedDataMax[i])){
#if debug
      Serial.println(savedDataVal[i]);
#endif
      savedData[i] = -1;
      saved = false;
    }  
    if(saved) EEPROM.get (0,savedData);  
  }

// Iambic Key
pinMode(P_DOT,INPUT_PULLUP);
pinMode(P_DASH,INPUT_PULLUP );
}

//*****************************************
void loop() {
  state();
}


//*****************************************
void startMorse(){
    if ((savedData[2] == Letters) | (savedData[2] == Numbers) | (savedData[2] == Mixed)) 
    sendSequence((byte)random(savedData[3],savedData[4]+1));
    else sendSequence ((byte)1);
    colPos = 0;
    strPos = 0;
    state = IambicKey;
}


//*****************************************
void config_Menu() {
  lcd.setBacklight(YELLOW);
  lcd.clear();
  lcd.print (F("Config use: ")); lcd.write(UP); lcd.write(DOWN); lcd.write(0x7F); lcd.write(0x7E);
  lcd.setCursor(0,1);
  lcd.print(F("SELECT to return"));
  delay(5000);
  state = menuOption;
}

//*****************************************
void menuOption(){
  lcd.clear();
  int menuLen = ((sizeof(menuOptions)/sizeof(menuOptions[0]))-1);
  while(1) {
    read_button_clicks();
    if (clicked_buttons & BUTTON_LEFT) {
      menu_idx = (menu_idx > 0) ? menu_idx - 1 : menuLen;
      lcd.clear();
    }
    else if (clicked_buttons & BUTTON_RIGHT) {
      menu_idx = (menu_idx < menuLen) ? menu_idx + 1 : 0;
      lcd.clear();
    }
    else if (clicked_buttons & (BUTTON_UP|BUTTON_DOWN)) {
      configValue(menu_idx);  
      lcd.setBacklight(YELLOW);
    }
    else if (clicked_buttons & BUTTON_SELECT) {
    if (checkInc()) state = menuOption; 
    else state = splashScreen;
    return;
    }
     lcd.setCursor(0,0);
     lcd.print(menuOptions[menu_idx]);
     lcd.setCursor(0,1);
     lcd.print (savedData[menu_idx]);
     if ((menu_idx == 2) | (menu_idx ==8)) { 
        lcd.print (" - ");
        if (menu_idx == 2) lcd.print (modeOptions[savedData[2]-1]); 
        else lcd.print (pause[savedData[8]]);   
     }
  }
}


//*****************************************
void read_button_clicks() {
  static byte last_buttons = 0;
  
  byte buttons = lcd.readButtons();
  clicked_buttons = last_buttons & (~buttons);
  last_buttons = buttons;
}


//*****************************************
void splashScreen(){
  lcd.clear();
  lcd.setBacklight(TEAL);
  lcd.print(F("Morse Gen V0.3"));
  lcd.setCursor(0,1);
  lcd.print("Mode: "); lcd.print(modeOptions[savedData[2]-1]); 
  delay(2000);
  lcd.setBacklight(GREEN);
  lcd.clear();
  lcd.print(savedData[0]); lcd.print(F(" cWPM ")); lcd.print (savedData[1]); lcd.print(F(" eWPM"));
  lcd.setCursor(0,1);
  if (saved) state = startMorse;
  else state = config_Menu;
}
         

//*****************************************
void sendSequence(byte number){
  byte localnr;
  static byte linenr = 0;
  unsigned long t0,t1,t2 = 0;
  timeUnit = (1000*1.2/savedData[0]); // in mili seconds
  timeUnitf = ((((60*savedData[0]) - (37.2*savedData[1]))/(savedData[0]*savedData[1]))/19)*1000; // in mili seconds
/* if ((colPos+number+1) > 17){
    linenr++;
    while ((linenr == 2) & (savedData[8])) {
      read_button_clicks();
      if (clicked_buttons & BUTTON_SELECT) {
         linenr=0;
         delay (1500);
         break;	
      }   
    }
    t0 = millis();
    lcd.clear();
    lcd.setCursor(0,0);
    for (int i=0; i<17; i++){
	     if (firstLinestr[i] < 10) lcd.write((byte)firstLinestr[i]);
	     else lcd.print((char)firstLinestr[i]);
	     firstLinestr[i]=' ';
    }
    lcd.setCursor(0,1);   
    colPos = 0;
    t2 = millis() - t0;
  }
*/  

      
  lcd.clear();
  lcd.setCursor(0,0);
  for (int i=0; i<number; i++) {
    switch (savedData[2]) {
      case Letters:
          localnr = random(1,savedData[6]+1);
          break;
      case Numbers:
          localnr = random(27,savedData[7]+27);
          break;
      case Mixed:
         // localnr = random(1,45);
          localnr = random(1,38);
          break;
      case Special:
          localnr = random(37,41);	
          break;
      case ProSign:
          localnr = random(41,45);
          break;
      default: //Letters
          localnr = random(1,savedData[6]+1);
          break;
    }
    playLetter(localnr);        
// display letter and consider time spent to process it
    t0 = millis(); 
    printLetter(localnr);
    t1 = millis() - t0;      
// delay 3 Time Units (inter char space) minus time spent to display the letter. Not applied for the last char in the word
    if (i<number) delay (charSpace - t1);
  }
  t0 = millis(); 
  printLetter(0);
  t1 = millis() - t0;
  lcd.setCursor(0,1);
  if (t2 > 0) {
// delay 7 Time Units (word space) minus time spent to display the letter,
// minus time spent to clear lcd and repeat line 2
    //delay (wordSpace - t1 - t2); *************
    t2 = 0;
  }
// delay 7 Time Units (word space) minus time spent to display the letter
  //else delay (wordSpace - t1); **************
}

//*****************************************
void playLetter(byte idx) {  
  byte sizeLetter = CHARS[idx][2];
  byte letter = CHARS[idx][1] << (8-sizeLetter);
  for (byte i=0; i<sizeLetter; i++){
    if (letter & mask) mult = 3;
    else mult = 1;
    tone (8,savedData[5], (mult*timeUnit));
    if (i<sizeLetter) delay ((mult+1)*timeUnit);
    else delay (mult*timeUnit);
    letter = letter << 1;
    } 
}

//*****************************************
void printLetter(byte idx){
 lcd.print((char)CHARS[idx][0]);
 // if (idx > 40) lcd.write((byte)CHARS[idx][0]); 
 // else lcd.print((char)CHARS[idx][0]);
  firstLinestr[colPos] = (char)CHARS[idx][0];
  colPos++;
}

//*****************************************
void configValue(byte idx) {
  lcd.setBacklight(VIOLET);
  while (1){
    read_button_clicks();
    if (clicked_buttons & BUTTON_UP) { 
      if (savedData[idx] == savedDataMax[idx]) savedData[idx] = savedDataMax[idx];
      else savedData[idx]++;
      lcd.setCursor(0,1);
      lcd.print(savedData[idx]); 
      if ((idx==2) | (idx==8)) {
         lcd.print(" - ");
         if (idx==2) lcd.print(modeOptions[savedData[idx]-1]);
         else lcd.print (pause[savedData[8]]); 
         lcd.print("       ");	
      }
      lcd.setCursor(0,1); 
    }
    else if (clicked_buttons & BUTTON_DOWN)
     {
      if (savedData[idx] == savedDataMin[idx]) savedData[idx] = savedDataMin[idx];
      else savedData[idx]--;
      if (savedData[idx] <= 9) {
        lcd.setCursor (0,1);
        lcd.print("  ");
      }
      lcd.setCursor (0,1);
      lcd.print(savedData[idx]); 
      if ((idx==2) | (idx==8)) {
        lcd.print(" - ");
        if (idx==2) lcd.print(modeOptions[savedData[idx]-1]);
        else lcd.print (pause[savedData[8]]); 
        lcd.print("       "); 
      }
      lcd.setCursor(0,1);  
     }
    else if (clicked_buttons & BUTTON_SELECT){
      if (checkInc()) {
         lcd.clear();    
         lcd.setBacklight(VIOLET);
         break;
      }   	
      lcd.clear();
      lcd.setCursor(0,1);
      EEPROM.put(0,savedData);  
      saved = true; 
      return;
    }  
  }
}

//*****************************************
boolean checkInc() {
if ((savedData[0] < savedData[1])|(savedData[3] > savedData[4])) {
   lcd.setBacklight(RED);
   lcd.clear();
   lcd.print(F("Please check"));
   lcd.setCursor(0,1);
   lcd.print(F("Inconsistency !"));
   delay (5000);
   return true;
   } else return false;
}

// Iambic Key

void IambicKey()
{
 
  if ((lcd.readButtons() & BUTTON_LEFT) || (!digitalRead(P_DOT))) // If the dot lever is presssed..     
  {
    keyAndBeep(speed);            // ... send a dot at the given speed
    currentAssumedChar = lookup('.');
    delay(speed);
    t2 = millis(); 
    flag = true;
    flag2 = false;
  }
  

  if ((lcd.readButtons() & BUTTON_RIGHT) || (!digitalRead(P_DASH))) // If the dash lever is pressed...      
  {
  
    keyAndBeep(speed*3);         // ... send a dash at the given speed
    currentAssumedChar = lookup('-');
    delay(speed);
    t2 = millis();
    flag = true;
    flag2 = false;
  
  }

  if ((millis()-t2 > speed) & flag)
  { 
    t0 = millis();
    lcd.print(currentAssumedChar);
    enteredStr[strPos] = currentAssumedChar;
    strPos++;
/*    curPos++;
    if (curPos == 16)
    {  
      lcd.setCursor(0,1);
      curPos = 0;      
    }  
*/    lookup('\0');
    flag = false;
    t1 = millis()-t0;
    delay (2*speed-t1);
    t3 = millis();
    flag2=true;
  }


  if (((millis()-t3) > (25*speed)) & flag2)
  {
/*    lcd.print(" ");
    curPos++;
    if (curPos == 16)
    {  
      lcd.setCursor(0,1);
      curPos = 0;      
    }
 */  flag2 = false;   
   t3 = 0;

  boolean ts = false;
  for(int i=0;i<17;i++){
      Serial.print("i= ");
      Serial.println(i);
      if(enteredStr[i]==firstLinestr[i]){
        ts = true;         
      }else{
        break;
      }
  }
Serial.println(ts);
  if (ts){
       Serial.println("OK");
       lcd.setBacklight(GREEN);
  }
  else {     
       Serial.println("NOK");
       lcd.setBacklight(RED);
  }
   state = startMorse;      
  }
  
}
// Key the transmitter and sound a beep
void keyAndBeep(int speed)
{
  tone (8,580,speed);
  delay (speed);
}

char lookup(char currentMark)
{
    currentDashJump = floor(currentDashJump / 2.0f);
    if (currentMark == '.')
    {                
        currentDecoderIndex++;
    }
    else if (currentMark == '-')
    {
        currentDecoderIndex += currentDashJump;
    }
    else if (currentMark == '\0')
    {
        currentDecoderIndex = 0;
        currentDashJump = 128;
        return '\0';
    }
    //Serial.println(currentDecoderIndex);
    if (currentDecoderIndex == 73) return 3; // Display BK prosign
    else if (currentDecoderIndex == 90) return 4; // Display KN prosign
    else if (currentDecoderIndex == 14) return 5; // Display SK prosign    
    return lookupString[currentDecoderIndex];
}
