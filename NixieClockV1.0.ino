// Arduinix 6 Bulb - Also supports Hour and Min. time set.
//
// This code runs a six bulb setup and displays a prototype clock setup.
// NOTE: the delay is setup for IN-17 nixie bulbs.
//
// original code by Jeremy Howa
// www.robotpirate.com
// www.arduinix.com
// 2008 - 2009
// (Nov 2013) code modified by Andrea Biffi www.andreabiffi.com to work with only one SN74141 (Nov 2013)
// (Jan 2016) code modified by Germán Ruiz to add various features as: Now the clock will read the current time
// from a Real Time Clock Module. Using code from this tutorial:
// http://tronixstuff.com/2014/12/01/tutorial-using-ds1307-and-ds3231-real-time-clock-modules-with-arduino/
// Elimination of the "Ghosting effect".
// "Slot machine" routine every hour at XX:59:55 to prevent catothe poisoning. Using <Timer.h> to control the
// antiPoisoning routine  http://playground.arduino.cc/Code/Timer
// Two single buttons to change the hours and minutes. Using <Bounce.h> http://playground.arduino.cc/Code/Bounce
// Added an ilumination routine "iluminacion(RTChour, RTCdayOfWeek)" to control the color of 6 RGB leds, each hour
// of each day of the week will show a different color.
//

// SN74141 : True Table
//D C B A #
//L,L,L,L 0
//L,L,L,H 1
//L,L,H,L 2
//L,L,H,H 3
//L,H,L,L 4
//L,H,L,H 5
//L,H,H,L 6
//L,H,H,H 7
//H,L,L,L 8
//H,L,L,H 9

#include <Timer.h>    //To control the antiPoisoning routine  http://playground.arduino.cc/Code/Timer
const unsigned long period = 100; //0.1 seconds
Timer t;

//Librería para el uso del RTC
#include "Wire.h"
#define DS3231_I2C_ADDRESS 0x68
///////////////////////////

//Constantes y librerias para el uso de botones con debouncer
#include <Bounce.h>

const int button1Pin = 7;
const int button2Pin = 4;
Bounce pushbutton1 = Bounce(button1Pin, 10);  // 10 ms debounce
Bounce pushbutton2 = Bounce(button2Pin, 10);  // 10 ms debounce

byte previousState = HIGH;         // what state was the button last time
unsigned long RTChourAt = 0;         // when hora changed
unsigned long RTCminuteAt = 0;         // when minuto changed
unsigned int  RTChourPrinted = 0;    // last hora printed
unsigned int  RTCminutePrinted = 0;    // last minuto printed

///////////////////////////

// SN74141       Ard.Pin     IC.pin
int ledPin_0_a = A0; //A0  A  2
int ledPin_0_b = A1; //A1  B  6
int ledPin_0_c = A2; //A2  C  7
int ledPin_0_d = A3; //A3  D  4

// anod pins
int nixiePin_a_1 = 3;
int nixiePin_a_6 = 5;
int nixiePin_a_5 = 6;
int nixiePin_a_4 = 8;
int nixiePin_a_3 = 13;
int nixiePin_a_2 = 12;

int j1 = 0;  //Variable 1 auxiliar de funcionAntiPoison()

//Leds RGB
int ledR = 9;
int ledG = 10;
int ledB = 11;

//                     1     2      3       4        5       6
//Patron de colores: Rojo, Morado, Azul, Turquesa, Verde, Amarillo
int patronR[] = {255, 255, 0,   0,   0,   255, 255};
int patronG[] = {0,   0,   0,   255, 255, 255, 255};
int patronB[] = {0,   255, 255, 255, 0,   0,   255};

