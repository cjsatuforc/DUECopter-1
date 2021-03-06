/*
The MIT License (MIT)

Copyright (c) 2015 Francis Newton J. Ybanez

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.

Created by Francis Newton J. Ybanez <fnjy@hotmail.com>

DUECOPTER
Processor: Supports Atmel SAM3X8E ARM Cortex-M3 CPU 32-bit a 84 MHz clock, Arduino DUE
Sensor: MPU 6050
*/
#define Divider_100HZ 4
#define Divider_50HZ 8
#define Divider_20HZ 20
#define Divider_10HZ 40
#define Divider_5HZ 80
#define Divider_2HZ 200
#define Divider_1HZ 400

static float Output_YMax=150.0, Output_YMin=-150.0;
static float Output_XMax=150.0, Output_XMin=-150.0;

float  OutputAtt_y, OutputGyr_y, OutputGyr_x, OutputAtt_x;
#include "Arduino.h"
#define M1 0 //Pin  34  (Not used)
#define M2 1 //Pin  36  (Not used)
#define M3 2 //Pin  38  (Not used)
#define M4 3 //Pin  40  (Not used)
#define M5 4 //Pin  9  Actual Motor 1 RF1 FRONR RIGHT QUAD X
#define M6 5 //Pin  8  Actual Motor 2 LF2 FRONT LEFT  QUAD X
#define M7 6 //Pin  7  Actual Motor 3 LB3 BACK  LEFT  QUAD X
#define M8 7 //Pin  6  Actual Motor 4 RB4 BACK  RIGHT QUAD X
#define MINPULSE 1100
#define MAXPULSE 2000
#define PWM_PERIOD 2500 

volatile byte numberOfMotors = 4;
int motorCommand[8] = {0,0,0,0,0,0,0,0};  
  
void initializeMotors(byte numbers = 4);
void writeMotors();


/**********RC Definition**************/
#define USE_PPM 0 
volatile uint16_t PPM[16];
/*********IMU Definition**************/
#include "Wire.h"
#include "Math.h"
#include "I2Cdev.h"
#include "MPU6050_6Axis_MotionApps20.h"
MPU6050 mpu;
#define LED_PIN 13 


float last_x_angle=0; 
float last_y_angle=0;
float last_z_angle=0;  
float prev_gyro_x_angle =0;  
float prev_gyro_y_angle =0;
float prev_gyro_z_angle =0;




//  Use the following global variables 
//  to calibrate the gyroscope sensor and accelerometer readings
float    x_gyro = 0;
float    y_gyro = 0;
float    z_gyro = 0;
float    x_accel = 0;
float    y_accel = 0;
float    z_accel = 0;

float   Gyr_Factor;//scale gyroscope data
float   Acc_Factor;//scale acclerometer data

// Variables to store the values from the sensor readings
int16_t ax, ay, az;
int16_t gx, gy, gz;

// Buffer for data output
char dataOut[256];

//ALL MY DEFINITIONS:
int Setpoint=0, n=0;  byte DelimRc, Delim; boolean ESC_ARMED=false, statusESC=false;

/*IMU STABILIZATION VARIABLES */
float Acc_x, Acc_y, Acc_z;
float Gyr_x, Gyr_y, Gyr_z;
float Rol_x, Pit_y, Yaw_z;
float  Mag_z; float Z; 

/*Define RATE and ATTITUDE Mode by stick movement*/
bool AI_Rate=false, EL_Rate=false;

/*PID VARIABLES*/
float  Output_X, Output_Y;  volatile int Output_Z;
//Attitude Variable
float Att_error_X=0, Att_errSum_X=0, Att_lastErr_X=0, Att_dErr_X=0; 
float Att_error_Y=0, Att_errSum_Y=0, Att_lastErr_Y=0, Att_dErr_Y=0;
float error_Z=0, errSum_Z=0, lastErr_Z=0,dErr_Z=0;
//Rate/Gyro Variable

float Gyr_error_Y=0, Gyr_errSum_Y=0, Gyr_dErr_Y=0, Gyr_lastErr_Y=0;
float Gyr_error_X=0, Gyr_errSum_X=0, Gyr_dErr_X=0, Gyr_lastErr_X=0;
float Gyr_error_Z=0;

