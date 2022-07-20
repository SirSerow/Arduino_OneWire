
/* ==================================================================================
 *  
 *  Following program is written for Arduino Nano, embeded in the testing device.
 *  
 *  The device is designed for cable testing puposes. 
 *  
 *  The tested cable is 12x0.25 cable with two D-SUB 15 connectors on each side. 
 *  Each connector has a built-in serial number device DS2401+, connected via one-wire 
 *  protocol.
 *  
 *  The intended test sequence:
 *  
 *  1. Red "START" button is beeing pressed.
 *  
 *  2. Controller checks electrical connection for every of 12 cable cores
 *  and stores the information on the status of each core.
 *  
 *  3. Controller checks each of two 1-Wire devices separately and 
 *  stores the information on the status of each core. 
 *  
 *  4. The device outputs a intermittent sound signal: 1 - system is OK, 3 - malfunction
 *  is detected.
 *  
 *  5. Test data is displayed on the LCD screen.
 *  
 *  6. Repetitve pressing of the "START" button restarts the program.
 *  
 *  Pressing of the "RESET" button reloads the device
 * 
 * ===================================================================================
 */


#include <OneWire.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

LiquidCrystal_I2C lcd(0x27, 16, 2); //Declare LCD configuration and adress

OneWire  ds1(A0); //Declare One-Wire pin to check the presence of first DS2401+
OneWire  ds2(A1); //Declare One-Wire pin to check the presence of second DS2401+

int buzzerPin = A2; //Declare buzzer pin
bool ChanState[14]; // Declare an array for wire check flag
int ChanNum[14] = { 1, 2, 3, 4, 5, 6, 9, 10, 11, 12, 13, 14, 241, 242}; // Numbers of the conductors in a wire (7 - not used) corresponding to Chanstate Array

byte LeftChip[8] = {
  B11111,
  B11111,
  B11111,
  B10101,
  B10101,
  B10101,
  B10101,
};


void setup() {
  
  Serial.begin(9600);
  
  for(int i = 1; i < 13; i++){  //Intialize input pin to verify crimping quality
    pinMode(i, INPUT_PULLUP);
  }
  
  pinMode(buzzerPin, OUTPUT); //Initialize buzzer pin
  digitalWrite(buzzerPin, HIGH); //Set buzzer pin to high (for low-level trigger)
  
  pinMode(A3, INPUT_PULLUP); // Initialize button pin
  
  lcd.init(); // Initialize LCD display
  lcd.backlight(); // Turn the backlight on

  lcd.createChar(0, LeftChip);
  lcd.createChar(1, RightChip);
}

void Beep (int freq, int dur, int rep, int del){
  digitalWrite(buzzerPin, HIGH); //Set buzzer pin to high (for low-level trigger)
  for (int i = 0; i <= rep; i++){
    tone(buzzerPin,freq,dur);
    delay(del);
  }
}

int DeviceCapture( OneWire& object )
{
  byte i;           // This is for the for loops
  boolean present;  // device present varj
  byte data[8];     // container for the data from device
  byte crc_calc;    //calculated CRC
  byte crc_byte;    //actual CRC as sent by DS2401
  //1-Wire bus reset, needed to start operation on the bus,
  //returns a 1/TRUE if presence pulse detected
  object.reset_search();
  present = object.reset();
  if (present == 1)
  {
    Serial.print("===============================================\r\n");
    Serial.println("---------- Device present ----------");
    object.write(0x33);  //Send Read data command
    data[0] = object.read();
    Serial.print("Family code: 0x");
    PrintTwoDigitHex (data[0], 1);
    Serial.print("Hex ROM data: ");
    for (i = 1; i <= 6; i++)
    {
      data[i] = object.read(); //store each byte in different position in array
      PrintTwoDigitHex (data[i], 0);
      Serial.print(" ");
    }
    Serial.println();
    crc_byte = object.read(); //read CRC, this is the last byte
    crc_calc = OneWire::crc8(data, 7); //calculate CRC of the data
    Serial.print("Calculated CRC: 0x");
    PrintTwoDigitHex (crc_calc, 1);
    Serial.print("Actual CRC: 0x");
    PrintTwoDigitHex (crc_byte, 1);
    return 1;
  }
  else //Nothing is connected in the bus
  {
    Serial.print("===============================================\r\n");
    Serial.println("xxxxx Nothing connected xxxxx");
    return 0;
  }
  delay(300);
}

