#include "LedControl.h"

// --------------------------------------------------------------- //
// -------------------- supporting variables --------------------- //
// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- //
/*
 * schema
 * cerinte
 * specificatii
 * utilizare
 * explicatii hard+soft
 * 
 * 
 */

unsigned char rowsDisplayA[8];
unsigned char rowsDisplayB[8];
int buttonState;

float deltaTime;
unsigned long time_;
unsigned long timeOld;

//Player input info
float inputX;
float inputY;
bool buttonDown;
bool buttonUpThisFrame;
bool buttonDownThisFrame;
float buttonDownDuration;

//game variables/////////////////////////////
static const int width = 16;
static const int height = 8;
const float timeBetweenMoves = .2;
const int foodSpawnMillisMin = 400;
const int foodSpawnMillisMax = 2000;

unsigned char x[16 * 8];
unsigned char y[16 * 8];

int snakeLength;
float timeSinceLastMove;

int dirX;
int dirY;
int nextDirX;
int nextDirY;

bool foodExists;
float timeRemainingToNextFoodSpawn;
int foodX;
int foodY;

bool gameOver;
float scoreDisplayAmount;

// --------------------------------------------------------------- //
// ------------------------- user config ------------------------- //
// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- //

//Joystick pins
const int VRx = A0;
const int VRy = A1;
const int SW = 2;

//Display pins
const int dinPin = 9;
const int clkPin = 11;
const int csPin = 10;

//Matrix brightness
const short intensity = 1;

// Led controller
LedControl lc = LedControl(dinPin, clkPin, csPin, 2);  // Pins: DIN, CLK, CS, number of displays


void setup() {
  randomSeed(analogRead(A5));
  
  //joystick setup
  pinMode(VRx, INPUT);
  pinMode(VRy, INPUT);
  pinMode(SW, INPUT_PULLUP); 

  //displays setup
  for(int i=0; i<lc.getDeviceCount(); i++){
        lc.shutdown(i, false); //wake up display
        lc.setIntensity(i, intensity); //set intensity level
        lc.clearDisplay(i); //clear display
  }

  time_ = 0;
  timeOld = millis();
  gameOver = true;

  Serial.begin(9600);
}

void loop() {
    //Get delta time
    unsigned long frameStartTime = millis();
    unsigned long deltaTimeMillis = frameStartTime - timeOld;
    deltaTime = deltaTimeMillis/1000.0;
    timeOld = frameStartTime;
  
    //Update screen
    clearScreen();
    updateLoop();
    updateSnakeGame();
    drawDisplay();
  
    //wait for fps target
    unsigned long endTime = millis();
    unsigned long frameTime = endTime - frameStartTime;
  
    const unsigned long targetDelay = 16;
    if (frameTime < targetDelay) {
      unsigned long waitForFPSTime = targetDelay - frameTime;
      delay(waitForFPSTime);
    }
}


// --------------------------------------------------------------- //
// -------------------------- functions -------------------------- //
// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- //

void clearScreen(){
  for(int i=0; i<8; i++){
    rowsDisplayA[i] = 0;
    rowsDisplayB[i] = 0;
  }
}

void drawDisplay(){
  for(int row=0; row<8; row++){
    lc.setRow(0, row, rowsDisplayA[row]);
    lc.setRow(1, row, rowsDisplayB[row]);
  }
}

float remap(float value, float minOld, float maxOld, float minNew, float maxNew){
  return minNew + (value - minOld)/(maxOld - minOld) * (maxNew - minNew);
}

void setPixelToValue(int x, int y, bool on){
  if (x >= 16 || x < 0 || y >= 8 || y < 0) {
    return;
  }

  if (x >= 8) {
    if (on) { //on
      rowsDisplayA[x-8] |= 1 << y;
    }
    else { //off
      rowsDisplayA[x-8] &= ~(1 << y);
    }
  }
  else {
    if (on) { //on
      rowsDisplayB[x] |= 1 << y;
    }
    else { //off
      rowsDisplayB[x] &= ~(1 << y);
    }
  }
}

void setPixel(int x, int y) {
  setPixelToValue(x, y, true);
}

void updateLoop(){
  time_ += deltaTime;

  //Button
  int buttonStateOld = buttonState;
  
  buttonState = digitalRead(SW);
  buttonDown = buttonState == 1;
  buttonDownThisFrame = buttonDown && buttonState != buttonStateOld;
  buttonUpThisFrame = buttonState == 0 && buttonStateOld == 1;

  if(buttonDownThisFrame){
    buttonDownDuration = 0;
  }
  if(buttonDown){
    buttonDownDuration += deltaTime;
  }

  //Joystick
  const float inputThreshold = 0.1;
  inputX = remap(analogRead(VRx), 0, 1023, -1, 1);
  inputY = remap(analogRead(VRy), 0, 1023, -1, 1);

  Serial.print("X: ");
  Serial.print(nextDirX);
  Serial.print(" Y: ");
  Serial.print(nextDirY);
  Serial.print(" time: ");
  Serial.print(timeSinceLastMove);
  Serial.print(" time2: ");
  Serial.print(timeBetweenMoves);
  Serial.print("\n");
//  Serial.print("Button :");
//  Serial.print(buttonState);
//  Serial.print("\n");
  
  if (abs(inputX) < inputThreshold) {
    inputX = 0;
  }
  if (abs(inputY) < inputThreshold) {
    inputY = 0;
  }
}

