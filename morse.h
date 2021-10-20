void startMorse();
void config_Menu();
void menuOption();
void read_button_clicks();
void splashScreen();
void sendSequence(byte number);
void playLetter(byte idx);
void printLetter(byte idx);
void configValue(byte idx);
void repeatLast();

#define debug true
#define mask 0b10000000

#define charSpace (3)*timeUnitf
#define wordSpace (7)*timeUnitf
