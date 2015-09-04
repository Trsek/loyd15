#include <stdint.h>
#include <EEPROM.h>
#include "TouchScreen.h"
#include <Adafruit_GFX.h>    // Core graphics library
#include <Adafruit_TFTLCD.h> // Hardware-specific library

#define LCD_CS A3 // Chip Select goes to Analog 3
#define LCD_CD A2 // Command/Data goes to Analog 2
#define LCD_WR A1 // LCD Write goes to Analog 1
#define LCD_RD A0 // LCD Read goes to Analog 0
#define LCD_RESET A4 // Can alternately just connect to Arduino's reset pin

#define YP A1  // must be an analog pin, use "An" notation!
#define XM A2  // must be an analog pin, use "An" notation!
#define YM 7   // can be a digital pin
#define XP 6   // can be a digital pin

#define TS_MINX 160
#define TS_MINY 140
#define TS_MAXX 880
#define TS_MAXY 940

// Assign human-readable names to some common 16-bit color values:
#define BLACK   0x0000
#define	GRAY    0x0101
#define	BLUE    0x001F
#define	RED     0xF800
#define	GREEN   0x07E0
#define CYAN    0x07FF
#define MAGENTA 0xF81F
#define YELLOW  0xFFE0
#define WHITE   0xFFFF

#define MIN_TOUCH     10
#define WIDTH         60    // width of button
#define HEIGHT        60    // height of button
#define _EEPROM_READ   0
#define _EEPROM_WRITE  1

Adafruit_TFTLCD tft(LCD_CS, LCD_CD, LCD_WR, LCD_RD, LCD_RESET);
TouchScreen ts = TouchScreen(XP, YP, XM, YM, 300);

Adafruit_GFX_Button button[4][4];
Adafruit_GFX_Button button_new;
Adafruit_GFX_Button button_solve;
Adafruit_GFX_Button button_save;
Adafruit_GFX_Button button_load;

byte loyd[4][4];
byte moves;
char text[3];
// -----------------------------------------------------------------------------


// save/load play square to/from EEPROM
void EEPROM_modul(byte mode)
{
  if( mode == _EEPROM_WRITE )
  {
      for(int y=0; y<4; y++)
        for(int x=0; x<4;  x++)
           EEPROM.write(y*4+x, loyd[x][y]);
           
      EEPROM.write(sizeof(loyd), moves);
  }
  else
  {
      for(int y=0; y<4; y++)
        for(int x=0; x<4;  x++)
           loyd[x][y] = EEPROM.read(y*4+x);
           
      moves = EEPROM.read(sizeof(loyd));
  }
}
// -----------------------------------------------------------------------------

char* ConvertToChar(byte number)
{
  if( number >= 10 )
  {
    text[0] = '0' + number / 10;
    text[1] = '0' + number % 10;
  }
  else
  {
    text[0] = '0' + number % 10;
    text[1] = '\0';
  }

  text[2] = '\0';
  return text;
}
// -----------------------------------------------------------------------------

void redrawSquare(byte x, byte y, byte xv, byte yv)
{
  // in square?
  if((x != 0) || (y != 0) || (xv != 0) || (yv != 0))
  {
     button[x][y].initButton(&tft, WIDTH/2 + x*WIDTH, HEIGHT/2 + y*HEIGHT, WIDTH,HEIGHT, 1,GRAY, GRAY, "", 3);
     button[xv][yv].initButton(&tft, WIDTH/2 + xv*WIDTH, HEIGHT/2 + yv*HEIGHT, WIDTH,HEIGHT, 1,BLUE, WHITE, ConvertToChar(loyd[xv][yv]), 3);
     // show it
     button[x][y].drawButton(false);
     button[xv][yv].drawButton(false);
    return;
  }
  
  for(int y=0; y<4; y++)
    for(int x=0; x<4; x++)
    {
      if( loyd[x][y] == 0 )
          button[x][y].initButton(&tft, WIDTH/2 + x*WIDTH, HEIGHT/2 + y*HEIGHT, WIDTH,HEIGHT, 1,GRAY, GRAY, "", 3);
      else
          button[x][y].initButton(&tft, WIDTH/2 + x*WIDTH, HEIGHT/2 + y*HEIGHT, WIDTH,HEIGHT, 1,BLUE, WHITE, ConvertToChar(loyd[x][y]), 3);
       
      // ok, show
      button[x][y].drawButton(false);
    }
}
// -----------------------------------------------------------------------------