//Time Variable
volatile unsigned long  lastTime=0, now=0; 
volatile unsigned long  dt_loop=0; 
float SetPitchAbout_Y=0, SetRollsAbout_X=0, SetRuddrAbout_Z=0, SetRuddrInput=0; 
boolean StickCenter=true;
//http://www.w//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//http://www.w//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//http://www.w//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//http://www.w//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//Variable P Limit
float kxlim=16.0, kylim=16.0; 
//Variable P
float kxf=0.12, kyf=0.12, fkgx=0.00, fkgy=0.00;
//Integral Limit
float IaxLimPos=2.0, IaxLimNeg=4.0, IayLimPos=2.0, IayLimNeg=2.0; 
//Stick Limit
float mapxlim=100, mapylim=120;

float  kpy=1.80,   kiy=kpy*0.0001,     kdy=kpy*1.00,     gkpy=0.0,   gkiy=gkpy*0.10,   gkdy=0/6.0;
float  kpx=1.80,   kix=kpx*0.0001,     kdx=kpx*1.45,     gkpx=0.0,   gkix=gkpx*0.10,   gkdx=0/6.0; 
float  kpz=4.00,   kiz=kpz*0.0000,     kdz=kpz*1.45,     gkpz=0.00,  gkiz=0.00,        gkdz=0/15.0;
//http://www.w////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//http://www.w////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//http://www.w////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//http://www.w////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

float Th=0, Ave=0;
void setup() 
{
 pinMode(16, OUTPUT);
//Serial Initialization
Serial.begin(115200); Serial2.begin(115200); delay(500);//give time for serial to initialized 
while (Serial.available() && Serial.read()); delay(500);// empty buffer 

initializeMotors(numberOfMotors);
Motor_Controll_All(MINPULSE);


    Wire.begin();

    while (!Serial); // wait for Leonardo enumeration, others continue immediately


    // initialize device
    Serial.println(F("Initializing I2C devices..."));
    mpu.initialize();

    // verify connection
    Serial.println(F("Testing device connections..."));
    Serial.println(mpu.testConnection() ? F("MPU6050 connection successful") : F("MPU6050 connection failed"));

        // Set the full scale range of the gyro
        uint8_t FS_SEL = 0;
        //mpu.setFullScaleGyroRange(FS_SEL);


        uint8_t READ_FS_SEL = mpu.getFullScaleGyroRange();
        Serial.print("FS_SEL = ");
        Serial.println(READ_FS_SEL);
       Gyr_Factor = 200.0/(FS_SEL + 1);
        

        uint8_t AFS_SEL = 0;
        mpu.setFullScaleAccelRange(AFS_SEL);
        
        uint8_t READ_AFS_SEL = mpu.getFullScaleAccelRange();
        Serial.print("AFS_SEL = ");
        Serial.println(READ_AFS_SEL);


    calibrate_sensors();

setupInput(USE_PPM);


// Configure LED Indicator
pinMode(LED_PIN, OUTPUT);
digitalWrite(LED_PIN, LOW);
pinMode(52, OUTPUT);
digitalWrite(52, LOW);
}

volatile float AI_Pulse, EL_Pulse, TH_Pulse, RD_Pulse;
volatile float RF1, LF2, LB3, RB4;
float Zdrift=0, OutputGyr_z,Acc_Zprev; uint16_t pulseCounter=0; 
void loop() 
{digitalWrite(LED_PIN,HIGH);
   now=micros();
   dt_loop = (now- lastTime);
       
    if(dt_loop <= 0){lastTime = 2501;}   
    if (dt_loop >= 2500)
       {//Start 400 Hz
          digitalWrite(52, HIGH);
          lastTime = micros();  pulseCounter++;
          loopIMU(dt_loop); //Extract IMU 
 
              //Capture RC Receiver @ 50 Hz (20 ms)  Divider_50HZ=50Hz/2.5ms = 8 
              //For 50Hz(20ms) there are 8 of 2.5ms pulse in 20ms
              if (pulseCounter % Divider_50HZ == 0) 
               {//Start 50Hz loop
                 smoothRCinput();   //Extract RC 
               }//End 50Hz loop  
          Compute_Elevator(dt_loop);   // Calculate for Output_Y
          Compute_Aileron(dt_loop);    // Calculate for Output_X
          Compute_Rudder(Acc_z, dt_loop);     // Calculate for Output_Z        
          loopMotor(); // Calculate Value for Motor PWM input to writeMotors();
          writeMotors(); //PWM Motor Output     
          digitalWrite(52, LOW);
        /*
          Serial.print(Gyr_error_Z);  
          Serial.print('\t');
          Serial.print(OutputGyr_z);  
          Serial.print('\t');
          Serial.println("."); */ 
 
     }//End 400 Hz loop
digitalWrite(LED_PIN,LOW);     
}//End Main Loop