void setup()
{
  pinMode(ledPin_0_a, OUTPUT);
  pinMode(ledPin_0_b, OUTPUT);
  pinMode(ledPin_0_c, OUTPUT);
  pinMode(ledPin_0_d, OUTPUT);

  pinMode(nixiePin_a_1, OUTPUT);
  pinMode(nixiePin_a_2, OUTPUT);
  pinMode(nixiePin_a_3, OUTPUT);
  pinMode(nixiePin_a_4, OUTPUT);
  pinMode(nixiePin_a_5, OUTPUT);
  pinMode(nixiePin_a_6, OUTPUT);

  pinMode(ledR, OUTPUT);
  pinMode(ledG, OUTPUT);
  pinMode(ledB, OUTPUT);

  Wire.begin();

  pinMode(button1Pin, INPUT_PULLUP);
  pinMode(button2Pin, INPUT_PULLUP);

  int counter = t.every( period, poisonAux); //every 100ms change the value of j1

  //De-comment this to allow Serial Monitor
  /*Serial.begin(9600);
    set the initial time here:
    DS3231 (seconds, minutes, hours, day, date, month, year)
    setDS3231time(30,42,21,4,26,11,14);*/
}

////////////////////////////////////////////////////////////////////////
//FUNCIONES DEL RTC

byte decToBcd(byte val)
{
  return ( (val / 10 * 16) + (val % 10) );
}
// Convert binary coded decimal to normal decimal numbers
byte bcdToDec(byte val)
{
  return ( (val / 16 * 10) + (val % 16) );
}

void setDS3231time(byte second, byte minute, byte hour, byte dayOfWeek, byte
                   dayOfMonth, byte month, byte year)
{
  // sets time and date data to DS3231
  Wire.beginTransmission(DS3231_I2C_ADDRESS);
  Wire.write(0); // set next input to start at the seconds register
  Wire.write(decToBcd(second)); // set seconds
  Wire.write(decToBcd(minute)); // set minutes
  Wire.write(decToBcd(hour)); // set hours
  Wire.write(decToBcd(dayOfWeek)); // set day of week (1=Sunday, 7=Saturday)
  Wire.write(decToBcd(dayOfMonth)); // set date (1 to 31)
  Wire.write(decToBcd(month)); // set month
  Wire.write(decToBcd(year)); // set year (0 to 99)
  Wire.endTransmission();
}
void readDS3231time(byte *second,
                    byte *minute,
                    byte *hour,
                    byte *dayOfWeek,
                    byte *dayOfMonth,
                    byte *month,
                    byte *year)
{
  Wire.beginTransmission(DS3231_I2C_ADDRESS);
  Wire.write(0); // set DS3231 register pointer to 00h
  Wire.endTransmission();
  Wire.requestFrom(DS3231_I2C_ADDRESS, 7);
  // request seven bytes of data from DS3231 starting from register 00h
  *second = bcdToDec(Wire.read() & 0x7f);
  *minute = bcdToDec(Wire.read());
  *hour = bcdToDec(Wire.read() & 0x3f);
  *dayOfWeek = bcdToDec(Wire.read());
  *dayOfMonth = bcdToDec(Wire.read());
  *month = bcdToDec(Wire.read());
  *year = bcdToDec(Wire.read());
}

void serialMonitorShowTime()
{
  byte second, minute, hour, dayOfWeek, dayOfMonth, month, year;
  // retrieve data from DS3231
  readDS3231time(&second, &minute, &hour, &dayOfWeek, &dayOfMonth, &month,
                 &year);
  // send it to the serial monitor
  Serial.print(hour, DEC);
  // convert the byte variable to a decimal number when displayed
  Serial.print(":");
  if (minute < 10)
  {
    Serial.print("0");
  }
  Serial.print(minute, DEC);
  Serial.print(":");
  if (second < 10)
  {
    Serial.print("0");
  }
  Serial.print(second, DEC);
  Serial.print(" ");
  Serial.print(dayOfMonth, DEC);
  Serial.print("/");
  Serial.print(month, DEC);
  Serial.print("/");
  Serial.print(year, DEC);
  Serial.print(" Day of week: ");
  switch (dayOfWeek) {
    case 1:
      Serial.println("Sunday");
      break;
    case 2:
      Serial.println("Monday");
      break;
    case 3:
      Serial.println("Tuesday");
      break;
    case 4:
      Serial.println("Wednesday");
      break;
    case 5:
      Serial.println("Thursday");
      break;
    case 6:
      Serial.println("Friday");
      break;
    case 7:
      Serial.println("Saturday");
      break;
  }
}
////////////////////////////////////////////////////////////////////////



