#include <Arduino.h>

/*keypad matrix dimensions*/
#define matrixSize 4

/*wait 0.1 ms before accepting a input*/
#define timerCount 100
/*Wait 1.2s before accepting a duplicate input*/
#define repeatCount 12000

/*wait 150ms between subsequent repeated inputs after the second one*/
#define repeatShortCount 1500

/*GPIO pins used for rows & columns*/
#define C1 12
#define C2 13
#define C3 14
#define C4 15

#define R1 16
#define R2 17
#define R3 18
#define R4 19

/*column that is pulled high*/
volatile uint8_t activeColumn  = 0;
/*value of button to be consumed in loop()*/
volatile uint8_t btnPressed = 0;
/*most recently captured input*/
volatile uint8_t lastPressed = 0;
/*row that is pulled high*/
volatile int8_t activeRow = -1;
/*timer to wait a delay to debounce button sufficiently*/
hw_timer_t * timer = NULL;
/*timer to wait a delay before accepting repeat inputs*/
hw_timer_t * repeatTimer = NULL;

/*keypad matrix is as follows*/
//uint8_t btnMatrix[matrixSize][matrixSize]= {{1,2,3,4},{5,6,7,8},{9,10,11,12},{13,14,15,16}};
uint8_t btnMatrix[matrixSize][matrixSize]= {{1,2,3,4},{5,6,7,8},{9,10,11,12},{13,14,15,16}};
/*
      | 1 |  | 2 |  | 3 |  | 4 | 
      | 5 |  | 6 |  | 7 |  | 8 | 
     | 9 |  | 10 |  | 11 |  | 12 | 
     | 13 |  | 14 |  | 15 |  | 16 | 
*/

portMUX_TYPE interruptMux = portMUX_INITIALIZER_UNLOCKED;

void setRepeatTimer(bool setShort)
{
  timerAlarmDisable(repeatTimer);
  timerAlarmWrite(repeatTimer, setShort ? repeatShortCount : repeatCount, false);
}

void startTimer(hw_timer_t * tim)
{
  timerRestart(tim);
  timerAlarmEnable(tim);
}


/*Convert row pin number into 0 based index*/
uint8_t cheekyIndex(uint8_t row)
{
  return row -16;
  //return row == 0 ? 0 : log(row)/log(2);
}

/*Called once per row to determine state*/
uint8_t checkPressed(uint8_t row)
{
  /*Check to see if row is high & check to see if it wasn't already pressed*/
  uint8_t result =digitalRead(row);
  if(result)
  {
    /*Set row to active and start timer to repeat until the button is pulled low*/
    activeRow = row;
    startTimer(timer);
  }
  return result;
}

void IRAM_ATTR pressedTimer() {
  portENTER_CRITICAL_ISR(&interruptMux);
  noInterrupts();
  /*after delay if the pin is still pulled high, accept input*/
  if(digitalRead(activeRow))
  {
    /*check to make sure we havent captured a duplicate, else wait for repeatTimer before we capture a new input*/
    if(lastPressed != btnMatrix[cheekyIndex(activeRow)][activeColumn])
    {
      /*Capture last pressed button*/
      btnPressed = btnMatrix[cheekyIndex(activeRow)][activeColumn];
      lastPressed = btnPressed;
      /*Start repeat timer to accept repeated input after delay*/
      startTimer(repeatTimer);
    }
  }
  /*after delay, button became low so start cycling again & stop timers*/
  else
  {
    timerAlarmDisable(timer);
    setRepeatTimer(false);
    Serial.println("Button Released");
    activeRow = -1;
    lastPressed = 0;
  }
  interrupts();
 portEXIT_CRITICAL_ISR(&interruptMux);
}

void IRAM_ATTR clearRepeat() {
  portENTER_CRITICAL_ISR(&interruptMux);
  noInterrupts();
  /*After delay, clear value*/
  setRepeatTimer(true);
  lastPressed = 0;
  interrupts();
 portEXIT_CRITICAL_ISR(&interruptMux);
}


void setup() {
  /*Init internals*/
  Serial.begin(115200);
  timer = timerBegin(1, 8000, true);
  repeatTimer = timerBegin(2, 8000, true);
  timerAttachInterrupt(timer,pressedTimer,true);
  timerAttachInterrupt(repeatTimer,clearRepeat,true);
  timerAlarmWrite(timer, timerCount, true);
  setRepeatTimer(false);

  /*Init IO*/
  pinMode(C1,OUTPUT);
  pinMode(C2,OUTPUT);
  pinMode(C3,OUTPUT);
  pinMode(C4,OUTPUT);

  pinMode(R1,INPUT);
  pinMode(R2,INPUT);
  pinMode(R3,INPUT);
  pinMode(R4,INPUT);
}

void loop() {
  // put your main code here, to run repeatedly:
  delay(10);
  portENTER_CRITICAL(&interruptMux);
  noInterrupts();
  /*Cycle keypad when no button is pressed && most recent input has been consumed*/
  if(activeRow == -1 && btnPressed ==0)
  {
    /*Cycle columns of keypad*/
    activeColumn = (activeColumn+1) %matrixSize;
    digitalWrite(C1, activeColumn == 0);
    digitalWrite(C2, activeColumn == 1);
    digitalWrite(C3, activeColumn == 2);
    digitalWrite(C4, activeColumn == 3);

    if(checkPressed(R1) || checkPressed(R2) || checkPressed(R3)|| checkPressed(R4))
    {
      Serial.println("Button Pressed");
    }

  }
  /*Consume button pressed*/
  if(btnPressed != 0)
  {
    Serial.println(btnPressed);
    btnPressed = 0;
  }
  interrupts();
  portEXIT_CRITICAL(&interruptMux);
  
}





