/**
 * By Jan Boeyens May 2017
 */

#include <LiquidCrystal_I2C.h>

#include <DallasTemperature.h>

#include <OneWire.h>

int use_lux;
int lcdlight;

int blue_led = 9;  // Arduino Uno R3 pin number 9          


int photo_input=1;     //Analog 1 for photo resistor 
int photo_value;       // reading from the photo resistor 

int temp_input=A3;     // temperature probe used on 3 pin tmp36GT9Z  room temperature sensor   - not actually used..
int wtemp_input=7;     // digital water proof temperature pin on pin 7  used with 4.7 K ohm resistor
int Nwtemp_input=8;    // temperature of water inlet area.   // exact same probe as on pin 7
OneWire ow(wtemp_input);
OneWire Now(Nwtemp_input);
DallasTemperature Dt(&ow);
DallasTemperature NDt(&Now);

LiquidCrystal_I2C lcd(0x3F, 2, 1, 0, 4, 5, 6, 7, 3, POSITIVE);  // use the sketch_may12a to find the address ;-)

float temp,wtemp,WC;         // WC : watr temp in Celsius
float Ntemp,Nwtemp,NWC;         // WC : watr temp in Celsius

int times;                  // system loops between writes

// debug values:
//long int on_time=5000;
//long int off_time=65000;

// live values:
long int on_time=75000;               //miliseconds 1 min 15 seconds
//long int off_time=650000;   // 10 and a halfminutes
 long int off_time=1800000;  //half an hour
// long int off_time=60000;  // one minute

//int sun_brightness=150;  //direct sunlight produces about 190 + with a 100ohm resistor
                         // prototype used 1K ohm - this value=700 

                        // Whereas before full direct sunlight would see this go over 200,
                        // in January 2018 it now barely passes 145...

//int sun_brightness=120;  //9/1/2018
                         
//int sun_brightness=90;  //19/2/2018  // else it literally never comes on
int sun_brightness=70;  // it seems summer sun is way weaker than in winter.
                        // it sits in direct sun at 83 or so...  2/3/2018

int temp_diff_trigger=5;  // 6degree celsius  
                          // stays on while in temp is x  degree over ambient tank

int heater_status;  /* if in auto mode - is it on or off;
                   0 off because of overheat
                   1 off because its dark
                   2 off waiting to go on
                   3 on waiting to go off
*/
long int current_time;         //time since last timer started  runs up to either on_time or off_time          


int green_led=10;              // LED pin
int red_led=11;                //LED pin  with  100Ohm

int motor_pin=6;                // Blue LED
int green_button=4;             // Pin for Green button
int red_button=3;               // was 2 Pin for Off button
int blue_button=5;              // Pin for auto button

int button;  // 0 both off 1 red on 2 green on  3 blue on in auto mode
int changed;   // the motor needs to be switched on or off
boolean motor;  // true on false off



// stuff for checking if the temperature got stuck. i.e. the heater is not getting cooled down

int prev_temp,repeats, new_temp_diff_trigger;


// the setup routine runs once when you press reset:
void setup() {
  // initialize serial communication at 9600 bits per second:
  Serial.begin(9600);
    pinMode(green_button, INPUT);
    pinMode(red_button, INPUT);
    pinMode(blue_button,INPUT);
    pinMode(green_led, OUTPUT);
    pinMode(red_led, OUTPUT);
    pinMode(motor_pin,OUTPUT);

     pinMode(temp_input,INPUT);

    pinMode (blue_led,OUTPUT);
  button=3;  //start in auto made
  changed=1;
  use_lux=1;
  motor=false;
  heater_status=2;  // off because of poor light..
  current_time=off_time-60000;  //start in 1 minute if everything checks out.

     temp=0;     //Temperature reading 
     times=0;    // number of loops completed during current cycle
     lcdlight=LOW;

     reset_temp_repeater();
     
    lcd.begin(20,4); //Start the lcd and define how many rows and characters it has. 20 characters and 4 rows.

    lcd.backlight();//Start the backlight 
    lcd.setBacklight(lcdlight);
    operate_motor();
}

