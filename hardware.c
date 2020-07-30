#include<SoftwareSerial.h>
#include "TinyGPS++.h"
SoftwareSerial GPS_serial(10,11); //GPS connection
SoftwareSerial GSM_serial(2,3); //GSM connection
TinyGPSPlus gps;
//analog pins of Arduino
int data=A4;
#define x A1
#define y A2
#define z A3
int xsample=0;
int ysample=0;
int zsample=0;
int start=6;
int buttonstate=0;
int flag=0;
int waitcount=0;
#define samples 10
#define MaxVal -10
int i=0,k=0;
int gps_status=0;
float latitude=0;
float logitude=0;
int count=0;
int pulse_count=0;
int twosec=0;
void setup()
{
	 Serial.begin(9600);
	 GSM_serial.begin(9600); //GSM baud rate
	 GPS_serial.begin(9600); //gps baud rate
	 pinMode(data,INPUT); //heartbeat sensor
	 pinMode(start,INPUT); //button
	 pinMode(8, OUTPUT); //For speaker output

	 for(int i=0;i<samples;i++) //accelerometer readings - taking average of 10 readings for accuracy
	 {
		 xsample+=analogRead(x);
		 ysample+=analogRead(y);
		 zsample+=analogRead(z);
	 }
	 xsample/=samples;
	 ysample/=samples;
	 zsample/=samples;
	 delay(2000);
}
void loop()
{
	 flag=0;
	 if(count==0){
		 int value1=analogRead(x);
		 int value2=analogRead(y);
		 int value3=analogRead(z);
		 int xValue=xsample-value1;
		 int yValue=ysample-value2;
		 int zValue=zsample-value3;
		 Serial.println(xValue);
		 Serial.println(yValue);
		 Serial.println(zValue);
		 //calculating pulses in the last few seconds (to check for a surge)
		 if (millis()<twosec+2000)
		 {
			 if(analogRead(data)<100){
				 pulse_count++;
				 while(analogRead(data)<100);
			 }
		 }
		 else{
			 twosec=millis(); //refresh that variable every 2 sec
			 Serial.println("No. of pulses in last 2 sec=" + String(pulse_count));
			 pulse_count=0;
		 }
		 if((pulse_count>4)&&(zValue<MaxVal)) //accident detected (pulse surges to around 120 BPM and vehicle undergoes a fall)
		 {
			speaker_with_button();
		 }
	 }
}
void speaker_with_button()
{
	 while(waitcount<15 && flag==0) // 15 seconds waiting time for the user to cancel the accident alert
	 {
		 tone( 8, 33, 500); //buzzing sound issued to alert the user
		 delay(1000);
		 waitcount=waitcount+1;
		 buttonstate = digitalRead(start);
		 if(buttonstate==1)
		 {
			flag=1;
		 }
	 }
	 if(flag==1)
	 {
		Serial.println("No accident");
	 }

	 else if(flag==0) //not cancelled - location fetched and accident alert to be sent
	 {
		gpsEvent();
	 }
}
void ShowSerialData() //function to write data to GSM stream
{
	 while(GSM_serial.available()!=0)
		Serial.write(GSM_serial.read());
	 delay(5000);

}
void gpsEvent()
{
	 while(GPS_serial.available())
	 {
		gps.encode(GPS_serial.read());
	 }
	 if(gps.location.isUpdated())
	 {
		 float latt=gps.location.lat();
		 float longg=gps.location.lng();
		 String latitude=String(latt,6);
		 String longitude=String(longg,6);
		 if (GSM_serial.available())
		 {
			 Serial.write(GSM_serial.read());
			 //GSM commands to initiate communication with cloud
			 GSM_serial.println("AT");
			 delay(1000);
			 GSM_serial.println("AT+CPIN?");
			 delay(1000);
			 GSM_serial.println("AT+CREG?");
			 delay(1000);
			 GSM_serial.println("AT+CGATT?");
			 delay(1000);
			 GSM_serial.println("AT+CIPSHUT");
			 delay(1000);
			 GSM_serial.println("AT+CIPSTATUS");
			 delay(2000);
			 GSM_serial.println("AT+CIPMUX=0");
			 delay(2000);
			 ShowSerialData();
			 GSM_serial.println("AT+CSTT=\"airtelgprs.com\"");//start task and set the APN
			 delay(1000);
			 ShowSerialData();
			 GSM_serial.println("AT+CIICR");//bring up wireless connection
			 delay(3000);
			 ShowSerialData();
			 GSM_serial.println("AT+CIFSR");//get local IP adress
			 delay(2000);
			 ShowSerialData();
			 GSM_serial.println("AT+CIPSPRT=0");
			 delay(3000);
			 ShowSerialData();

			 GSM_serial.println("AT+CIPSTART=\"TCP\",\"api.thingspeak.com\",\"80\"");//start up the connection	
			 delay(6000);
			 ShowSerialData();
			 GSM_serial.println("AT+CIPSEND");//begin send data to remote server
			 delay(4000);
			 ShowSerialData();

			 String str="GET https://api.thingspeak.com/update?
			 api_key=YYYYYYYY&field4=" + latitude +"&field5="+longitude; //YYYYYY=api key of thingspeak to upload data to the cloud
			 Serial.println(str);
			 GSM_serial.println(str);//begin send data to remote server

			 delay(4000);
			 ShowSerialData();
			 GSM_serial.println((char)26);//sending
			 delay(5000);//waiting for reply -important. time depends on the quality of network connection
			 GSM_serial.println();
			 ShowSerialData();
			 GSM_serial.println("AT+CIPSHUT");//close the connection
			 delay(100);
			 ShowSerialData();
		 }
		 while(count==0)
		 {

			 //SMS message sent to family member or friend (optional)
			 GSM_serial.println("AT+CMGF=1");
			 delay(1000);
			 GSM_serial.println("AT+CMGS=\"+91XXXXXXXXXX\"\r"); //mobile number
			 delay(1000);
			 GSM_serial.println("Accident has been detected at the following location. Alert has been sent to nearby hospital.");
			 GSM_serial.println("Latitude:");
			 GSM_serial.println(gps.location.lat(), 6);
			 GSM_serial.println("Longitude:");
			 GSM_serial.println(gps.location.lng(), 6);
			 delay(100);
			 GSM_serial.println((char)26);
			 delay(1000);
			count++; /*count variable is used so that once an accident has occurred and necessary data has been sent, system stops further monitoring and
						alerting. */

		 }
	 }
}
