#include <FlexiTimer2.h>

float stepSum_0;
float stepSum_1;
bool isPulse_0;
bool isPulse_1;
bool debugFlag;

int currentStep_0;
int currentStep_1;
int targetStep_0;
int targetStep_1;
double speed_0 = 0.0;
double speed_1 = 0.0;

bool Dir_0;
bool Dir_1;
bool isLimSwitch;
bool isFirstTouch;
bool isHoming;

// for serial command
char cmd[6][64];
char incommingByte[64];

void stepper()
{

  // set direction
  digitalWrite( 6, Dir_0 );
  digitalWrite( 8, Dir_1 );


  // unlock homing mode
  if( isHoming )
  {
    if( currentStep_0 == targetStep_0 )
    {
      isHoming = false;  
    }  
  }



  // if limswitch is ON, stop tilt-up 
  float coef;
  if( Dir_0 == true && isLimSwitch )
  {
    coef = 0.0;
  }
  else
  {
    coef = 1.0;
  }



  // add variables
  stepSum_0 += speed_0*coef; // for tilt
  stepSum_1 += speed_1; // for pan


  if( stepSum_0 > 1.0 )
  {
    stepSum_0 -= 1.0;
    isPulse_0 = !isPulse_0;

    if( isPulse_0 && ( currentStep_0 != targetStep_0 ))
    {
      digitalWrite( 5, HIGH );
      if(Dir_0){ currentStep_0++;}
      else{ currentStep_0--;}
    }
    else if( !isPulse_0 && (currentStep_0 != targetStep_0 ) )
    {
      digitalWrite( 5, LOW );
    }
  } // stepSum_0 > 1.0


  if( stepSum_1 > 1.0 )
  {
    stepSum_1 -= 1.0;
    isPulse_1 = !isPulse_1;

    if( isPulse_1 && (currentStep_1 != targetStep_1 ))
    {
      digitalWrite( 7, HIGH );
      if(Dir_1){ currentStep_1++; }
      else{ currentStep_1--; }
    }
    else if( !isPulse_1 && (currentStep_1 != targetStep_1 )) 
    {
      digitalWrite( 7, LOW );
    }
  }// stepSum_1 > 1.0
}// stepper()





void setup() {

  // disable motor before serial open;
   digitalWrite( 2, HIGH ); // ENABLE ( active at low )

  Serial.begin(9600);


  // init variable
  stepSum_0 = 0.0;
  stepSum_1 = 0.0;

  isPulse_0 = false;
  isPulse_1 = false;
  isLimSwitch = false;
  isFirstTouch = true;
  isHoming = false;

  debugFlag = false;
  
  currentStep_0 = 0;
  currentStep_1 = 0;
  targetStep_0 = 0;
  targetStep_1 = 0;
  speed_0 = 0.0;
  speed_1 = 0.0;

  // set pin mode
  pinMode( 2, OUTPUT ); // ENABLE
  pinMode( 3, OUTPUT ); // MS1,2,3 for Motor1
  pinMode( 4, OUTPUT ); // MS1,2,3 for Motor2
  pinMode( 5, OUTPUT ); // step for Motor1
  pinMode( 6, OUTPUT ); // dir for Motor1
  pinMode( 7, OUTPUT ); // step for Motor2
  pinMode( 8, OUTPUT ); // dir for Motor2
  
  digitalWrite( 2, LOW ); // ENABLE ( active at low )
  digitalWrite( 3, HIGH ); // ALL HIGH for 1/16 step
  digitalWrite( 4, HIGH ); // ALL HIGH for 1/16 step
  digitalWrite( 5, LOW );
  digitalWrite( 6, LOW );
  digitalWrite( 7, LOW );
  digitalWrite( 8, LOW );
  

  // set timer
  FlexiTimer2::set(1, 1.0/12500, stepper);
  FlexiTimer2::start();

}