// the loop routine runs over and over again forever:
void loop() {

   check_buttons();

  show_leds();

   if(button==2){  check_temperature();
                     }
   
   if(button==3 || button==1){  // we are in auto mode
                check_light();
                check_temperature();
              
               }

  check_delays();
  operate_motor();
}






void check_buttons(){

  int check;

changed=0;
/* disabling checking of red button:
if(button==3 || button==2) {  // auto or green
    check=digitalRead(red_button);
    if(check==HIGH){ // it has  been pressed
      use_lux=0;
      changed=1;

      if(motor==false){ current_time=off_time-60000; heater_status=2;}
      if(motor==true){ current_time=on_time-60000; heater_status=3; }
      button=1;
    }
    }
*/

/* old off code:
    Serial.print("Red");  
      button=1;   // switch red to on.
      changed=1;
      motor=false;
    }*/
     

if(button==3 || button==1){  // auto or red
   check=digitalRead(green_button);
    if(check==HIGH){ // it has  been pressed
      Serial.print("green");
      button=2;   // green is on.
      changed=1;
      motor=true;
      use_lux=1;
    }}

if(button==1 || button==2){  // off or on
  check=digitalRead(blue_button);
    if(check==HIGH){ // it has  been pressed
     Serial.print("blue");

     changed=1;
      //leave motor as is for one minute
      if(motor==false){ current_time=off_time-60000; heater_status=2;}
      if(motor==true){ current_time=on_time-60000; heater_status=3; }
      button=3;
      use_lux=1;
    }}

if(button==3 && changed==0){  // in auto mode - pressing blue
  check=digitalRead(blue_button);
   Serial.print(check);
    if(check==HIGH){ // it has  been pressed while active
 //     Serial.print(check);
     if(lcdlight==HIGH) lcdlight=LOW;
     else lcdlight=HIGH;

     lcd.setBacklight(lcdlight);
    }
} 

if(button==1 && changed==0){  // in lux off auto mode - pressing red
  check=digitalRead(red_button);
   Serial.print(check);
    if(check==HIGH){ // it has  been pressed while active
      Serial.print(check);
     if(lcdlight==HIGH) lcdlight=LOW;
     else lcdlight=HIGH;

     lcd.setBacklight(lcdlight);
    }
} 


  if(changed==1){
 //   Serial.print("button: ");
 //   Serial.println(button);
 //   show_status();
     delay(500);
  }

}

 
  

/**
 * The red and green ones.
 */
void show_leds(){  


     switch(button){
          case 1:
            digitalWrite(red_led,LOW);
            digitalWrite(green_led,HIGH);
            digitalWrite(blue_led,HIGH);
            break;
          case 2:
            digitalWrite(red_led,HIGH);
            digitalWrite(green_led,LOW);
            digitalWrite(blue_led,HIGH);
            break;
          case 3:
            digitalWrite(red_led,HIGH);
            digitalWrite(green_led,HIGH);
            digitalWrite(blue_led,LOW);
            break;
            
          default:  // 0
            digitalWrite(red_led,HIGH);
            digitalWrite(green_led,HIGH);
            digitalWrite(blue_led,HIGH);
            
            break;
        }
}


void operate_motor(){

  if(changed==1){
     switch(motor){
            case true:  //  - switch motor on:
                  digitalWrite(motor_pin,LOW);
                  break;
            case false:  //off
              digitalWrite(motor_pin,HIGH);
               break;
     }   
     show_status(); 
  }
}