// check if is possible solve it
bool isPossibleSolve()
{
  byte sum;
  byte test_array[16];

  for(int y=0; y<4; y++)
    for(int x=0; x<4; x++)
    {
      test_array[y*4+x] = loyd[x][y];
      // add row of empty
      if( loyd[x][y] == 0 )
          sum = (3-y);
    }

  // count smaller that I
  for(int i=0; i<16; i++)
  {
    for(int n=i+1; n<16; n++)
      if((test_array[n] < test_array[i])
      && (test_array[n] != 0 ))
         sum++;
  }

  // is even - then it is no possible
  return (sum % 2)? false: true;
}
// -----------------------------------------------------------------------------

// check if win
bool isWin()
{
  byte i=0;
  
  for(int y=0; y<4; y++)
    for(int x=0; x<4; x++)
    {
      if( ++i != loyd[x][y] )
         return (i==16)? true: false;
    }

  // all is ok
  return true;
}
// -----------------------------------------------------------------------------

void changeButtons(byte x, byte y, byte xv, byte yv)
{
  byte pom;
  
  pom = loyd[x][y];
  loyd[x][y] = loyd[xv][yv];
  loyd[xv][yv] = pom;
}
// -----------------------------------------------------------------------------

void mixLoyd15(void)
{
  byte x,y;
  
  // znulujeme  
  for(int y=0; y<4; y++)
    for(int x=0; x<4; x++)
    {
      loyd[x][y] = 0;
    }
  
  for(byte i=1; i<=15; i++)
  {
     x = random(0,4);
     y = random(0,4);
     while( loyd[x][y] != 0 )
     {
       x++;
       if( x>3 )
       {
         y++;
         x=0;
       }
       
       if( y>3 )
       {
         x=0;
         y=0;
       }
       
     } // while
     
     loyd[x][y] = i;
  }
  
  // when is not possible solve it then change 2 squares anf try again
  while( !isPossibleSolve())
  {
    x = random(0,3);
    y = random(0,4);
    changeButtons(x,y,x+1,y);
  }
}
// -----------------------------------------------------------------------------

bool freeButton(byte x, byte y, byte *xv, byte *yv)
{
  // is in left?
  if((x>0) && (loyd[x-1][y] == 0))
  {
     *xv = x-1;
     *yv = y;
     return true;
  }

  // is in right?
  if((x<3) && (loyd[x+1][y] == 0))
  {
     *xv = x+1;
     *yv = y;
     return true;
  }

  // is in up?
  if((y>0) && (loyd[x][y-1] == 0))
  {
     *xv = x;
     *yv = y-1;
     return true;
  }

  // is in bellow?
  if((y<3) && (loyd[x][y+1] == 0))
  {
     *xv = x;
     *yv = y+1;
     return true;
  }
  
  return false;
}
// -----------------------------------------------------------------------------

void showWin()
{
  TSPoint p;

  for(int i=0; i<10; i++)
  {
    tft.fillRect(20, 100, 22*i, 8*i, BLACK);
    delay(30);
  }
  
  tft.setTextColor(WHITE);
  tft.setTextSize(3);
  tft.setCursor(25, 108);
  tft.print(F(" Solve it!"));
  
  tft.setTextSize(2);
  tft.setCursor(30, 145);
  tft.print(F("need moves :"));
  tft.print(moves);

  do {
    p = ts.getPoint();
    pinMode(XM, OUTPUT);
    pinMode(YP, OUTPUT);
  }
  while( p.z < MIN_TOUCH );
  
  tft.fillRect(20, 100, 200, 80, BLACK);
}
// -----------------------------------------------------------------------------

