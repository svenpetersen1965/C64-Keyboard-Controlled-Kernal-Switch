#include <EEPROM.h>

/* Configuration */
const bool recover_empty = true;     //enable recover to Kernal one, in case an empty/not working kernal was selected
const int  NumKernal = 8;            //This is the highest valid Kernal number
 
/* pin definitions */
const int row0 = 2;         // Row signal, input, active low, INT0
const int row3 = 3;         // Row signal, input, active low, INT1
const int KSW_A13 = 4;      // output, A13 to kernal adaptor/switch
const int KSW_A14 = 5;      // output, A14 to kernal adaptor/switch
const int KSW_A15 = 6;      // output, A15 to kernal adaptor/switch
const int C64RESET = 7;     // output, RESET signal for the C64
const int col1 = A0;        // input, column signal, active low
const int col2 = A1;        // input, column signal, active low
const int col3 = A2;        // input, column signal, active low
const int col4 = A3;        // input, column signal, active low
const int col7 = A4;        // input, column signal, active low
const int nRESTORE = A5;    // input, RESTORE key, active low
const int nSHORT_BRD = 8;   // input, short board jumper
const int RSVD1 = 9;        // reserved. Pin connected to pin header J4, pin 5 
const int RSVD2 = 10;       // reserved. Pin connected to pin header J4, pin 6
const int Restore_Count = 2000; // count down appr. 2 seconds for RestoreKey before Reset

/* other constants */
#define EEPROMAddr 0        // EEPROM Address of last kernal selection
#define ScanDelay 10000     // delay ibn ms before reset for no scan activity or an empty kernal memory slot, respectively

/* variables */
byte KOffset = 0;           // offset for kernal number
byte KNumber = 0;           // Kernal number
byte KAddr = 0;             // Address bits for Kernal
bool RestoreKey = 0;        // holds the info, if the restore key is pressed 
int  countdown = 0;         // count down for something
byte key = 128;             // calculate the Key
int  KMax;                  // holds the highest Kernal Number


/* Data Exchange variables for ISR (have to be volatile) */
volatile bool NumPressed = 0;  // ISR flag for number pressed
volatile bool NumOdd = 0;      // ISR flag for odd numbers. Row0 => odd numbers, Row3 => even numbers
volatile byte ColData = 0x1F;  // column data, read from PORTD

/* in case, the kernal switch switches to a memory slot without a proper kernal software
 *  there is no keyboard scan activity. That is why the kernal switch cannot detect keys anymore
 *  to circumvent this, the column scan activity is detected. If there is no activity after switching kernals
 *  the kernal is set to #1 and the C64 is reset.
 */
byte columns;           // variable for reading out the column
int scan_cnt;           // 5000 miliseconds without scan changing to kernal #1 and resetting
int scan_found;         // flag for keybioard activity detected
byte inactive = 0;      // keyboard scan inactive: 1 -> inactive, 0 = active