/****************************************************************************/
/************************(RC PPM RECEIVER FONBCTIONS)*****************************/
/****************************************************************************/
void setupInput(int isPPM)
{
  pmc_enable_periph_clk(ID_TC1);


    TC_Configure(TC0, 1, TC_CMR_TCCLKS_TIMER_CLOCK1);
    TC_Start(TC0,1);
    attachInterrupt(46,pwmINT1,CHANGE);
    attachInterrupt(47,pwmINT2,CHANGE);
    attachInterrupt(48,pwmINT3,CHANGE);
    attachInterrupt(49,pwmINT4,CHANGE);

}

void pwmINT1() { pwmSet(2, 46); }
void pwmINT2() { pwmSet(3, 47); }
void pwmINT3() { pwmSet(4, 48); }
void pwmINT4() { pwmSet(5, 49); }


uint32_t pwmPrev[8];
void pwmSet(uint8_t ch, uint32_t pin) {
  uint32_t cv = TC0->TC_CHANNEL[1].TC_CV;
  if (digitalRead(pin)) {
    pwmPrev[ch] = cv;
  } else {
    PPM[ch] = (cv - pwmPrev[ch]) / 42;
  }
}


float emaX=0, emaY=0, emaZ=0, emaXg=0, emaYg=0, emaZg=0;
float Gyr_xp=0, Gyr_yp=0, Gyr_zp=0;float get_last_time=0;
const float alphax = 0.121; //0 to 1
void loopIMU(unsigned long  dt_imu)
{ float dt=dt_imu*0.000001; //micro to seconds
  

const float RADIANS_TO_DEGREES = 57.2958; //180/3.14159
        mpu.getMotion6(&ax, &ay, &az, &gx, &gy, &gz);
        unsigned long t_now = millis();
        

        double axx = ax ;//Serial.print(ax); Serial.print("\t");
        double ayy = ay ;//Serial.print(ay); Serial.print("\t");
        double azz = az ;//Serial.println(az);              
        //Normalise the measurements
        double R = sqrt(axx*axx + ayy*ayy + azz*azz); //Serial.println(R/16384);// R^2 = Rx^2 + Ry^2 + Rz^2  Pythegorian  

        float  Ax = axx/R ;//Serial.print(axx); Serial.print("\t");
        float  Ay = ayy/R ;//Serial.print(ayy); Serial.print("\t");
        float  Az = azz/R ;//Serial.println(azz);
          
        // Remove offsets and scale gyro data  
        float gyro_x = (gx - x_gyro)/Gyr_Factor;
        float gyro_y = (gy - y_gyro)/Gyr_Factor;
        float gyro_z = (gz - z_gyro)/Gyr_Factor;
        float accel_x = Ax; // - x_accel;
        float accel_y = Ay; // - y_accel;
        float accel_z = Az; // - z_accel;
        

              
        float Acc_Angle_y = atan(-1*accel_x/sqrt(pow(accel_y,2) + pow(accel_z,2)))*RADIANS_TO_DEGREES;
        float Acc_Angle_x = atan(accel_y/sqrt(pow(accel_x,2) + pow(accel_z,2)))*RADIANS_TO_DEGREES;
        float Acc_Angle_z = 0;

        // Compute the (filtered) gyro angles
        
        float Gyr_Angle_x = gyro_x*dt + last_x_angle;
        float gyro_angle_y = gyro_y*dt + last_y_angle;
        float Gyr_Angle_z = gyro_z*dt + last_z_angle;
                           
         // Apply the complementary filter to figure out the change in angle - choice of alpha is
        // estimated now.  Alpha depends on the sampling rate...
        const float alpha = 0.98;
        float angle_x = alpha*Gyr_Angle_x + (1.0 - alpha)*Acc_Angle_x; last_x_angle = angle_x; 
        float angle_y = alpha*gyro_angle_y + (1.0 - alpha)*Acc_Angle_y; last_y_angle = angle_y;
        float angle_z = Gyr_Angle_z; last_z_angle = angle_z;  //Accelerometer doesn't give z-angle
        
        // Compute the drifting gyro angles
        float UGyr_angle_x = gyro_x*dt + prev_gyro_x_angle ; prev_gyro_x_angle  = UGyr_angle_x;
        float UGyr_angle_y = gyro_y*dt + prev_gyro_y_angle ; prev_gyro_y_angle  = UGyr_angle_y;
        float UGyr_angle_z = gyro_z*dt + prev_gyro_z_angle ; prev_gyro_z_angle  = UGyr_angle_z;     
       
        

///////////////////////////////////////////END IMU DATA WXTRACTION///////////////////////////////////////////////

////////////////////////////////////START IMU DATA USAGE AND MANUPULATION////////////////////////////////////////

                    Acc_x= angle_x+0.00; Acc_x=xEMA(Acc_x);///Yaw
                    Acc_y=-angle_y+0.00; Acc_y=yEMA(Acc_y);//Pitch  (-)FORWARD   (+)BACKWARD
                    Acc_z=-angle_z;      Acc_z=zEMA(Acc_z); //RollS  (+)ROLLRIRGT (-)ROLLLEFT
                                       
                    if       (Acc_x>45)  {Acc_x=45;}
                    else if  (Acc_x<-45) {Acc_x=-45;}
                    else     {Acc_x=Acc_x;}
                    
                    if       (Acc_y>45)  {Acc_y=45;}
                    else if  (Acc_y<-45) {Acc_y=-45;}
                    else     {Acc_y=Acc_y;}  
                    
                    
                    Gyr_x=gyro_x; //Rolls Gyro + Rigt    -Left about X (See Axis top page)
                    Gyr_x=gxEMA(Gyr_x);
                    Gyr_y=-gyro_y; //Pitch Gyro + Forward -Backward about Y (See Axis top page)                          
                    Gyr_y=gyEMA(Gyr_y);
                    Gyr_z=gyro_z; //Pitch Gyro -CW +CCW about Z (See Axis top page)                     
                    Gyr_z=gzEMA(Gyr_z);                                         

                   
                    /*DISPLAY*/
                     Serial.print(Acc_x);
                     Serial.print("\t");
                     Serial.print(Acc_y);
                     Serial.print("\t"); 
                     Serial.print(Acc_z);                    
                     Serial.println();Serial.flush(); 

////////////////////////////////////END IMU DATA USAGE AND MANUPULATION////////////////////////////////////////       
}


