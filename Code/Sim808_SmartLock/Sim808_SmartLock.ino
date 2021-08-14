/*
 Name:		Sim808_SmartLock.ino
 Created:	5/22/2019 11:38:19 PM
 Author:	
*/

#include <DFRobot_sim808.h>
#include <SoftwareSerial.h>  
#include "Wire.h"
#include "I2Cdev.h"
#include "MPU6050.h"


#define SOFT_RX    10
#define SOFT_TX    11


#define LED_GREEN  6
#define LED_RED    7
#define LED_BLUE   8
#define LED_YELLOW 9
#define SPK        12
#define VIBRATE    2

#define ON_OFF     A2
#define OFF_LOCK   A1
#define OFF_SPK    A0

#define SIM_PWR    3



#define PHONE_NUMBER "0582226601"
#define PHONE_NUMBER_1 "0987509513"
#define MESSAGE_DOXE  "[NGUY HIEM] Xe dang gap su co nguy hiem"
#define MESSAGE_RUNGXE  "[CANH BAO] Xe ban bi rung"
#define MESSAGE_LENGTH 160
char message[MESSAGE_LENGTH];
int messageIndex = 0;
char phone[16];
char datetime[24];

MPU6050 accelgyro;
int16_t ax, ay, az;
int16_t gx, gy, gz;

SoftwareSerial ObitSerial(SOFT_RX, SOFT_RX);
DFRobot_SIM808 sim808(&Serial);

// ham so sánh chuỗi
int obit_strcmp(uint8_t *str_real, uint8_t size_str_real, uint8_t *str_expected,uint8_t size_str_expected)
{
 int i=0;
 uint8_t next=0;
 while(i<size_str_real)
 {
   if(str_expected[0]==str_real[i])
   {
     for(int j=0; j<size_str_expected; j++)
     {
       if(str_expected[j]!=str_real[j+i]) 
       {
         i++;
         next=1;
         break;
       }
       else
         next = 0;
         //return -1;
     }
     if(next==0) return 1;
   }
   else 
   {
     i++;
     next=0;
   }
 }
 return 0;
}

char smslink[100];
// tạo link google map từ tọa độ gps đo được
void SMS_Location_googlemap(char *link)
{
 char lat[20];
 char lon[20];
 // chuyển đổi tọa độ gps thành chuỗi
 dtostrf(sim808.GPSdata.lon, 2, 6, lon);
 dtostrf(sim808.GPSdata.lat, 2, 6, lat);
 Serial.println(lon);
 Serial.println(lat);
 sprintf(link,"https://maps.google.com/maps/place/%s,%s",lat,lon);
 Serial.println(link);
}

// xử lý tin nhắn đến
void GetCMDSMS()
{
  messageIndex = sim808.isSMSunread();  // kiểm tra xem có tin nhắn mới hay không
  if (messageIndex > 0) {  // nếu có tin nhắn mới
    
      sim808.readSMS(messageIndex, message, MESSAGE_LENGTH, phone, datetime); // đọc nội dung tin nhắn
      sim808.deleteSMS(messageIndex);          // xóa tin nhắn
      //***********In order not to full SIM Memory, is better to delete it**********
      //sim808.deleteSMS(messageIndex);
      if(obit_strcmp(message,MESSAGE_LENGTH,"vitri",5)) // nếu nội dung tin là vitri => gửi tin nhắn vị trí gps
      {
        #if defined  DEBUG
        ObitSerial.println("Xu ly tin nhan vi tri");
        #endif
        sim808.sendSMS(PHONE_NUMBER,"Dang tim vi tri GPS");
        //SMS_Location_googlemap(smslink);
        sim808.sendSMS(PHONE_NUMBER,smslink);  /// send toa do gps
        sim808.sendSMS(PHONE_NUMBER_1,smslink);  /// send toa do gps
       
      }
      if(obit_strcmp(message,MESSAGE_LENGTH,"bat",3)) // tương tự, nếu bật > bật thiết bị
      {
        #if defined  DEBUG
        ObitSerial.println("Xu ly tin nhan bat");
        #endif
        digitalWrite(SPK,1); //  bat chuong canh bao
        sim808.sendSMS(PHONE_NUMBER,"Da bat chuong bao");
        //SMS_Location_googlemap(smslink);
        //sim808.sendSMS(PHONE_NUMBER,smslink);  /// send toa do gps
        
      }
      if(obit_strcmp(message,MESSAGE_LENGTH,"tat",3)) 
      {
        #if defined  DEBUG
        ObitSerial.println("Xu ly tin nhan tat");
        #endif
        digitalWrite(SPK,0); // tat chuong canh bao
        sim808.sendSMS(PHONE_NUMBER,"Da tat chuong bao");
      }

      if(obit_strcmp(message,MESSAGE_LENGTH,"tatnguon",8)) 
      {
        #if defined  DEBUG
        ObitSerial.println("Xu ly tin nhan tat nguon");
        #endif
        digitalWrite(LED_BLUE,0); // tat chuong canh bao
        sim808.sendSMS(PHONE_NUMBER,"Da tat nguon cua xe");
      }
  }
}

