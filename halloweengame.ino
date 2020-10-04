/*
  The Halloween Game

  The circuit:
 * LCD RS pin to digital pin 12
 * LCD Enable pin to digital pin 11
 * LCD D4 pin to digital pin 5
 * LCD D5 pin to digital pin 4
 * LCD D6 pin to digital pin 3
 * LCD D7 pin to digital pin 2
 * LCD R/W pin to ground
 * LCD VSS pin to ground
 * LCD VCC pin to 5V
 * 10K resistor:
 * ends to +5V and ground
 * wiper to LCD VO pin (pin 3)
 * BUTTON V to 5V, G to ground, S to digital pin 7

 For wiring please look here:

 http://www.arduino.cc/en/Tutorial/LiquidCrystal
 */

#include <LiquidCrystal.h>

byte dude1[8] = {
  B00100,
  B01110,
  B00100,
  B01110,
  B10101,
  B01010,
  B01010,
};

byte dude2[8] = {
  B00100,
  B01110,
  B00100,
  B11111,
  B00100,
  B01010,
  B10001,
};

byte ghost1[8] = {
  B00100,
  B01110,
  B10101,
  B10101,
  B11111,
  B11111,
  B10101,
};

byte ghost2[8] = {
  B00100,
  B01110,
  B10101,
  B11111,
  B11111,
  B11011,
  B01001,
};

// initialize the library with the numbers of the interface pins
LiquidCrystal lcd(12, 11, 5, 4, 3, 2);

const int LINE_LENGTH = 16;

char line1[LINE_LENGTH + 1];
char line2[LINE_LENGTH + 1];
char* background[] = { line1, line2 };
const int NUMBER_OF_LINES = sizeof(background) / sizeof(*background);

const char ASCII_DUDE_1 = 1;
const char ASCII_DUDE_2 = 2;
const char ASCII_GHOST_1 = 3;
const char ASCII_GHOST_2 = 4;

const int BUTTON_INPUT_PIN = 7;

const int MAX_LEVEL = 9;

void setup() {
  pinMode(BUTTON_INPUT_PIN, INPUT);
  lcd.createChar(ASCII_DUDE_1, dude1);
  lcd.createChar(ASCII_DUDE_2, dude2);
  lcd.createChar(ASCII_GHOST_1, ghost1);
  lcd.createChar(ASCII_GHOST_2, ghost2);
  lcd.begin(LINE_LENGTH, NUMBER_OF_LINES);
  reset_background();
}

void reset_background()
{
  for (int y = 0; y < NUMBER_OF_LINES; ++y) { 
    for (int x = 0; x < LINE_LENGTH; ++x) {
      background[y][x] = ' ';
    }
  }  
}

void scroll_background()
{
  for (int y = 0; y < NUMBER_OF_LINES; ++y) { 
    for (int x = 0; x < LINE_LENGTH - 1; ++x) {
      background[y][x] = background[y][x + 1];
    }
  }
}

void draw_background()
{
  for (int y = 0; y < NUMBER_OF_LINES; ++y) { 
    lcd.setCursor(0, y);
    lcd.print(background[y]);
  }
}

void animate_background()
{
  for (int y = 0; y < NUMBER_OF_LINES; ++y) { 
    for (int x = 0; x < LINE_LENGTH; ++x) {
      char ch = background[y][x];
      if (ch == ASCII_GHOST_1) {
        background[y][x] = ASCII_GHOST_2;
      } else if (ch == ASCII_GHOST_2) {
        background[y][x] = ASCII_GHOST_1;
      }
    }
  }  
}

void add_monsters(int frame, int level)
{
  bool prev = false;
  for (int y = 0; y < NUMBER_OF_LINES; ++y) {
    int x = random(MAX_LEVEL + 2 - level);
    if (!prev) {
      prev = (x == 1);
    } else {
      prev = false;
    }
    background[y][LINE_LENGTH - 1] = prev ? ( frame % 2 ? ASCII_GHOST_1 : ASCII_GHOST_2) : ' ';
  }
}

void draw_character(int frame, int y) 
{
  background[y][1] = frame % 2 ? ASCII_DUDE_1 : ASCII_DUDE_2;
}

void remove_character(int y) {
  background[y][1] = ' ';
}

int check_ending(int frame, int end_frame, int char_x, int char_y) {
  if (end_frame == 0) {
    if (background[char_y][char_x + 1] != ' ') {
      return frame;
    }
  } else {
    if (frame - end_frame == 7) {
      return 0;
    } else {
      return end_frame;
    }
  }
  return 0;
}

int score_game(int level, int char_x, int char_y) {
  int score = 0;
  if (background[char_y][char_x+1] != ' ') {
    score = 10 * level;
  }
  if (background[char_y][char_x+2] != ' ') {
    score = 5 * level;
  }
  ++score;
  return score;
}

void game_over(int frame, int score) 
{
  int line = NUMBER_OF_LINES / 2 - 1;
  reset_background();
  if (frame % 4 != 0) {
    const char* message = "GAME OVER";
    sprintf(&background[line][(LINE_LENGTH - strlen(message))/2], "%s", message);
  }
  char buffer[LINE_LENGTH + 1];
  sprintf(buffer, "SCORE: %d", score);
  sprintf(&background[line + 1][(LINE_LENGTH - strlen(buffer))/2], "%s", buffer);
}

void loop() {
  static int curMils = 0;
  static int frame = 0;
  static int end_frame = 0;
  static int char_pos = 0;
  static int new_char_pos = 0;
  static int level = 0;
  static int lastRead = 0;
  static int attempts = 3;
  static bool remove_attempt = false;
  static int score = 0;
  static bool enable_input = true;

  const bool GLORIAS_MODE = true;

  // handle input
  int newRead = digitalRead(BUTTON_INPUT_PIN);
  if (enable_input && lastRead == 0 && newRead == 1) {
    new_char_pos = char_pos + 1;
    new_char_pos %= NUMBER_OF_LINES;
  }
  lastRead = newRead;

  int newMils;
  // game speed
  if (GLORIAS_MODE) {
    newMils = millis() / 1000;
  } else {
    newMils = millis() / (500 - level * 50);
  }
  
  // handle game state
  if (curMils != newMils) {
    end_frame = check_ending(frame, end_frame, 1, new_char_pos);
    if (end_frame != 0) {
      // oh, no!
      // captured by ghost
      enable_input = false;
      remove_character(char_pos);
      remove_attempt = true;
      // blink character
      frame % 2 ?  draw_character(end_frame - 1, new_char_pos) : remove_character(new_char_pos);
    } else {
      enable_input = true;
      if (remove_attempt) {
        --attempts;
        remove_attempt = false;
      }
      // is it game over?
      if (attempts == 0) {
        enable_input = false;
        game_over(frame, score);
      } else {
        if (level < MAX_LEVEL && frame % 100 == 0) {
          // spice up the game!
          ++level;
        }
        remove_character(char_pos);
        scroll_background();
        animate_background();
        add_monsters(frame, level);
        char_pos = new_char_pos;
        draw_character(frame, char_pos);
        score += score_game(level, 1, char_pos);
      }
    }
    draw_background();
    curMils = newMils;
    ++frame;
  }
}