void smoothRCinput()
{
  AI_Pulse=PPM[3];
  EL_Pulse=PPM[2];
  TH_Pulse=PPM[4];
  RD_Pulse=PPM[5];  
}


void loopMotor()
{
  //1 deg = 0.0174532925 rad
  float Ax=Acc_x*0.0174532925; 
  float Ay=Acc_y*0.0174532925; 
  float ThX=(TH_Pulse/cos(Ax))-TH_Pulse;
  float ThY=(TH_Pulse/cos(Ay))-TH_Pulse;
  Th=sqrt(sq(ThX)+sq(ThY));
  if (Th>10) {Th=10.0;}
  float zTh=abs(Gyr_z);
  if (zTh>10) {zTh=10.0;}

   float Throttle=0+0+TH_Pulse;   
    //FRONT MOTOR
    RF1 = Throttle + (-Output_Y)  + (+Output_X) + (-Output_Z) -  0.0;
    LF2 = Throttle + (-Output_Y)  + (-Output_X) + (+Output_Z) +  0.0;
    
    //BACK MOTOR 
    RB4 = Throttle + (+Output_Y)  + (+Output_X) + (+Output_Z) -  0.0;
    LB3 = Throttle + (+Output_Y)  + (-Output_X) + (-Output_Z) +  0.0;
    
    Ave = ( RF1 + LF2 + RB4 + LB3 ) * 0.25;
    //Serial.println(Throttle);
}