void setup() {
  /* keyboard scan signals */
  pinMode( row0, INPUT_PULLUP);  // pullup required for MOS logic
  pinMode( row3, INPUT_PULLUP);  // pullup required for MOS logic
  pinMode( col1, INPUT_PULLUP);  // pullup required for MOS logic
  pinMode( col2, INPUT_PULLUP);  // pullup required for MOS logic
  pinMode( col3, INPUT_PULLUP);  // pullup required for MOS logic
  pinMode( col4, INPUT_PULLUP);  // pullup required for MOS logic
  pinMode( col7, INPUT_PULLUP);  // pullup required for MOS logic
  pinMode( nRESTORE, INPUT_PULLUP);  // pullup required for MOS logic

  /* jumper */
  pinMode(nSHORT_BRD, INPUT);   // jumper: LOW = Short Board => lowest kernal number is 1
  if (digitalRead(nSHORT_BRD) == LOW)
  { 
    KOffset = 1; // it is a short board! kernal number offset is 1, since the BASIC is in the lowest 8k of the Kernal ROM
  }

  /* last kernal from eeprom */
  KNumber = EEPROM.read(EEPROMAddr);
  if ((KNumber < 1) || (KNumber > 8-KOffset)) { // if out of range, the first kernal is selected.
    KNumber = 1;
    EEPROM.write(EEPROMAddr, KNumber);
  }
  inactive = EEPROM.read(EEPROMAddr+1);
  if (inactive > 1) {
    inactive = 0;
    EEPROM.write(EEPROMAddr+1, inactive);
  }
  KAddr = KNumber - 1 + KOffset; // Calculate the address bits.
  // For long boards, it should be between 0 and 7,
  // for short boards, it should be 1 to 7, because the first 8k are occupied by the BASIC ROM. 
  
  /* Control outputs (for Kernal Adaptor and C64) */
  SetAddressBits( KAddr );        // set the MS address bits for the kernal ROM
  pinMode( KSW_A13, OUTPUT );     // make it an output   
  pinMode( KSW_A14, OUTPUT );     // make it an output
  pinMode( KSW_A15, OUTPUT );     // make it an output
  digitalWrite( C64RESET, LOW );  // no reset for the C64
  pinMode( C64RESET, OUTPUT );    // make it an output

  if (recover_empty == true) {
    scan_cnt = ScanDelay;         // reset scan countdown
    scan_found = false;           // reset detection flag 
    KMax = 8 - KOffset; 
  }
  else {
    scan_cnt = 0;                 // reset scan countdown
    scan_found = true;            // reset detection flag
    KMax = NumKernal;
  }
  if (inactive == 1) {             // if scan timeout is inactive, just set scan_found
    scan_found = true;
    scan_cnt = 0;
  }
      
  attachInterrupt(digitalPinToInterrupt(row0), ISR_row0, LOW); 
  attachInterrupt(digitalPinToInterrupt(row3), ISR_row3, LOW);
  
  Serial.begin(9600);  // start serial interface
  Serial.println( "\nC64 Keyboard Controlled Kernal Switch v0.0" );
  if (KOffset == 1) {
    Serial.println("Short Board configured.");
  }
  else {
    Serial.println("Long Board configured.");
  }
  if (recover_empty == true) {
    Serial.print("Empty Kernal slot (=scan activity) detection: ");
    if (inactive == 1 ) {
      Serial.println("inactive.");
    }
    else {
      Serial.println("active.");
    }
  }
  else {
    Serial.print("Number of Kernals: ");
    Serial.println(NumKernal);
  }
  Serial.print("Kernal number: ");
  Serial.println( KNumber );
}

/* This function sets the address bits on the pin header J4. Parameter is addr (0..7) */
void SetAddressBits( byte addr ) {
   if ((addr & 1) == 0) {         // A13
    digitalWrite(KSW_A13, LOW);
  }
  else {
    digitalWrite(KSW_A13, HIGH);  
  }
  if ((addr & 2) == 0) {          // A14
    digitalWrite(KSW_A14, LOW);
  }
  else {
    digitalWrite(KSW_A14, HIGH);  
  }
  if ((addr & 4) == 0) {          // A15
    digitalWrite(KSW_A15, LOW);
  }
  else {
    digitalWrite(KSW_A15, HIGH);  
  }
}

void SwitchKernal() {
  SetAddressBits( KAddr );                                   // set the address bits A15..A13
  digitalWrite( C64RESET, HIGH );                            // issue a reset pulse for the C64
  Serial.print("Change to Kernal number: ");                 // send the status to serial
  Serial.println( KNumber );
  delay( 2000 );                                             // pulse duration: two seconds
  digitalWrite( C64RESET, LOW );                             
  delay( 2000 );                                             // wait 2 seconds
  if ((recover_empty == true) && (inactive == 0)) {
    scan_cnt = ScanDelay;                                    // reset scan countdown
    scan_found = false;                                      // reset detection flag
  }  
  else {
    scan_cnt = 0;
    scan_found = true;
  }
}

// Interrupt Service Routine for Row0
void ISR_row0() {
  if (NumPressed == 0) {                  // only process info, after the flag was reset by main routine
     ColData = PINC & 0x1F;               // read the columns
     scan_found = true;                   // any activity here indicates a working kernal
     NumOdd = 1;                          // row0 has the odd numbers
     if ((ColData != 0x1F) && (ColData !=0)) {               // only set the flag, when a number is pressed 
       NumPressed = 1;
     }  
  }
}

// Interrupt Service Routine for Row3
void ISR_row3 () {
  if (NumPressed == 0) {                  // only process info, after the flag was reset by main routine
     ColData = PINC & 0x1F;               // read the columns
     scan_found = true;                   // any activity here indicates a working kernal
     NumOdd = 0;                          // row3 has the even numbers
     if ((ColData != 0x1F) && (ColData !=0)) { // only set the flag, when a number is pressed 
       NumPressed = 1;
     }  
  }
}