void setup(void) 
{
  Serial.begin(9600);
  Serial.println(F("Loyd 15. Software by Zdeno Sekerak (c) 2015."));
  randomSeed(analogRead(0));

  tft.reset();
  tft.begin(tft.readID());

  mixLoyd15();
  moves = 0;
  tft.fillScreen(BLACK);
  redrawSquare(0,0,0,0);
  
  // button nova
  button_new.initButton  (&tft,  62,260, 105,32, 1,GREEN, MAGENTA, "New", 2);
  button_solve.initButton(&tft,  62,294, 105,32, 1,GREEN, MAGENTA, "Solve", 2);
  button_save.initButton (&tft, 177,260, 105,32, 1,GREEN, MAGENTA, "Save", 2);
  button_load.initButton (&tft, 177,294, 105,32, 1,GREEN, MAGENTA, "Load", 2);

  button_new.drawButton(false);
  button_solve.drawButton(false);
  button_save.drawButton(false);
  button_load.drawButton(false);

  tft.setTextColor(BLUE);
  tft.setTextSize(1);
  tft.setCursor(20, 311);
  tft.print(F("Software by Zdeno Sekerak (c) 2015"));  
}
// -----------------------------------------------------------------------------

void solveFake()
{
  int x, y;
  byte xv,yv;
  
  for(int i=75; i>1; i--)
  {
      // find space
     x = random(0,4);
     y = random(0,4);
     
     while( !freeButton(x, y, &xv, &yv))
     {
       x++;
       if( x>3 )
       {
         y++;
         x=0;
       }
       
       if( y>3 )
       {
         x=0;
         y=0;
       }
       
     } // while
     
     changeButtons(x, y, xv, yv);
     redrawSquare(x, y, xv, yv);
     delay(3*i);
  }
  
  // solve it, horray
  for(int i=0; i<15; i++)
      loyd[i%4][i/4] = i+1;
      
  loyd[3][3] = 0;
}
// -----------------------------------------------------------------------------

void loop(void) 
{
  TSPoint p = ts.getPoint();
  byte xv,yv;

  if( p.z > MIN_TOUCH )
  {
    pinMode(XM, OUTPUT);
    pinMode(YP, OUTPUT);
    
    p.x = map(p.x, TS_MINX, TS_MAXX, tft.width(), 0);
    p.y = map(p.y, TS_MINY, TS_MAXY, tft.height(), 0); 

    if( button_new.contains(p.x, p.y))
    {
       button_new.drawButton(true);
       mixLoyd15();
       moves = 0;
       redrawSquare(0,0,0,0);
       button_new.drawButton(false);
       return;
    }

    if( button_solve.contains(p.x, p.y))
    {
       button_solve.drawButton(true);
       delay(100);
       button_solve.drawButton(false);
       solveFake();
       redrawSquare(0,0,0,0);
       return;
    }
    
    if( button_save.contains(p.x, p.y))
    {
       button_save.drawButton(true);
       EEPROM_modul(_EEPROM_WRITE);
       delay(100);
       button_save.drawButton(false);
       return;
    }

    if( button_load.contains(p.x, p.y))
    {
       button_load.drawButton(true);
       EEPROM_modul(_EEPROM_READ);
       redrawSquare(0,0,0,0);
       delay(100);
       button_load.drawButton(false);
       return;
    }
    
    for(int y=0; y<4; y++)
      for(int x=0; x<4; x++)
      {
        if( button[x][y].contains(p.x, p.y))
        {
          button[x][y].drawButton(true);
          delay(300);
          // around is a free button
          if(freeButton(x, y, &xv, &yv))
          {
            moves++;
            changeButtons(x, y, xv, yv);
            redrawSquare(x, y, xv, yv);
            if( isWin())
            {
                showWin();
                redrawSquare(0,0,0,0);
            }
          }
          else
              button[x][y].drawButton(false);
        } // if button
      } // for  
  } // if (touch)
}
// -----------------------------------------------------------------------------