//PID Calculation Elevator  
float Iay=0,Igy=0; 
void Compute_Elevator(unsigned long dty)
{float SetPitchAbout_Yg; float erRangeY=0.10, kyg=0;  bool CenterY; 
dty =dty*0.001; 
  //ERROR BY STICK COMMAND
  if (EL_Pulse>1505|| EL_Pulse<1490)
  {EL_Rate=true;
    SetPitchAbout_Y = -map(EL_Pulse,1495,1940,0,85); // rolls output range 0 - 8 degrees  "AI_Pulse"  
  }else {SetPitchAbout_Y=0; EL_Rate=false;}

   if   (Acc_y<=erRangeY && Acc_y>=-erRangeY) {Acc_y=0; CenterY=1;}
   else if   (Acc_y>erRangeY)  {Acc_y=Acc_y-erRangeY; CenterY=0;}   
   else if   (Acc_y<-erRangeY) {Acc_y=Acc_y+erRangeY; CenterY=0;}    

  //P.I.D Activation
   if (TH_Pulse>1180)
   {//P.I. Start only at Throttle >1170  
   
/****************************ATTITUDE****************************************/
         //INTEGRAL 
            Att_error_Y = Acc_y - SetPitchAbout_Y;
            Att_errSum_Y = Att_errSum_Y + (Att_error_Y * dty);
            Iay = kiy * Att_errSum_Y;         
            if  (Iay>=IayLimPos || Iay<=-IayLimNeg) {Att_errSum_Y=Att_errSum_Y - Att_error_Y * dty;} 
         //DERIVATIVE     
            Att_dErr_Y = (Att_error_Y - Att_lastErr_Y) / dty;
/***************************GYRO RATE****************************************/         
        //INTEGRAL
            if(EL_Rate==false) {Gyr_error_Y = Gyr_y + Acc_y;} else {Gyr_error_Y = Acc_y - SetPitchAbout_Y;}
            
            Gyr_errSum_Y = Gyr_errSum_Y + (Gyr_error_Y * dty);
            Igy = gkiy * Gyr_errSum_Y;            

         //DERIVATIVE  
            Gyr_dErr_Y = kdy*Gyr_y - (Gyr_error_Y - Gyr_lastErr_Y) /dty;                
          
/*******************************P.I.D (Y)************************************/
         //GYRO MODE
            float OutRate_y = gkpy * Gyr_error_Y +  Igy;
         //ATTITUDE MODE
          float ky1;
          kyg=fkgy*(kyg + Gyr_y);
          if (kyg>=0.00) ky1=min(kyg, 2.0); else ky1=max(kyg, -2.0);          
         
          float jy=abs(Acc_y)*kyf; float Ky=jy*jy; float my=jy+Ky;         
          float ky0=min(my, kylim);
          
          float  ky=kpy + ky0;          
          float OutAtti_y = ky * Att_error_Y +  Iay + kdy*Gyr_dErr_Y;       
         //Final Combined Output
          OutputAtt_y=OutAtti_y + OutRate_y;


/****************************OUTPUT-Y****************************************/      
      if ((OutputAtt_y <= Output_YMax) && (OutputAtt_y >Output_YMin)){Output_Y=OutputAtt_y;}
        else if  (OutputAtt_y>Output_YMax)  {Att_errSum_Y=Att_errSum_Y - Att_error_Y * dty; Output_Y=Output_YMax;}//Stop integration at limits
        else if  (OutputAtt_y<Output_YMin)  {Att_errSum_Y=Att_errSum_Y - Att_error_Y * dty; Output_Y=Output_YMin;}//Stop integration at limits

/****************************RECORD LAST READING******************************/        
        Gyr_lastErr_Y = Gyr_error_Y ; Att_lastErr_Y = Att_error_Y;
        
        
/*******************************DEBUG*****************************************/
        //Serial.print(Acc_y);Serial.print("\t");Serial.print(Iay);Serial.print("\t");Serial.print(Output_Y);Serial.println();
        //Serial.flush();
  }
   else
  {  
    Reset_Y();dty=0;
  }//End of if TH_Pulse   
}