////////////////////////////////////////////////////////////////////////
//
// DisplayNumberSet
// Use: Passing anod number, and number for bulb, this function
//      looks up the truth table and opens the correct outs from the arduino
//      to light the numbers given to this funciton.
//      On a 6 nixie bulb setup.
//
////////////////////////////////////////////////////////////////////////
void DisplayNumberSet( int anod, int num1, byte minuto, byte segundo, byte RTChour, byte RTCdayOfWeek)
{
  int anodPin;
  int a, b, c, d;
  boolean antiGhosting = false;   //Sirve para evitar el ghosting, cuando está a TRUE todos los nixies estaran apagados
  boolean antiPoison = false;     //Sirve para evitar cathode poisoning, cuando está a TRUE llamará a una función que iluminará todos los dígitos de todos los nixies (Una vez cada hora)

  //Cuando tengamos estos valores llamaremos la rutina de funcionAntiPoison
  if (minuto == 59 && segundo == 55) antiPoison = true;
  if (minuto == 59 && segundo == 56) antiPoison = true;
  if (minuto == 59 && segundo == 57) antiPoison = true;
  if (minuto == 59 && segundo == 58) antiPoison = true;
  if (minuto == 59 && segundo == 59) antiPoison = true;
  if (minuto == 00 && segundo == 00) antiPoison = false;

  // set defaults.
  a = 0; b = 0; c = 0; d = 0; // will display a zero.
  anodPin =  nixiePin_a_1;     // default on first anod.

  if (antiPoison == false) {
    iluminacion(RTChour, RTCdayOfWeek);
    // Select what anod to fire.      Antes de avanzar al siguiente anodo apagaremos todos los nixies para evitar el ghosting
    switch ( anod )
    {
      case 0:    anodPin =  nixiePin_a_1;     break;
      case 1:    antiGhosting = true;       break;
      case 2:    anodPin =  nixiePin_a_2;     break;
      case 3:    antiGhosting = true;       break;
      case 4:    anodPin =  nixiePin_a_3;     break;
      case 5:    antiGhosting = true;       break;
      case 6:    anodPin =  nixiePin_a_4;     break;
      case 7:    antiGhosting = true;       break;
      case 8:    anodPin =  nixiePin_a_5;     break;
      case 9:    antiGhosting = true;       break;
      case 10:   anodPin =  nixiePin_a_6;     break;
      case 11:   antiGhosting = true;       break;
    }


    // Load the a,b,c,d to send to the SN74141 IC (1)
    switch ( num1 )
    {
      case 0: a = 0; b = 0; c = 0; d = 0; break;
      case 1: a = 1; b = 0; c = 0; d = 0; break;
      case 2: a = 0; b = 1; c = 0; d = 0; break;
      case 3: a = 1; b = 1; c = 0; d = 0; break;
      case 4: a = 0; b = 0; c = 1; d = 0; break;
      case 5: a = 1; b = 0; c = 1; d = 0; break;
      case 6: a = 0; b = 1; c = 1; d = 0; break;
      case 7: a = 1; b = 1; c = 1; d = 0; break;
      case 8: a = 0; b = 0; c = 0; d = 1; break;
      case 9: a = 1; b = 0; c = 0; d = 1; break;
    }



    if (antiGhosting == false) {            //Si no estamos en etapa de ghosting se iluminará el anodo que toque
      // Write to output pins.
      digitalWrite(ledPin_0_d, d);
      digitalWrite(ledPin_0_c, c);
      digitalWrite(ledPin_0_b, b);
      digitalWrite(ledPin_0_a, a);

      // Turn on this anod.
      digitalWrite(anodPin, HIGH);
    } else {  //Con antiGhosting en TRUE apagamos el último anodo y quedan todos los nixies apagados durante 0.2ms
      digitalWrite(anodPin, LOW);
      delay(0.2);
      antiGhosting = false;
    }
    delay(1.8);   // Aunque 1.8 + los 0.2ms del antighosting suman los 2ms, que es el delay standard
  } else {  //Con antiPoison TRUE llamamos a la funcion que hace que todos los digitos se muestren como una máquina tragaperras
    funcionAntiPoison(anod, RTCdayOfWeek);
  }

  // Shut off this anod.
  digitalWrite(anodPin, LOW);
}