// blue light:
void reset(){
  if(motor==true) heater_status=3;
  if(motor==false) heater_status=2;
                 /* if in auto mode - is it on or off;
                   0 and 1 off staying off
                   2 off waiting to go on
                   3 on waiting to go off
*/
 current_time=0;                   
}


  
void    check_temperature(){

  float reads=5;
  
   
        float C;    
        int tmp,wtmp,Nwtmp;
        tmp=analogRead(temp_input);    // reading the room temperature..

        Dt.requestTemperatures();
        wtmp=Dt.getTempCByIndex(0);
          //Serial.println(wtmp);
           NDt.requestTemperatures();
        Nwtmp=NDt.getTempCByIndex(0);
        

      
      if(times<reads){  // supposedly the temperature readings need to be averaged...
      
          if(wtmp>5)    // you get random shit happening here like -30 !
              // temp+=tmp;
                wtemp+=wtmp;  // else use what we had before
                else wtemp+=WC;
                
           if(Nwtmp>5)     
                Nwtemp+=Nwtmp;
             else Nwtemp+=NWC;  
                
          times++;
        }

        
              if(times==reads){   // we have enough readings
 //                 C=temp/reads;
 //                 C=(C*5)/1024;
 //                 C=(C-0.50)*100.0;
              //     Serial.print("Average Temperature: ");
              //     Serial.println(C);
 
                //Water temperature - the relevant stuff:    
                    WC=wtemp/reads;
                    NWC=Nwtemp/reads;

                              //check_photo();
                            //     Serial.print("Average Water Temperature: ");
                            //     Serial.println(WC);
 
                            //     Serial.print("Photo value: ");
                            //    Serial.println(photo_value);


      if(button==3 || button==1){

                if( WC>30){ //switch off heating...

                    motor=false;
                    if(heater_status!=0)
                            changed=1; 
                    heater_status=0;
        
              }else{  // cool enough to need heating

                  if(heater_status==0){
              //start heating again
                       motor=false;
                      changed=1;
                      heater_status=2; 
                      reset();
                   }
              }
         
                 
             
  
      }
      show_status();
   
      
     
     times=0;
     temp=0;
     wtemp=0;
     Nwtemp=0;
  }
  
}
  
void check_light(){

      

     photo_value=analogRead(photo_input);
     if(heater_status==0)  return;   // if its too hot - that is the main issue


                   //    Serial.print("Photo: ");
                  //  Serial.println(photo_value);
                 //    Serial.print("Heater:  ");
                  //    Serial.println(heater_status);
     

 if(photo_value>sun_brightness || use_lux==0){
  
     if(heater_status==1){  // if its off because of no light 
                            // start a timer to switch it on
                           motor=false;
                           reset();
                           }   
         
 
      if(heater_status==3) return;  // on stay on
      if(heater_status==2) return;  // off carry on waiting to go on

    }else{
        if(heater_status==1) return; // its off because of low light - nothing chnages
        if(heater_status==2){ motor=false;  heater_status=1; changed=1; }// restart the wait..
        if(heater_status==3){  return; // its on - let it run untill the water is cold //motor=false; changed=1;  // its on - terminate it - and set to off because of low light.
                                         //heater_status=1;
                               }
           
       }

      

      }



void check_delays(){

 
  delay(50);            // this is also good for other buttons a little pause inbetween reads.
  if(button==3 || button==1){  // if in auto mode
  if(heater_status==0 || heater_status==1) return;     //its off because of low light or high temperature
 
  current_time+=250;  //processor time added..
  
    switch(heater_status){
           case 2:   // off waiting to go on
                   if(current_time>=off_time){
                      changed=1;  motor=true; reset();
                   }
                   break;
            case 3:           // on waiting to go off:
                   if(current_time>=on_time){
                      if(NWC==prev_temp){ // remained the same:
                        repeats++;
                        if(repeats==6){  // been stuck on one temp for a minute
                          repeats=0;
                          new_temp_diff_trigger++;  // increase the diff trigger by a degree
                        }
                      }else repeats=0;


                      if(!(NWC>WC+new_temp_diff_trigger)){//                   
                      changed=1;  motor=false; reset();
                      reset_temp_repeater();
                   }else {// else carry on untill the incominmg water is cold
                      current_time-=10000;  // run some more
                   }
                   prev_temp=NWC;
                   } 
                   break;

            
                   
    }
  /* debug:
       Serial.print("Motor: ");
      Serial.println(motor);

        Serial.print("current_time: ");
      Serial.println(current_time);
      */
  }
}
  