//PID Calculation Elevator
float Iax=0,Igx=0;  float erRangeX=0.10, kxg=0;  bool CenterX; 
void Compute_Aileron(unsigned long dtx)
{dtx =dtx*0.001; 
  if (AI_Pulse>1510 || AI_Pulse<1495)
  {AI_Rate=true;
    SetRollsAbout_X = -map(AI_Pulse, 1505,1900,0,85); // pitch output range 0 - 8 degrees  "EL_Pulse"
  }else {SetRollsAbout_X=0;AI_Rate=false;}

   if   (Acc_x<=erRangeX && Acc_x>=-erRangeX) {Acc_x=0; CenterX=1;}
   else if   (Acc_x>erRangeX)  {Acc_x=Acc_x-erRangeX; CenterX=0;}   
   else if   (Acc_x<-erRangeX) {Acc_x=Acc_x+erRangeX; CenterX=0;}  

  //P.I.D Activation
    if (TH_Pulse>1180)
    {
/****************************ATTITUDE****************************************/      
          //INTEGRAL 
          Att_error_X = Acc_x - SetRollsAbout_X;         
          Att_errSum_X = Att_errSum_X + (Att_error_X * dtx);    
          Iax = kix * Att_errSum_X;                
          if  (Iax>=IaxLimPos || Iax<=-IaxLimNeg) {Att_errSum_X=Att_errSum_X - (Att_error_X * dtx);}
          //DERIVATIVE     
          Att_dErr_X = (Att_error_X - Att_lastErr_X) / dtx;
         
/***************************GYRO RATE****************************************/
          //INTEGRAL
          if(AI_Rate==false) {Gyr_error_X = Gyr_x + Acc_x ;} else {Gyr_error_X = Gyr_x - SetRollsAbout_X;}
          
          Gyr_errSum_X = Gyr_errSum_X + (Gyr_error_X * dtx);
          Igx = gkix * Gyr_errSum_X;            
        
          //DERIVATIVE 
          Gyr_dErr_X = kdx*Gyr_x - (Gyr_error_X - Gyr_lastErr_X) /dtx; 


/*******************************P.I.D (X)****************************************/
         //GYRO MODE
          float OutRate_x  = gkpx * Gyr_error_X + Igx;
         
         //ATTITUDE MODE
          kxg=fkgx*(kxg + Gyr_x);         
          float kx1;
          if (kxg>=0.00) kx1=min(kxg, 4.0); else kx1=max(kxg, -4.0);
          
          float jx=abs(Acc_x)*kxf; float Kx=jx*jx; float mx=jx+Kx;          
          float kx0=min(mx, kxlim);
          
          float  kx=kpx + kx0; //Serial.print(kx); Serial.print("\t"); Serial.print(kpx);Serial.print("\t"); Serial.println(kx0);         
          float OutAtti_x = kx * Att_error_X + Iax + Gyr_dErr_X; 
         //Final Combined Output          
          OutputAtt_x=OutAtti_x + OutRate_x;
          
/****************************OUTPUT-X ****************************************/          
      if ((OutputAtt_x <= Output_XMax) && (OutputAtt_x >=Output_XMin)){Output_X=OutputAtt_x;} 
          else if  (OutputAtt_x>Output_XMax)  {Att_errSum_X=Att_errSum_X - (Att_error_X * dtx);Output_X= Output_XMax;}//Stop integration at limits
          else if  (OutputAtt_x<Output_XMin)  {Att_errSum_X=Att_errSum_X - (Att_error_X * dtx);Output_X= Output_XMin;}//Stop integration at limits

/****************************RECORD LAST READING******************************/ 
          Gyr_lastErr_X = Gyr_error_X ; Att_lastErr_X = Att_error_X;


/*******************************DEBUG******************************************/          
          //Serial.print(Acc_x);Serial.print("\t");Serial.print(Iax);Serial.print("\t");Serial.print(Output_X);Serial.println();
          //Serial.flush();
   }
     else
     {
      Reset_X();dtx=0;
     }//End of if TH_Pulse

}