void PrintTwoDigitHex (byte b, boolean newline)  //Prints out device serial numbers
{
  Serial.print(b/16, HEX);
  Serial.print(b%16, HEX);
  if (newline) Serial.println();
}

void TestResult () //Prints out test result
{     
      lcd.clear();    // clear the display
      lcd.setCursor (0,0); // go to start of 1st line
      
      int i,j=0,cr=0;
      
      for(i=0;i<14;i++){ // Check each channel state
        if (ChanState[i] == 1){
          j++;           // increment j if the channel is ok
        }
      }
      if (j == 14){                       // Output successfull test message
           while (digitalRead(A3) == 1){
               lcd.print("WIRE IS OK");
               lcd.setCursor (0,1);           // go to start of 2nd line
               lcd.print("ALL CHANNELS WORK");
               delay(100);
      }
               Beep(1600,500,1,150);      // Single sound signal
          
      }
      else {                            // If any channel is not OK
          Beep(1400,100,3,150);         // Beep three times
               lcd.setCursor (0,0); // go to start of 1st line
               lcd.print("CHECK WIRE");  // Display malfunction notification and clear the display
               delay(1000);
               lcd.clear();
               lcd.setCursor (0,0);    // go to start of 1st line to display failed wires (not chips) 
           while (digitalRead(A3) == 1){
               for(i=0;i<12;i++){
                    if (ChanState[i] == 0 && i < 8){   // Fill up first string of LCD
                      lcd.setCursor (2*i,0);
                      lcd.print(ChanNum[i]);
                    }
                    else if (ChanState[i] == 0 && 8 <= i < 12 ){ // Fill up second string of LCD
                      lcd.setCursor (3*(i-8),1);
                      lcd.print(ChanNum[i]);
                    }
                  }
               if (ChanState[12] == 0){
                  lcd.setCursor (12,1);
                  lcd.write(0);
                  lcd.setCursor (13,1);
                  lcd.print("L");
               }
               if (ChanState[13] == 0){
                  lcd.setCursor (14,1);
                  lcd.write(0);
                  lcd.setCursor (15,1);
                  lcd.print("R"); 
               }
               
           }

      }
      lcd.clear();
  
}


void loop() {

  
   lcd.setCursor (0,0); // go to start of 1st line
   lcd.print("Press to start");
   delay(300);
   if(digitalRead(A3) == 0)  // If "START" button is pressed
   {
      lcd.clear();
      lcd.print("Testing..."); // Continiously display the test sequence
      
      int i, st, j = 0;
      byte addr[8];
      
      for(i = 1; i < 13; i++){
        
          st = digitalRead(i); //Read the state of each wire 
          
          Serial.print("Channel "); // Print the chanel number in the console
          Serial.print(i-1);
          
          if(st == 1)
          {
            Serial.print(" : FAIL\r\n"); // Print the chanel state in the console
            ChanState[i-1]=0; // Store the chanel state in the aray 0 - no connection is ok, 1 - connection is ok
            delay(50);
          }
          else if(st == 0)
          {
            Serial.print(" : OK\r\n"); // Print the chanel state in the console
            ChanState[i-1]=1; // Store the chanel state in the aray
            delay(50);
          }
          else {
            Serial.print(" : UNDEFINED INPUT\r\n"); // Display uexpected result
          }
          lcd.setCursor (i-1,1); // go to start of 2nd line
          lcd.print(ChanState[i-1 ]); // Print the chanel state on the LCD
          
      }
      
      Serial.print("|||||||||||||||||\r\n"); // Print spacer in the console
      delay(100);

      ChanState[12] = DeviceCapture(ds1);
      ChanState[13] = DeviceCapture(ds2);
      
      Serial.print("|||||||||||||||||\r\n"); // Print spacer in the console
      Serial.print("{"); // Print spacer in the console
      for(int i = 0; i < 14; i++){
        Serial.print(ChanState[i]); // 
        Serial.print(" "); // 
      }
      Serial.print("}\r\n"); // 
      Serial.print("===============================================\r\n");
      TestResult();


      
    }
  
}