///////  Funcion para evitar el cathode poisoning
void poisonAux() {
  if ( j1 == 9) {
    j1 = 0;
  } else j1++;

}
void funcionAntiPoison(int anod, int RTCdayOfWeek) {

  int anodPin;
  int a, b, c, d, num1;
  int x; //to enable multicoloring the leds when doing the antipoison routine
  boolean antiGhosting = false;   //Sirve para evitar el ghosting, cuando está a TRUE todos los nixies estaran apagados
  boolean antiPoison = false;     //Sirve para evitar cathode poisoning, cuando está a TRUE llamará a una función que iluminará todos los dígitos de todos los nixies (Una vez cada hora)

  // set defaults.
  a = 0; b = 0; c = 0; d = 0; // will display a zero.
  anodPin =  nixiePin_a_1;     // default on first anod.

  num1 = j1;
  // Select what anod to fire.      Antes de avanzar al siguiente anodo apagaremos todos los nixies para evitar el ghosting
  switch ( anod )
  {
    case 0:    anodPin =  nixiePin_a_1; x = num1; num1 = num1 + 1;    break;
    case 1:    antiGhosting = true;                           break;
    case 2:    anodPin =  nixiePin_a_2; x = num1; num1 = num1 + 2;    break;
    case 3:    antiGhosting = true;                           break;
    case 4:    anodPin =  nixiePin_a_3; x = num1; num1 = num1 + 3;    break;
    case 5:    antiGhosting = true;                           break;
    case 6:    anodPin =  nixiePin_a_4; x = num1; num1 = num1 + 4;    break;
    case 7:    antiGhosting = true;                           break;
    case 8:    anodPin =  nixiePin_a_5; x = num1; num1 = num1 + 5;    break;
    case 9:    antiGhosting = true;                           break;
    case 10:   anodPin =  nixiePin_a_6; x = num1; num1 = num1 + 6;    break;
    case 11:   antiGhosting = true;                           break;
  }

  if ( num1 == 7 ) x = 0;
  if ( num1 == 8 ) x = 1;
  if ( num1 == 9 ) x = 2;
  if ( num1 == 10 ) {
    num1 = 0;
    x = 3;
  }
  if ( num1 == 11 ) {
    num1 = 1;
    x = 4;
  }
  if ( num1 == 12 ) {
    num1 = 2;
    x = 5;
  }
  if ( num1 == 13 ) {
    num1 = 3;
    x = 6;
  }
  if ( num1 == 14 ) {
    num1 = 5;
    x = 0;
  }
  if ( num1 == 15 ) {
    num1 = 6;
    x = 1;
  }

  // Load the a,b,c,d to send to the SN74141 IC (1)
  switch ( num1 )
  {
    case 0: a = 0; b = 0; c = 0; d = 0; break;
    case 1: a = 1; b = 0; c = 0; d = 0; break;
    case 2: a = 0; b = 1; c = 0; d = 0; break;
    case 3: a = 1; b = 1; c = 0; d = 0; break;
    case 4: a = 0; b = 0; c = 1; d = 0; break;
    case 5: a = 1; b = 0; c = 1; d = 0; break;
    case 6: a = 0; b = 1; c = 1; d = 0; break;
    case 7: a = 1; b = 1; c = 1; d = 0; break;
    case 8: a = 0; b = 0; c = 0; d = 1; break;
    case 9: a = 1; b = 0; c = 0; d = 1; break;
  }

  // Write to output pins.
  digitalWrite(ledPin_0_d, d);
  digitalWrite(ledPin_0_c, c);
  digitalWrite(ledPin_0_b, b);
  digitalWrite(ledPin_0_a, a);

  // Turn on this anod.
  digitalWrite(anodPin, HIGH);

  if (antiGhosting == false) {            //Si no estamos en etapa de ghosting se iluminará el anodo que toque
    // Write to output pins.
    digitalWrite(ledPin_0_d, d);
    digitalWrite(ledPin_0_c, c);
    digitalWrite(ledPin_0_b, b);
    digitalWrite(ledPin_0_a, a);

    // Turn on this anod.
    digitalWrite(anodPin, HIGH);
  } else {  //Con antiGhosting en TRUE apagamos el último anodo y quedan todos los nixies apagados durante 0.2ms
    digitalWrite(anodPin, LOW);
    delay(0.2);
    antiGhosting = false;
  }

  delay(1.8);   // Aunque 1.8 + los 0.2ms del antighosting suman los 2ms, que es el delay standard
  iluminacion(num1, RTCdayOfWeek);
  // Shut off this anod.
  digitalWrite(anodPin, LOW);
}
///////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
//
// DisplayNumberString
// Use: passing an array that is 6 elements long will display numbers
//      on a 6 nixie bulb setup.
//
////////////////////////////////////////////////////////////////////////
void DisplayNumberString( int* array , byte minuto, byte segundo, byte RTChour, byte RTCdayOfWeek)
{
  // bank 1 (bulb 1)
  DisplayNumberSet(0, array[0], minuto, segundo, RTChour, RTCdayOfWeek);
  // Anti Ghosting (Todos apagados) (Evita el ghosting)
  DisplayNumberSet(1, array[0], minuto, segundo, RTChour, RTCdayOfWeek);
  // bank 2 (bulb 2)
  DisplayNumberSet(2, array[1], minuto, segundo, RTChour, RTCdayOfWeek);
  // Anti Ghosting (Todos apagados) (Evita el ghosting)
  DisplayNumberSet(3, array[0], minuto, segundo, RTChour, RTCdayOfWeek);
  // bank 3 (bulb 3)
  DisplayNumberSet(4, array[2], minuto, segundo, RTChour, RTCdayOfWeek);
  // Anti Ghosting (Todos apagados) (Evita el ghosting)
  DisplayNumberSet(5, array[0], minuto, segundo, RTChour, RTCdayOfWeek);
  // bank 4 (bulb 4)
  DisplayNumberSet(6, array[3], minuto, segundo, RTChour, RTCdayOfWeek);
  // Anti Ghosting (Todos apagados) (Evita el ghosting)
  DisplayNumberSet(7, array[0], minuto, segundo, RTChour, RTCdayOfWeek);
  // bank 5 (bulb 5)
  DisplayNumberSet(8, array[4], minuto, segundo, RTChour, RTCdayOfWeek);
  // Anti Ghosting (Todos apagados) (Evita el ghosting)
  DisplayNumberSet(9, array[0], minuto, segundo, RTChour, RTCdayOfWeek);
  // bank 6 (bulb 6)
  DisplayNumberSet(10, array[5], minuto, segundo, RTChour, RTCdayOfWeek);
  // Anti Ghosting (Todos apagados) (Evita el ghosting)
  DisplayNumberSet(11, array[0], minuto, segundo, RTChour, RTCdayOfWeek);
}