int vibra = 0;
void DetecVibrate() // thực hiện trong ngắt, nếu cảm biến rung thì set biến lên 1
{
  while(digitalRead(VIBRATE) == 0); // đợi cho hết rung
  delay(10);
  vibra++;
}

void Vibarte_Process() // xu ly rung
{
  if(vibra>5)  // chạm 5 lần thì bắt đầu gửi tin nhắn
  {
    digitalWrite(SPK,1);
    int i=0;
    while(i<10)
    {
      digitalWrite(LED_BLUE,0);
      delay(500);
      digitalWrite(LED_BLUE,1);
      delay(500);
      i++;
    }
    digitalWrite(SPK,0);
    #if defined DEBUG
    ObitSerial.println("Xe bi rung");
    #endif
    sim808.sendSMS(PHONE_NUMBER,MESSAGE_RUNGXE); // gửi tin nhắn rung xe
    //SMS_Location_googlemap(smslink);
    //sim808.sendSMS(PHONE_NUMBER,smslink);  /// send toa do gps
    vibra=0;
  }
  
}

int bike_crash;
void DetecCrash() // phát hiện ngã xe
{
  int16_t First_value=0, Last_value=0;
  accelgyro.getMotion6(&ax, &ay, &az, &gx, &gy, &gz);
  First_value = ax; // đọc gốc nghiêng ban đầu
  Serial.println("first");
  Serial.println(ax);
  delay(1000);
  accelgyro.getMotion6(&ax, &ay, &az, &gx, &gy, &gz);
  Serial.println("last");
  Last_value = ax; // góc nghiêng sau 1s
   Serial.println(ax);
  if((abs(abs(Last_value) - abs(First_value)) > 10000)&&(bike_crash == 0)) //nếu nghiên
  {
    Serial.println("vlue");
    Serial.println(abs(abs(Last_value) - abs(First_value)));
    #if defined DEBUG
    ObitSerial.println(Last_value );
    ObitSerial.println(First_value );
    ObitSerial.println("Phat hien xe bi nghieng");
    #endif
    bike_crash = 1;
    delay(500);
  }
  if(bike_crash)
  {
    digitalWrite(SPK,1); // kêu còi
    int i=0;
    while(i<20)
    {
      digitalWrite(LED_YELLOW,0);
      delay(500);
      digitalWrite(LED_YELLOW,1);
      delay(500);
      i++;
    }
    digitalWrite(SPK,0);
    sim808.sendSMS(PHONE_NUMBER,MESSAGE_DOXE); // send sms warning
    //sim808.sendSMS(PHONE_NUMBER,smslink);  /// send toa do gps
    sim808.sendSMS(PHONE_NUMBER_1,MESSAGE_DOXE); // send sms warning
    //sim808.sendSMS(PHONE_NUMBER_1,smslink);  /// send toa do gps
    bike_crash = 0;
  }
}