void loop() {
  /* find the scan activity to make sure, the selected Kernal is working */
  if (scan_found == false) {                            // if not yet a scan activity is detected
    columns = PINC & 0x1F;                              // read the column data
    if ((columns != 0x1F)) {                            // activity found?
      scan_found = true;                                // set flag
      Serial.println( "Keyboard scan detected."); 
    }
    else {
      if (scan_cnt > 0) {                               // if the count down is not elapsed
        scan_cnt--;                                     // then decrement
        delayMicroseconds( 33 );
      }
      else {
        Serial.println( "No keyboard scan detected.");  // the count down is elapsed!
        KNumber = 1;                                    // Set Kernal Number 1 (it is assumed, that this is not empty)
        EEPROM.write(EEPROMAddr, KNumber);              // and save in EEPROM
        KAddr = KOffset;                                // Calculate the address bits.
        SwitchKernal();                                 // Switch Kernal Number and reset C64
      }
    }
  }
  
  /* process restore key */ 
  if (digitalRead( nRESTORE ) == HIGH) {
     RestoreKey = 0;                      // in case the reading is HIGH, the Restore key is not pressed
     countdown = Restore_Count;           // restart the count down
  }
  else {
    RestoreKey = 1;                       // in case the restore is pressed, count down
    if (countdown > 0) {
      --countdown;
      if (countdown == 0) {               // count down elapsed?
        digitalWrite( C64RESET, HIGH );   // issue a reset pulse for the C64
        Serial.println("Restore key => Reseting the C64");
        if (recover_empty == true) {
          scan_cnt = ScanDelay;             // reset scan countdown
          scan_found = false;               // reset detection flag
        }
        delay( 2000 );                    // for two seconds
        digitalWrite( C64RESET, LOW );  
      }
    }
  }

  /* The ISR will set NumPressed to 1, in case some number key is pressed 
     In case something is found, the column data, which was read by the ISR 
     is being processed here.  
  */
  if (NumPressed == 1) {                     // if a number key is pressed: process the data
    if ((ColData & B00010000) == 0) {        // column 7: "1"
      key = 1;
    }
    else if ((ColData & B00000001) == 0) {   // column 1: "3"
      key = 3;
    }
    else if ((ColData & B00000010) == 0) {   // column 2: "5"
      key = 5;
    }
    else if ((ColData & B00000100) == 0) {   // column 3: "7"
      key = 7;
    }
    else if ((ColData & B00001000) == 0) {   // column 4: "9"
      key = 9;
    }
    /*
     * The even numbers are on row 3, the odd numbers are on row 1
     * NumOdd is set by the ISR
    */
    if (NumOdd == 0) {                       // the even number are higher by one
      key++;
    }
    if (key == 10) {                         // in case it is a "0" 
      key = 0;
    }
    Serial.print("Key ");
    Serial.print(key);
    Serial.println(" detected.");
    if ((key > 0) && (key < KMax+1) && (RestoreKey == 1)) {       // restore pressed and key in range
       KNumber = key;                                             // set Kernal Number
       EEPROM.write(EEPROMAddr, KNumber);                         // write it to the EEPROM
       KAddr = KNumber - 1 + KOffset;                             // Calculate the address bits. 
                                                                  // For long boards, it should be between 0 and 7,
                                                                  // for short boards, it should be 1 to 7, because the first 8k are occupied by the BASIC ROM. 
       inactive = 0;                                              // every valid kernal leaves inactive state
       EEPROM.write(EEPROMAddr+1, inactive);                      // write it to EEPROM
       SwitchKernal();                                            // Switch Kernal Number and reset C64
    }
    else if ((key == 0) && (RestoreKey == 1)) {                   // key "0" switches to kernal #1 and enters inactive state
      KNumber = 1;                                                // inactove means, that the initial key scan timeout is inactive
      EEPROM.write(EEPROMAddr, KNumber);                          // which is useful for catridges, that do not perform a keyboard scan
      KAddr = KNumber - 1 + KOffset;
      inactive = 1;                                               // enter inactive state
      EEPROM.write(EEPROMAddr+1, inactive);                       // write it to EEPROM
      Serial.println( "Kernal 1 and scan timeout inactive." );     // send to serial
      SwitchKernal();                                             // Switch Kernal Number and reset C64
    }
    else {
      key = 128;                                                  // reset the key number
    }
    NumPressed = 0;                                               // reset the flag, so the ISR can process key matrix again
  }
  else {
    key = 128;                                                    // reset the key number
  }
  delayMicroseconds( 1000 );                                       // delay 1ms per loop
}