////////////////////////////////////////////////////////////////////////
//
//
////////////////////////////////////////////////////////////////////////
void displayNixies( ) {
  byte RTCsecond, RTCminute, RTChour, RTCdayOfWeek, RTCdayOfMonth, RTCmonth, RTCyear;
  // retrieve data from DS3231
  readDS3231time(&RTCsecond, &RTCminute, &RTChour, &RTCdayOfWeek, &RTCdayOfMonth, &RTCmonth, &RTCyear);

  // Get the high and low order values for hours,min,seconds.
  int lowerHours = RTChour % 10;
  int upperHours = RTChour - lowerHours;
  int lowerMins = RTCminute % 10;
  int upperMins = RTCminute - lowerMins;
  int lowerSeconds = RTCsecond % 10;
  int upperSeconds = RTCsecond - lowerSeconds;
  if ( upperSeconds >= 10 )   upperSeconds = upperSeconds / 10;
  if ( upperMins >= 10 )      upperMins = upperMins / 10;
  if ( upperHours >= 10 )     upperHours = upperHours / 10;

  // Fill in the Number array used to display on the tubes.
  int NumberArray[6] = {0, 0, 0, 0, 0, 0};
  NumberArray[0] = upperHours;
  NumberArray[1] = lowerHours;
  NumberArray[2] = upperMins;
  NumberArray[3] = lowerMins;
  NumberArray[4] = upperSeconds;
  NumberArray[5] = lowerSeconds;

  // Display.
  DisplayNumberString( NumberArray, RTCminute, RTCsecond, RTChour, RTCdayOfWeek);
}