void startupSequence() {
  // Frame 1
  for (int y = 0; y < 8; y ++) {
    for (int x = 0; x < 16; x ++) {
      if (!(x <= y || x-8 >= y)) {
        setPixel(x,y);
      }
    }
  }
  drawDisplay();
  delay(320);

  // Frame 2
  for (int y = 0; y < 8; y ++) {
    for (int x = 0; x < 16; x ++) {
      if (x <= y) {
        setPixel(x,y);
      }
    }
  }
  drawDisplay();
  delay(320);

  // Frame 3
  for (int y = 0; y < 8; y ++) {
    for (int x = 0; x < 16; x ++) {
        setPixel(x,y);
     }
   }
  drawDisplay();
  delay(700);

  // Transition out
  for (int i = 0; i < 16; i ++) {
    for (int x = 0; x < 16; x ++) {
      for (int y = 0; y < 8; y ++) {
        if ((x+(8-y) <= i || 16-x+y <= i)) {
          setPixelToValue(x, y, false);
        }
      }
    }
    drawDisplay();
    delay(35);
  }
  delay(30);
}


void snakeGame(){
  snakeLength = 3;
  timeSinceLastMove = 0;

  x[0] = 0;
  y[0] = 3;
  dirX = 1;
  dirY = 0;
  nextDirX = dirX;
  nextDirY = dirY;

  timeRemainingToNextFoodSpawn = 1.5;
  foodExists = false;
  gameOver = false;
  scoreDisplayAmount = -5;
}

int sign(float value) {
  if(value == 0) return 0;
  if(value < 0) return -1;
  if(value > 0) return 1;
}

void updateSnakeGame(){
  if(gameOver){
//    scoreDisplayAmount += deltaTime * 10;
//    for(int i=0; i<min(snakeLength, (int)scoreDisplayAmount); i++){
//      int x = i%16;
//      int y = i/16;
//      setPixel(x, y);
//    }
    startupSequence();
    snakeGame();
  }

  timeSinceLastMove += deltaTime;

  if(inputX != 0 || inputY != 0){
    if(abs(inputX) > abs(inputY)){
      int inputDirX = sign(inputX);
      if(inputDirX != -dirX){ 
        nextDirX = inputDirX;
        nextDirY = 0;
      }
    }
    else{
      int inputDirY = sign(inputY);
      if(inputDirY != -dirY){
        nextDirY = inputDirY;
        nextDirX = 0;
      }
    }
  }

  //move
  if(timeSinceLastMove > timeBetweenMoves){
    timeSinceLastMove = timeSinceLastMove - timeBetweenMoves;

    dirX = nextDirX;
    dirY = nextDirY;

    int newHeadX = x[0] + dirX;
    int newHeadY = y[0] + dirY;

    newHeadX = (newHeadX >= width) ? 0 : newHeadX;
    newHeadX = (newHeadX < 0) ? width-1 : newHeadX;
    newHeadY = (newHeadY >= height) ? 0 : newHeadY;
    newHeadY = (newHeadY < 0) ? height-1 : newHeadY;

    for(int i=snakeLength - 1; i>0; i--){
      x[i] = x[i-1];
      y[i] = y[i-1];
      
      if(newHeadX == x[i] && newHeadY == y[i]){
        gameOver = true;
      }
    }

    x[0] = newHeadX;
    y[0] = newHeadY;

    //eat food
    if(foodExists){
      if(x[0] == foodX && y[0] == foodY){
        foodExists = false;

        int nextPointDirX = -dirX;
        int nextPointDirY = -dirY;
        if(snakeLength > 1){
          nextPointDirX = sign(x[snakeLength-1]-x[snakeLength-2]);
          nextPointDirY = sign(y[snakeLength-1]-y[snakeLength-2]);
        }
        x[snakeLength] = x[snakeLength-1] + nextPointDirX;
        y[snakeLength] = y[snakeLength-1] + nextPointDirY;
        snakeLength = snakeLength + 1;
      }
    }
  }

  for(int i=0; i<snakeLength; i++){
    setPixel((int)x[i], (int)y[i]);
  }

  if(foodExists){
    setPixel(foodX, foodY);
  }else{
    timeRemainingToNextFoodSpawn -= deltaTime;
    if(timeRemainingToNextFoodSpawn <= 0){
      placeFood();
    }
  }
}

void placeFood(){
  int numTiles = width * height;
  int randomIndex = random(0, numTiles);

  bool tilesMap[numTiles] = {false};
  for(int i=0; i<snakeLength; i++){
    int snakeIndex = y[i] * width + x[i];
    tilesMap[snakeIndex] = true;
  }

  for(int i=0; i<numTiles; i++){
    if(tilesMap[randomIndex] == true){
      randomIndex = (randomIndex+1)%numTiles;
    }else{
      foodX = randomIndex % width;
      foodY = randomIndex / width;
      timeRemainingToNextFoodSpawn = random(foodSpawnMillisMin, foodSpawnMillisMax) / 1000.0;
      foodExists = true;
      return;
    }
  }
}