float Iaz=0; 
void Compute_Rudder(float Acc_Znow, unsigned long dtz)
{static float Output_ZMax=100, Output_ZMin=-100; 
        float OutputAtt_z, OutputGyr_z;

    if (TH_Pulse>1180)
    {
          //IDENTIFY SETPOINT AT CENTER STICK
         if (RD_Pulse>=1490 && RD_Pulse<=1515 )
          {//SETPOINT AT CENTER STICK
               if (n<1)
               {//One time Capture
                SetRuddrAbout_Z=Acc_Znow;
                Acc_Zprev = Acc_Znow; 
                Gyr_error_Z=0;error_Z=0;errSum_Z=0;
                Zdrift=0;n=+1;
                //Serial.print("I ");
               }//catch the 1st value once Stick at center range    
          
              Zdrift=Acc_Zprev-Acc_Znow;;                    
              if (abs(Zdrift)<=0.010)
              {                  
                SetRuddrAbout_Z=Acc_Znow; Acc_Zprev=Acc_Znow;n=+1; //Serial.print("J ");
                Gyr_error_Z=0;error_Z=0;errSum_Z=0;            
              }
              else {SetRuddrAbout_Z=Acc_Zprev; n=+1;//Serial.print("K ");
              }                       
           Z=0;
           error_Z = Acc_z-SetRuddrAbout_Z;
           //Serial.print("C ");
          }           
          else //SETPOINT AT MOVING STICK
          { n=0;
           Z =map(RD_Pulse,1500, 1930, 0, 50);             
           Gyr_error_Z=Z; //Gyr_z=0;           
           SetRuddrAbout_Z=Acc_z-Z; Zdrift=0; Gyr_z=0;          
           error_Z =  Acc_z-SetRuddrAbout_Z;
           //Serial.print("R ");
          }
                    
                                             
             /*Compute all the working error variables*/   
             errSum_Z = errSum_Z + (error_Z * dtz * 0.001);
             dErr_Z = (error_Z - lastErr_Z) / (dtz * 0.001); //Try Att_dErr_Y=Gyro_z for fast response;            

             Iaz = kiz * errSum_Z;           
             if  (Iaz>=10 || Iaz<=-10)  {errSum_Z = errSum_Z - (error_Z * dtz * 0.001);}            

              Gyr_error_Z = -Gyr_z+Z;        
              OutputGyr_z = gkpz * Gyr_error_Z;
              float kz=kpz+min(abs(error_Z)*0.40, 8.0);
              OutputAtt_z = (kz * error_Z + Iaz - kdz*Gyr_z)+ OutputGyr_z;
                 
              if       (OutputAtt_z>Output_ZMax)  {errSum_Z = errSum_Z - (error_Z * dtz * 0.001); Output_Z=Output_ZMax; }
              else if  (OutputAtt_z<Output_ZMin)  {errSum_Z = errSum_Z - (error_Z * dtz * 0.001); Output_Z=Output_ZMin; }
              else     {Output_Z=OutputAtt_z;}

          lastErr_Z = error_Z;  
     }
     else
     {
       Acc_Zprev=Acc_z;
       Acc_Znow=Acc_z; 
       SetRuddrAbout_Z=Acc_z; 
       error_Z=0;
       Reset_Z(); dtz=0;//Reset Yaw setpoint when motor on land to allow repositioning
     }//End of if TH_Pulse>1170
  
}//End Compute Rudder




void Reset_X()
{
   Output_X = 0 ;  
   lastTime = now;// 
   Att_lastErr_X=0;   
   Att_errSum_X=0; 
   Att_dErr_X=0; 
}



void Reset_Y()
{
   Output_Y = 0 ;  
   lastTime = now;
   Att_lastErr_Y=0;   
   Att_errSum_Y=0; 
   Att_dErr_Y=0;  
}


void Reset_Z()
{
   Output_Z = 0 ;  
   lastTime = now;// Lasttime need to be updated to avoid big gap
   lastErr_Z=0;   
   errSum_Z=0;           //Revert to 0 
   dErr_Z=0;             //Revert to 0 
}

 
float readCompass(float readNow, float SetRuddr)
 {float eRR;
   
  if (abs(readNow-SetRuddr)<=180)  
       {eRR = readNow - SetRuddr;}
   else 
        if (SetRuddr<readNow)
        {eRR = readNow - SetRuddr - 360; }
        else 
        {eRR = readNow - SetRuddr + 360; }  
   return eRR; 
  }// end readCompass