void loop() {

  // check limit switch
  int a_5 = analogRead(5);
  if( a_5 > 500 )
  {
    isLimSwitch = true;
    if( isFirstTouch == true && isHoming == true )
    {
      // Serial.println("first touch of lim switch;");
      isFirstTouch = false;
      currentStep_0 = 1000*16;
      targetStep_0 = 0;
      decideMotorDirection();
    }  
  }
  else // lim switch off
  {
    isLimSwitch = false;
    if( isFirstTouch == false && isHoming == false )
    {
      //Serial.println("unlock first touch;");
      isFirstTouch = true;  
    }
  }


  // read serial
  int len = 0;

  // clear command-receive buffer
  for( int i = 0 ; i < 64 ; i++ )
  {
    incommingByte[i] = 0;  
  }



  // reveive command
  while( Serial.available() > 0 )
  {
     len = Serial.readBytesUntil('\0', incommingByte, 64 );
    
     if( len > 1 && !isHoming )
     {
        parseString( incommingByte );
     }
     else if( len > 1 && isHoming ) // ignore command during homing
     {
      //Serial.println("now homing, can't accept command;"); 
     }
  }




  // command processing
  if(len > 0 )
  {
    // tilt homing
    if( strcmp(cmd[0], "home") == 0 )
    {
        isHoming = true;
        targetStep_0 = 2000*16;
        currentStep_0 = 0;
        speed_0 = 0.7;
        decideMotorDirection();
        //Dir_0 = true; //tilt up
    }
    // set pan origin
    else if( strcmp( cmd[0], "setzero_pan" ) == 0 )
    {
      currentStep_1 = 0;
      targetStep_1 = 0;
    }
    // tilt movement
    else if( strcmp(cmd[0], "tilt") == 0 )
    {
      int num = atoi( cmd[1] ); // from -1000 to 1000
      num = limitRange( num );
      
      targetStep_0 = num*16;
      speed_0 = atof( cmd[2] );
      
      if( speed_0 > 1.0 ){ speed_0 = 1.0; }
      else if( speed_0 < 0.0 ){ speed_0 = 0.0; }

      decideMotorDirection();
      
    }
    // pan movement
    else if( strcmp(cmd[0], "pan") == 0 )
    {
      int num = atoi( cmd[1] ); // from -1000 to 1000
      num = limitRange( num );
      
      targetStep_1 = num*16;
      speed_1 = atof( cmd[2] );

      if( speed_1 > 1.0 ){ speed_1 = 1.0; }
      else if( speed_1 < 0.0 ){ speed_1 = 0.0;}

      decideMotorDirection();

    }
    // move both
    else if( strcmp(cmd[0], "rotate" ) == 0 )
    {
        int num_0 = atoi( cmd[1] ); // from -1000 to 1000
        int num_1 = atoi( cmd[3] );
        num_0 = limitRange(num_0);
        num_1 = limitRange(num_1);
        
        speed_0 = atof( cmd[2] );
        speed_1 = atof( cmd[4] );
        
        targetStep_0 = num_0*16;
        targetStep_1 = num_1*16;

        decideMotorDirection();
    }
    else
    {
      // unknown command
      //Serial.println("unknown command;"); 
    }
  }// if len > 0

  // clear command buffer
  for( int i = 0 ; i < 6 ; i++ )
  {
    strcpy(cmd[i], "");
  }

  
  delay(10);

 }// loop



void decideMotorDirection()
{
  if( targetStep_0 < currentStep_0 )
  { Dir_0 = false; } // tilt down
  else
  { Dir_0 = true; } // tilt up

  // pan
  if( targetStep_1 < currentStep_1 )
  { Dir_1 = false; } // pan left
  else
  { Dir_1 = true; } // pan right
}


int limitRange( int val )
{
    int returnVal = val;
    if( returnVal > 1000 )
    {
        returnVal = 1000;
    }
    else if( returnVal < -1000 )
    {
      returnVal = -1000;
    }

    return returnVal;
}


int parseString( char* str )
{
    // clear command buffer
    for( int i = 0 ; i < 6 ; i++ )
    {
      strcpy( cmd[i], "");
    }


    //divide string
    char kugiri[] = ",;";
    char* tok;
    int c = 0;

    // first split
    tok = strtok( str, kugiri );

    // loop
    while( tok != NULL )
    {
      strcpy( cmd[c], tok );
      c++;
      tok = strtok( NULL, kugiri );
    }


    return 0;
}
