/*
  Loyd15 for Arduino with touch LCD
  Software by Zdeno Sekerak (c) 2015. www.trsek.com/en/curriculum

  This software is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
*/

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

#define TS_MINX    160
#define TS_MINY    140
#define TS_MAXX    880
#define TS_MAXY    940

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

#define MIN_TOUCH       10
#define WIDTH           60    // width of button -> min( tft.width()/4, tft.height()/4)
#define HEIGHT          60    // height of button
#define _EEPROM_READ     0
#define _EEPROM_WRITE    1

#define MAX_MOVES       21    // for recursion (max=31), but 22 enought for all type
#define BLINK_TIME    1500    // blink solve button when solution takes a long time
#define SOLVE_DELAY    200    // time how long show press button in automatic 
#define SOLVE_MAX_TIME  13    // time = SOLVE_MAX_TIME * BLINK_TIME
#define BUTTON_DELAY   300    // time how long show press button
#define SOLVE_APPROX    20    // number must be more like 16 and mean special moves
//#define DEBUG               // when you want see debug info in serial monitor

typedef enum T_enum_move {
  e_move_start,
  e_move_left,
  e_move_right,
  e_move_up,
  e_move_down,
};

typedef struct T_find_button {
  Adafruit_GFX_Button *button;
  unsigned long time;
  byte count;
};

typedef struct T_moves {
  byte count;
  byte solve;
  byte moves[MAX_MOVES];
};

// structure solve - strong packet because it use in recursion
typedef struct T_loyd {
  byte square[2][4];
  byte hole_x:4, 
       hole_y:4;
  byte last_move:3, 
       count: 5;
};

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
// global variables for solve
T_moves win_moves;
T_moves akt_moves;
byte level;
byte solve_number;
byte border_y, border_x;
T_find_button fbutton;
byte solve_process[] = {SOLVE_APPROX + 1, 1, SOLVE_APPROX + 2, 2, SOLVE_APPROX + 3, 3, SOLVE_APPROX + 4, 4, 5, 6, 7, 8, 9, 13, 10, 11, 12, 14, 15 };
// -----------------------------------------------------------------------------

void solveRecursion(T_loyd loyd_solve, T_enum_move act_move);
T_enum_move oppositeMove(T_enum_move act_move);
// -----------------------------------------------------------------------------

#define GET_SOLVE(x,y)     ((loyd_solve.square[(x)/2][(y)] >> (((x)%2)? 0:4)) & 0x0F)
#define GETP_SOLVE(x,y)    ((loyd_solve->square[(x)/2][(y)] >> (((x)%2)? 0:4)) & 0x0F)
#define PUT_SOLVE(x,y,c)   (loyd_solve.square[(x)/2][(y)] = ((x)%2)? ((loyd_solve.square[(x)/2][(y)] & 0xF0) + (c)) : ((loyd_solve.square[(x)/(2)][(y)] & 0x0F) + ((c) << 4)) )


T_enum_move oppositeMove(T_enum_move act_move)
{
  switch( act_move )
  {
   case e_move_left  :  return e_move_right;
   case e_move_right :  return e_move_left;
   case e_move_up    :  return e_move_down;
   case e_move_down  :  return e_move_up;
   case e_move_start :
   default:             return e_move_up;
  }
}
// -----------------------------------------------------------------------------

bool isSolve(struct T_loyd *loyd_solve)
{
  byte number, y;
  
  // special for 1-4
  if( solve_number >= SOLVE_APPROX )
  {
    number = solve_number - SOLVE_APPROX;
    y = (number-1) /4;

    for(byte i=0; i<(number-1); i++)
    {
      if( GETP_SOLVE(i%4, i/4) != (i+1))
        return false;
    }
    for(byte i=0; i<4; i++)
    {
      if(( GETP_SOLVE(i, y) == number)
      || ( GETP_SOLVE(i, y+1) == number))
        return true;
    }
    return false;
  }

	// special for 13
  if( solve_number == 13 )
  {
     for(byte i=0; i<9; i++)
     {
       if( GETP_SOLVE(i%4, i/4) != (i+1))
          return false;
     }
     return (GETP_SOLVE(0,3) == 13);
  }
    
  for(byte i=0; i<solve_number; i++)
  {
    if( GETP_SOLVE(i%4, i/4) != (i+1))
      return false;
  }
  return true;
}
// -----------------------------------------------------------------------------