void initializeMotors(byte numbers) {

    setPWMpin(9);  //PWM L4 or  //Pin  9
    setPWMpin(8);  //PWM L5 or  //Pin  8
    setPWMpin(7);  //PWM L6 or  //Pin  7
    setPWMpin(6);  //PWM L7 or  //Pin  6
    


  pmc_enable_periph_clk(ID_PWM);

  // set PWM clock A to 1MHz
  PWMC_ConfigureClocks(1000000,0,VARIANT_MCK);
  
    config_Motor(4, PWM_PERIOD); //Pin  9
    config_Motor(5, PWM_PERIOD); //Pin  8
    config_Motor(6, PWM_PERIOD); //Pin  7
    config_Motor(7, PWM_PERIOD); //Pin  6
  
  //Start all motor with minimum pulse  
  Motor_Controll_All(MINPULSE);
}




//Sample fix PWM input for Motor
void writeMotors()
{//Start generate  fix PWM pulse @ pin  34, 36, 38, and 40
  motorCommand[M5]=LB3+0; //pin  9, Back  Left
  motorCommand[M6]=RB4+0; //pin  8, Back  Right
  motorCommand[M7]=LF2;   //pin  7, Front Left
  motorCommand[M8]=RF1;   //pin  6, Front Right



  for (int ch = 4;ch < 8; ch++) 
  {
    if (motorCommand[ch] <= 1000) { motorCommand[ch] = 1000;}
    if (motorCommand[ch] >= 2000) { motorCommand[ch] = 2000;}
  }



      PWMC_SetDutyCycle(PWM, 4, motorCommand[M5]); //pin  9,
      PWMC_SetDutyCycle(PWM, 5, motorCommand[M6]); //pin  8,
      PWMC_SetDutyCycle(PWM, 6, motorCommand[M7]); //pin  7,
      PWMC_SetDutyCycle(PWM, 7, motorCommand[M8]); //pin  6,    
}

////////////////////////////////////////////////////////////
////////////Startup Minimum Pulse for All motor/////////////
///////////////////called at void Setup()///////////////////
////////////////////////////////////////////////////////////
void Motor_Controll_All(int minPulse) 
{
      PWMC_SetDutyCycle(PWM, 4, minPulse);
      PWMC_SetDutyCycle(PWM, 5, minPulse);
      PWMC_SetDutyCycle(PWM, 6, minPulse);
      PWMC_SetDutyCycle(PWM, 7, minPulse);
}



void setPWMpin(uint32_t pin) 
{
  PIO_Configure(g_APinDescription[pin].pPort,
                PIO_PERIPH_B, 
                g_APinDescription[pin].ulPin,
                g_APinDescription[pin].ulPinConfiguration);
}

static void config_Motor(uint8_t ch, uint32_t period) {
  PWMC_ConfigureChannel(PWM, ch, PWM_CMR_CPRE_CLKA, 0, 0);
  PWMC_SetPeriod(PWM, ch, period);
  PWMC_SetDutyCycle(PWM, ch, 1000);
  PWMC_EnableChannel(PWM, ch);
}




float xEMA(float new_value) {
  emaX += alphax*(new_value - emaX);
  return(emaX);
}

float yEMA(float new_value) {
  emaY += alphax*(new_value - emaY);
  return(emaY);
}

float zEMA(float new_value) {
  emaZ += alphax*(new_value - emaZ);
  return(emaZ);
}

float gxEMA(float new_value) {
  emaXg += alphax*(new_value - emaXg);
  return(emaXg);
}

float gyEMA(float new_value) {
  emaYg += alphax*(new_value - emaYg);
  return(emaYg);
}

float gzEMA(float new_value) {
  emaZg += alphax*(new_value - emaZg);
  return(emaZg);
}


void calibrate_sensors() {
  int       num_readings = 10000; //Default 10


  mpu.getMotion6(&ax, &ay, &az, &gx, &gy, &gz);
  
  // Read and average the raw values
  for (int i = 0; i < num_readings; i++) {
    mpu.getMotion6(&ax, &ay, &az, &gx, &gy, &gz);
    x_gyro += gx;
    y_gyro += gy;
    z_gyro += gz;
    x_accel += ax;
    y_accel += ay;
    z_accel += az;
  }
  
  x_gyro /= num_readings;
  y_gyro /= num_readings;
  z_gyro /= num_readings;
  x_accel /= num_readings;
  y_accel /= num_readings;
  z_accel /= num_readings;
}