void show_status(){




 switch(button){
  /*
                 case 1: 
                          lcd.clear();
                          lcd.setCursor(0,2);
                          lcd.print("System is OFF");
                         Serial.println("System is switched OFF");
                           lcd.setBacklight(LOW);//Start the backlight 
                         break;
                         */
                 case 2: Serial.println("System is switched to override ON");
                          read_aan_temps();
                          lcd.setBacklight(HIGH);
                          lcd.clear();
                          lcd.setCursor(0,1);
                          lcd.print("Nou is hy AAN");

                    lcd.setCursor(0,2); //Start the text in the first row at the first character
                     lcd.print("Water temp: "); 
                     lcd.print(WC);
        
                     lcd.setCursor(0,3); //Start the text in the first row at the first character
                     lcd.print("Inlet temp: "); 
                     lcd.print(NWC);
  
                 
                         break;          

                 case 3:
                 case 1:
                 
                    {  // blue:   
                    if(changed==1) lcd.clear();
                     lcd.setCursor(0,0); //Start the text in the first row at the first character
                     lcd.print("Water temp: "); 
                     lcd.print(WC);
                     lcd.setCursor(0,1); //Start the text in the first row at the first character
                     if(button==3){
                     lcd.print("Photo Index: "); 
                     lcd.print(photo_value);
                     lcd.print("   ");
                     }else{
                      lcd.print("Photo Index ignored");
                     }

                     lcd.setCursor(0,3); //Start the text in the first row at the first character
                     lcd.print("Inlet temp: "); 
                     lcd.print(NWC);


  
 
  switch(heater_status){

       case 0:  Serial.print("Off temperature too high: ");
                Serial.println(WC);
                
                lcd.setCursor(0,2);
                lcd.print("Off - temp too high"); 
                                       break;
       case 1:  Serial.print("Off light too low: ");
                Serial.println(photo_value);      
                lcd.setCursor(0,2);
                lcd.print("Off - light too low"); 

                                      break;
       case 2: // Serial.print("Off - switching on in: ");
              //  Serial.println(off_time-current_time);
                lcd.setCursor(0,2);
                lcd.print("Switching on in:   "); 
                lcd.setCursor(0,3);
                lcd.print(get_minutes((off_time-current_time)/1000));
                lcd.print(" min ");
                lcd.print(get_seconds((off_time-current_time)/1000));
                lcd.print(" seconds    "); 

                                      break;
       case 3: // Serial.print("On - switchin off in: ");
              //  Serial.println(on_time-current_time);
                lcd.setCursor(0,2);
                lcd.print("off :"); 
                lcd.print(get_minutes((on_time-current_time)/1000));
                lcd.print(" min ");
                lcd.print(get_seconds((on_time-current_time)/1000));
                
                lcd.print(" secs  "); 
                 lcd.setCursor(0,3); //Start the text in the first row at the first character
                     lcd.print("Inlet: "); 
                     lcd.print(NWC);
                     lcd.print(" / ");
                     lcd.print(WC+new_temp_diff_trigger);

  }                     
                                     
                                                                
                         
break; }

  
  }
}


int get_minutes(long int val){
      if(val<60) return 0;
  return val/60;
}


int get_seconds(long int val){

   int min,sec;
   min=get_minutes(val);

        sec=val-(min*60);
    return sec;

}

  
void read_aan_temps(){

     Dt.requestTemperatures();
        WC=Dt.getTempCByIndex(0);
          //Serial.println(wtmp);
           NDt.requestTemperatures();
        NWC=NDt.getTempCByIndex(0);
}

void reset_temp_repeater(){
  prev_temp=0;
  repeats=0;
  new_temp_diff_trigger=temp_diff_trigger;
  
}