// the setup function runs once when you press reset or power the board
void setup() {
  pinMode(LED_YELLOW,OUTPUT);
  pinMode(LED_BLUE,OUTPUT);
  pinMode(LED_RED,OUTPUT);
  pinMode(LED_GREEN,OUTPUT);
  pinMode(SPK,OUTPUT);
  pinMode(SIM_PWR,OUTPUT);
  pinMode(ON_OFF,INPUT);
  pinMode(OFF_LOCK,INPUT);
  pinMode(OFF_SPK,INPUT);
  pinMode(VIBRATE,INPUT_PULLUP);
  attachInterrupt(0, DetecVibrate, FALLING);


  Wire.begin();
	Serial.begin(9600);
	ObitSerial.begin(9600);
	digitalWrite(SIM_PWR, 0);
  accelgyro.initialize();
	delay(1500);
	digitalWrite(SIM_PWR, 1);
	delay(5000);
	ObitSerial.println("Init");
	while (!sim808.init()) {
		digitalWrite(LED_YELLOW, 0);
		delay(500);
		digitalWrite(LED_YELLOW, 1);
		delay(500);
		ObitSerial.println("Sim808 init error\r\n");
	}
	delay(3000);
	ObitSerial.println("SIM init done");
	while (!sim808.attachGPS()) // khoi tao gps
	{
		ObitSerial.print("Sim808 gps init fail\r\n");
		digitalWrite(LED_GREEN , 0);
		delay(500);
		digitalWrite(LED_GREEN, 1);
		delay(500);
	}
 digitalWrite(LED_GREEN,1);
 digitalWrite(LED_RED,1);
 digitalWrite(LED_BLUE,1);
 digitalWrite(LED_YELLOW,1);
}



int active_gps, sms_run_flag;
// the loop function runs over and over again until power down or reset
void loop() 
{
  /*
  if (GetGPS())
  {
    digitalWrite(LED_GREEN, 0);
    SMS_Location_googlemap(smslink);
    sim808.detachGPS();
  }
  else digitalWrite(LED_GREEN, 1);
  */
  if (sim808.getGPS()) // đợi gps có tín hiệu
  {
    sim808.attachGPS();
    SMS_Location_googlemap(smslink);
    digitalWrite(LED_GREEN, !digitalRead(LED_GREEN));
    active_gps = 1;
    delay(200);
  }
  if(active_gps) // có tín hiệu gps
  {
  sim808.getGPS();
  //sim808.attachGPS();
  SMS_Location_googlemap(smslink);
  digitalWrite(LED_GREEN,0);
  for(int i=0; i<5; i++)
  {
  if((digitalRead(ON_OFF) == 1)) // mode chong trom
  {
    sms_run_flag = 1;
    if(digitalRead(OFF_LOCK) == 1)
    {
    digitalWrite(LED_RED,0);
    digitalWrite(LED_BLUE,1);
    digitalWrite(LED_YELLOW,1);
    Vibarte_Process();
    }
    else // tắt chống trộm
    {
    vibra = 0;
    digitalWrite(LED_YELLOW,0);
    digitalWrite(LED_RED,1);
    digitalWrite(LED_BLUE,1);
    }
  }
  else  // mode canh bao do xe
  {
    if(sms_run_flag)
    {
      sim808.sendSMS(PHONE_NUMBER,"Xe cua ban da bat nguon"); // send sms báo bật nguồn xe
      sms_run_flag = 0;
    }
    else
    {
    vibra = 0;
    digitalWrite(LED_RED,1);
    digitalWrite(LED_YELLOW,1);
    digitalWrite(LED_BLUE,0);
    DetecCrash();
    vibra = 0;
    }
  }
  }
  GetCMDSMS();
  //sim808.detachGPS();
  }
}