///////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////
//Rutina para el cambio de hora
void cambioHora() {
  byte RTCsecond, RTCminute, RTChour, RTCdayOfWeek, RTCdayOfMonth, RTCmonth, RTCyear;

  if (pushbutton1.update()) {
    if (pushbutton1.risingEdge()) {

      // retrieve data from DS3231
      readDS3231time(&RTCsecond, &RTCminute, &RTChour, &RTCdayOfWeek, &RTCdayOfMonth, &RTCmonth, &RTCyear);

      if (RTChour < 23) {
        RTChour++;
      } else RTChour = 0;
      RTCsecond = 0;

      // retrieve data from DS3231
      setDS3231time(RTCsecond, RTCminute, RTChour, RTCdayOfWeek, RTCdayOfMonth, RTCmonth, RTCyear);

      RTChourAt = millis();
    }
  } else {
    if (RTChour != RTChourPrinted) {
      unsigned long nowMillis = millis();
      if (nowMillis - RTChourAt > 100) {
        RTChourPrinted = RTChour;
      }
    }
  }

  if (pushbutton2.update()) {
    if (pushbutton2.risingEdge()) {

      // retrieve data from DS3231
      readDS3231time(&RTCsecond, &RTCminute, &RTChour, &RTCdayOfWeek, &RTCdayOfMonth, &RTCmonth, &RTCyear);

      if (RTCminute < 59) {
        RTCminute = 59; //=================================================RTCminute++;
      } else RTCminute = 0;

      RTCsecond = 53; //=======================================================RTCsecond = 0;

      // DS3231 (seconds, minutes, hours, day, date, month, year)
      setDS3231time(RTCsecond, RTCminute, RTChour, RTCdayOfWeek, RTCdayOfMonth, RTCmonth, RTCyear);

      RTCminuteAt = millis();
    }
  } else {
    if (RTCminute != RTCminutePrinted) {
      unsigned long nowMillis = millis();
      if (nowMillis - RTCminuteAt > 100) {
        RTCminutePrinted = RTCminute;
      }
    }
  }
}

//////////////////////////
//Control de los LEDs
void iluminacion( int RTChour, int RTCdayOfWeek) {          //Llamamos la función en la subrutina de antiPoison para poder hacer lo de las xiruraines

  int x;

  if ((RTChour == 0)  or  (RTChour == 7)  or  (RTChour == 14)  or  (RTChour == 21)) {
    x = -1 + RTCdayOfWeek;
  } else {
    if ((RTChour == 1)  or  (RTChour == 8)  or  (RTChour == 15)  or  (RTChour == 22)) {
      x = 0 + RTCdayOfWeek;
    } else {
      if ((RTChour == 2)  or  (RTChour == 9)  or  (RTChour == 16)  or  (RTChour == 23)) {
        x = 1 + RTCdayOfWeek;
      } else {
        if ((RTChour == 3)  or  (RTChour == 10)  or  (RTChour == 17)) {
          x = 2 + RTCdayOfWeek;
        } else {
          if ((RTChour == 4)  or  (RTChour == 11)  or  (RTChour == 18)) {
            x = 3 + RTCdayOfWeek;
          } else {
            if ((RTChour == 5)  or  (RTChour == 12)  or  (RTChour == 19)) {
              x = 4 + RTCdayOfWeek;
            } else {
              if ((RTChour == 6)  or  (RTChour == 13)  or  (RTChour == 20)) {
                x = 5 + RTCdayOfWeek;
              }
            }
          }
        }
      }
    }
  }

  if ( x == 7 )  x = 0;
  if ( x == 8 )  x = 1;
  if ( x == 9 )  x = 2;
  if ( x == 10 ) x = 3;
  if ( x == 11 ) x = 5;
  if ( x == 12 ) x = 6;

  analogWrite(ledR, patronR[x]);
  analogWrite(ledG, patronG[x]);
  analogWrite(ledB, patronB[x]);
}

///////////////////////////////////////////////////////////////////////
void loop()
{
  //serialMonitorShowTime();    //Muestra hora, día y año por el serial monitor (Hay que activar el baud 9600 en setup)
  t.update(); //This way the counter in the anti poison routine can keep track of the time
  displayNixies(); //Main code
  cambioHora(); //To change time with the two buttons
}