// most important function - do not touch if you don't know
void solveRecursion(T_loyd loyd_solve, T_enum_move act_move)
{
   // something already has and a long would it take, I resigned for ideal solution
   if(( win_moves.solve ) 
   && ( fbutton.count > SOLVE_MAX_TIME ))
   {
      return;
   }
  
   // add this move
   switch ( act_move )
   {
   case e_move_left  :
         PUT_SOLVE(loyd_solve.hole_x, loyd_solve.hole_y, GET_SOLVE(loyd_solve.hole_x-1, loyd_solve.hole_y));
         loyd_solve.hole_x--;
         break;
   case e_move_right :
         PUT_SOLVE(loyd_solve.hole_x, loyd_solve.hole_y, GET_SOLVE(loyd_solve.hole_x+1, loyd_solve.hole_y));
         loyd_solve.hole_x++;
         break;
   case e_move_up    :
         PUT_SOLVE(loyd_solve.hole_x, loyd_solve.hole_y, GET_SOLVE(loyd_solve.hole_x, loyd_solve.hole_y-1));
         loyd_solve.hole_y--;
         break;
   case e_move_down  :
         PUT_SOLVE(loyd_solve.hole_x, loyd_solve.hole_y, GET_SOLVE(loyd_solve.hole_x, loyd_solve.hole_y+1));
         loyd_solve.hole_y++;
         break;
   }

   PUT_SOLVE(loyd_solve.hole_x, loyd_solve.hole_y, 0);
   loyd_solve.last_move = act_move;
   akt_moves.moves[ loyd_solve.count++ ] = loyd_solve.hole_y*4 + loyd_solve.hole_x;

   // great - it is solve (one of the many)
   if( isSolve(&loyd_solve))
   {
      akt_moves.count = loyd_solve.count;
      win_moves = akt_moves;
#ifdef DEBUG
      Serial.print("  moves = ");
      Serial.println(win_moves.count);
#endif
      return;
   }
   
   // actually I have a shorter solutions, don't continue
   if(( loyd_solve.count + 1 ) >= win_moves.count )
   {
      return;
   }

   // button blink
   if(( fbutton.time + BLINK_TIME ) <= millis())
   {
      fbutton.time = millis();
      fbutton.count++;
      fbutton.button->press( !fbutton.button->isPressed());
      fbutton.button->drawButton( fbutton.button->isPressed());
   }

   // try all direction's
   if((oppositeMove( act_move ) != e_move_left )  && ( loyd_solve.hole_x != border_x )) solveRecursion(loyd_solve, e_move_left);
   if((oppositeMove( act_move ) != e_move_right ) && ( loyd_solve.hole_x != 3 ))        solveRecursion(loyd_solve, e_move_right);
   if((oppositeMove( act_move ) != e_move_up )    && ( loyd_solve.hole_y != border_y )) solveRecursion(loyd_solve, e_move_up);
   if((oppositeMove( act_move ) != e_move_down )  && ( loyd_solve.hole_y != 3 ))        solveRecursion(loyd_solve, e_move_down);
}
// -----------------------------------------------------------------------------

// initialize all structure for start solve
struct T_loyd initSolve(byte number)
{
   struct T_loyd loyd_solve;
   
   if( number >= SOLVE_APPROX )
     number -= SOLVE_APPROX;

   for(int y=0; y<4; y++)
    for(int x=0; x<4; x++)
    {
      PUT_SOLVE(x,y, loyd[x][y]);
      if(loyd[x][y] == 0)
      {
        loyd_solve.hole_x = x;
        loyd_solve.hole_y = y;
      }
      if(loyd[x][y] == number)
      {
        fbutton.button = &button[x][y];
      }
    }

    fbutton.count = 0;
    fbutton.time = millis();
    
    loyd_solve.last_move = e_move_start;
    loyd_solve.count = 0;
    
    win_moves.count = MAX_MOVES;
    win_moves.solve = false;
    akt_moves.count = 0;
    akt_moves.solve = true;
    return loyd_solve;
}
// -----------------------------------------------------------------------------

// show win moves
void viewSolve(struct T_moves* win_moves)
{
  byte x, y;
  byte xv, yv;

  fbutton.button->drawButton(false);

  for(int i=0; i<min(win_moves->count, MAX_MOVES); i++)
  {
    x = win_moves->moves[i] % 4;
    y = win_moves->moves[i] / 4;
    button[x][y].drawButton(true);
    delay(SOLVE_DELAY);

    // around is a free button
    if(freeButton(x, y, &xv, &yv))
    {
      changeButtons(x, y, xv, yv);
      redrawSquare(x, y, xv, yv);
      moves++;
    }
  }
}
// -----------------------------------------------------------------------------

void solveLoyd15()
{
   T_loyd loyd_solve;
   unsigned long time_sum = millis();

   border_x = 0;
   border_y = 0;
   moves = 0;
   
   for(int ii=0; ii< (sizeof(solve_process)/sizeof(solve_process[0])); ii++)
   {
     solve_number = solve_process[ii];
     loyd_solve = initSolve(solve_number);
#ifdef DEBUG
     Serial.print("solving ");
     Serial.println(solve_number);
#endif

     solveRecursion(loyd_solve, e_move_start);
     viewSolve(&win_moves);

     // reduce square border, will be quickly
     if( solve_number == 4 ) border_y = 1;
     if( solve_number == 8 ) border_y = 2;
     if( solve_number == 13 ) border_x = 1;
   }

#ifdef DEBUG
   Serial.print("time = "); Serial.print((millis()-time_sum) / 1000); Serial.println(" [s]");
#endif
}
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
      EEPROM.write(sizeof(loyd) + sizeof(moves), level);
  }
  else
  {
      for(int y=0; y<4; y++)
        for(int x=0; x<4;  x++)
           loyd[x][y] = EEPROM.read(y*4+x);
           
      moves = EEPROM.read(sizeof(loyd));
      level = EEPROM.read(sizeof(loyd) + sizeof(moves));
  }
}
// -----------------------------------------------------------------------------

char* ConvertToChar(byte number)
{
  switch( level )
  {
    case 3:                                       // random
          number = random(1, number + 1);
          /* no break; */
    case 0:                                       // number
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
          break;
    case 1:                                       // alphabet
          text[0] = 'A' + number - 1;
          text[1] = '\0';
          break;
    case 2:                                       // hex
          if( number >= 10 )
            text[0] = 'A' + number - 9;
          else
            text[0] = '0' + number % 10;
        
          text[1] = '\0';
          break;
  }
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
  
  // all buttons
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
  
  // clear
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
  
  // when is not possible solve it then change 2 squares and try again
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
    tft.fillRect(20, 100, 22*i, 11*i, BLACK);
    delay(30);
  }
  
  tft.setTextColor(YELLOW);
  tft.setTextSize(3);
  tft.setCursor(25, 108);
  tft.print(F(" Solve it!"));
  
  tft.setTextSize(2);
  tft.setCursor(30, 145);
  tft.print(F("need moves: "));
  tft.print(moves);

  tft.setTextSize(2);
  tft.setCursor(28, 171);
  tft.print(F("next level:"));
  level = (level+1) % 4;
  switch(level)
  {
    case 0:  tft.print("numb"); break;
    case 1:  tft.print("abcd"); break;
    case 2:  tft.print("hex");   break;
    case 3:  tft.print("rand");  break;  // numbers are randomize from <1,number>, never greather than number
  }
  
  do {
    p = ts.getPoint();
    pinMode(XM, OUTPUT);
    pinMode(YP, OUTPUT);
  }
  while( p.z < MIN_TOUCH );
  
  tft.fillRect(20, 100, 200, 100, BLACK);
}
// -----------------------------------------------------------------------------

void setup(void) 
{
  Serial.begin(9600);
  Serial.println(F("Loyd 15 ver0.92. Software by Zdeno Sekerak (c) 2015."));
  randomSeed(analogRead(A0));

  tft.reset();
  tft.begin(tft.readID());
  tft.fillScreen(BLACK);

  mixLoyd15();
  moves = 0;
  level = 0;
  redrawSquare(0,0,0,0);
  
  // buttons
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
       delay( BUTTON_DELAY/3 );
       button_solve.drawButton(false);
       solveLoyd15();
       showWin();
       redrawSquare(0,0,0,0);
       return;
    }
    
    if( button_save.contains(p.x, p.y))
    {
       button_save.drawButton(true);
       EEPROM_modul(_EEPROM_WRITE);
       delay( BUTTON_DELAY );
       button_save.drawButton(false);
       return;
    }

    if( button_load.contains(p.x, p.y))
    {
       button_load.drawButton(true);
       EEPROM_modul(_EEPROM_READ);
       redrawSquare(0,0,0,0);
       delay( BUTTON_DELAY/3 );
       button_load.drawButton(false);
       return;
    }
    
    for(int y=0; y<4; y++)
      for(int x=0; x<4; x++)
      {
        if( button[x][y].contains(p.x, p.y))
        {
          button[x][y].drawButton(true);
          delay( BUTTON_DELAY );
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
