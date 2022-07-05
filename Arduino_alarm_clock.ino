#include <Wire.h>
#include <LiquidCrystal.h>
#include <EEPROM.h>
#include <dht_nonblocking.h>
#include "RTClib.h"

#define DHT_SENSOR_TYPE DHT_TYPE_11

byte  ClockSymbol[8] = 
{
    0b00000,
    0b00100,
    0b01110,
    0b01110,
    0b11111,
    0b00000,
    0b00100,
    0b00000
};

byte  TempSymbol[8] =
{
  0b10000,
  0b00111,
  0b01000,
  0b01000,
  0b01000,
  0b01000,
  0b00111,
  0b00000
};

DHT_nonblocking DHT_Sensor(10, DHT_SENSOR_TYPE);
LiquidCrystal Screen(2, 3, 4, 5, 6, 7);
RTC_DS1307 RealTimeClock;

const int Buzzer = 9;
const int ButtonSet = A5;
const int ButtonAdjust = A4; // Used to increase integer values when setting clock/alarm
const int ButtonAlarm = A3; // set alarm on/off & decrease integer values when setting clock/alarm
const int Melody[4] = {600, 800, 1000, 1200};

int Day, Month, Year, Hours, Minutes, Seconds, SetState, AdjustState, AlarmState, AlarmHour, AlarmMinutes;
int i = 0;
int ButtonCount = 0;
int TempC;
int TempHumidity;
int SoundInterval = 300;

unsigned long LastTime;

float Temperature;
float Humidity;

String sDay;
String sMonth;
String sYear;
String sHours;
String sMinutes;
String sSeconds;
String aH = "00";
String aM = "00";

bool bInSetupScreen = false;
bool bAlarmSet = false;
bool bAlarmTriggered = false;
bool bSwitchDisplayMode = false;

void setup()
{
  Screen.begin(16, 2);
  Screen.clear();

  pinMode(ButtonSet, INPUT_PULLUP);
  pinMode(ButtonAdjust, INPUT_PULLUP);
  pinMode(ButtonAlarm, INPUT_PULLUP);
  pinMode(Buzzer, OUTPUT);

  Serial.begin(9600);
  Wire.begin();
  RealTimeClock.begin();
  Screen.createChar(1, ClockSymbol);
  Screen.createChar(2, TempSymbol);
  if (!RealTimeClock.isrunning())
  {
    Serial.println("RealTimeClock is not running");
    RealTimeClock.adjust(DateTime(__DATE__, __TIME__));
  }
  delay(100);

  AlarmHour = EEPROM.read(0);
  AlarmMinutes = EEPROM.read(1);

  if (AlarmHour > 23)
  {
    AlarmHour = 0;
  }
  if (AlarmMinutes > 59)
  {
    AlarmMinutes = 0;
  }
}

void loop()
{
  ReadButtons();
  GetDateTime();
  if (!bInSetupScreen)
  {
    PrintScreen();
    if (bAlarmSet)
    {
      CheckAlarm();
    }
    else
    {
      Screen.setCursor(10, 0);
      Screen.write(" ");
    }
  }
  else
  {
    SetupScreen();
  }
  if (MeasureEnvironment(&Temperature, &Humidity) == true)
  {
    TempC = Temperature;
    TempHumidity = Humidity;
  }
}

void CheckAlarm()
{
  if (AlarmMinutes == Minutes && AlarmHour == Hours && Seconds >= 0 && Seconds <= 10)
  {
    bAlarmTriggered = true;
  }
  else if (bAlarmTriggered && (AlarmState == LOW || Seconds >= 59))
  {
    bAlarmTriggered = false;
    bAlarmSet = false;
    i = 0;
    delay(50);
  }
  if (bAlarmTriggered)
  {
    unsigned long CurrentMS = millis();
    if (CurrentMS - LastTime > SoundInterval)
    {
      LastTime = CurrentMS;
      tone(Buzzer, Melody[i++], 100);
      if (i > 3)
      {
        i = 0;
      }
    }
  }
  else
  {
    noTone(Buzzer);
  }
}

void SetupScreen()
{
  int UpState = AdjustState;
  int DownState = AlarmState;

  if (ButtonCount <= 5)
  {
    if (ButtonCount == 1) // set hour
    {
      Screen.setCursor(4, 0);
      Screen.print(">");
      if (UpState == LOW)
      {
        if (Hours < 23)
        {
          Hours++;
        }
        else
        {
          Hours = 0;
        }
        delay(350);
      }
      if (DownState == LOW)
      {
        if (Hours > 0)
        {
          Hours--;
        }
        else
        {
          Hours = 23;
        }
        delay(350);
      }
    }
    else if (ButtonCount == 2) // set minutes
    {
      Screen.setCursor(4, 0);
      Screen.print(" ");
      Screen.setCursor(9, 0);
      Screen.print(">");
      if (UpState == LOW)
      {
        if (Minutes < 59)
        {
          Minutes++;
        }
        else
        {
          Minutes = 0;
        }
        delay(350);
      }
      if (DownState == LOW)
      {
        if (Minutes > 0)
        {
          Minutes--;
        }
        else
        {
          Minutes = 59;
        }
        delay(350);
      }
    }
    else if (ButtonCount == 3) // set day
    {
      Screen.setCursor(9,0);
      Screen.print(" ");
      Screen.setCursor(0,1);
      Screen.print(">");
      if (UpState == LOW)
      {
        if (Day < 31)
        {
          Day++;
        }
        else
        {
          Day = 1;
        }
        delay(350);
      }
      if (DownState == LOW)
      {
        if (Day > 1)
        {
          Day--;
        }
        else
        {
          Day = 31;
        }
        delay(350);
      }
    }
    else if (ButtonCount == 4) // set month
    {
      Screen.setCursor(0,1);
      Screen.print(" ");
      Screen.setCursor(5,1);
      Screen.print(">");
      if (UpState == LOW)
      {
        if (Month < 12)
        {
          Month++;
        }
        else
        {
          Month = 1;
        }
        delay(350);
      }
      if (DownState == LOW)
      {
        if (Month > 1)
        {
          Month--;
        }
        else
        {
          Month = 12;
        }
        delay(350);
      }
    }
    else if (ButtonCount == 5) // set year
    {
      Screen.setCursor(5,1);
      Screen.print(" ");
      Screen.setCursor(10,1);
      Screen.print(">");
      if (UpState == LOW)
      {
        if (Year < 2999)
        {
          Year++;
        }
        else
        {
          Year = 2000;
        }
        delay(350);
      }
      if (DownState == LOW)
      {
        if (Year > 2000)
        {
          Year--;
        }
        else
        {
          Year = 2999;
        }
        delay(350);
      }
    }
    Screen.setCursor(5,0);
    Screen.print(sHours);
    Screen.setCursor(8,0);
    Screen.print(":");
    Screen.setCursor(10,0);
    Screen.print(sMinutes);
    Screen.setCursor(1,1);
    Screen.print(sDay);
    Screen.setCursor(4,1);
    Screen.print("-");
    Screen.setCursor(6,1);
    Screen.print(sMonth);
    Screen.setCursor(9,1);
    Screen.print("-");
    Screen.setCursor(11,1);
    Screen.print(sYear);
  }
  else
  {
    SetAlarmTime();
  }
}

static void SetAlarmTimeHours()
{
  if (AdjustState == LOW)
  {
    if (AlarmHour < 23)
    {
      AlarmHour++;
    }
    else
    {
      AlarmHour = 0;
    }
    delay(350);
  }
  if (AlarmState == LOW)
  {
    if (AlarmHour > 0)
    {
      AlarmHour--;
    }
    else
    {
      AlarmHour = 23;
    }
    delay(350);
  }
}

static void SetAlarmTimeMinutes()
{
  if (AdjustState == LOW)
  {
    if (AlarmMinutes < 59)
    {
      AlarmMinutes++;
    }
    else
    {
      AlarmMinutes = 0;
    }
    delay(350);
  }
  if (AlarmState == LOW)
  {
    if (AlarmMinutes > 0)
    {
      AlarmMinutes--;
    }
    else
    {
      AlarmMinutes = 59;
    }
    delay(350);
  }
}

void SetAlarmTime()
{
  String line2;
  Screen.setCursor(0,0);
  Screen.print("SET  ALARM TIME");
  if (ButtonCount == 6)
  {
    SetAlarmTimeHours();
    line2 = "    >" + aH + " : " + aM + "    ";
  }
  else if (ButtonCount == 7)
  {
    SetAlarmTimeMinutes();
    line2 = "     " + aH + " :>" + aM + "    ";    
  }
  Screen.setCursor(0,1);
  Screen.print(line2);
}

void ReadButtons()
{
  SetState = digitalRead(ButtonSet);
  AdjustState = digitalRead(ButtonAdjust);
  AlarmState = digitalRead(ButtonAlarm);

  if (!bInSetupScreen)
  {
    if (AlarmState == LOW)
    {
      bAlarmSet = !bAlarmSet;
      delay(500);
    }
  }
  if (SetState == LOW)
  {
    if (ButtonCount < 7)
    {
      ButtonCount++;
      bInSetupScreen = true;
      if (ButtonCount == 1)
      {
        Screen.clear();
        Screen.setCursor(0, 0);
        Screen.print("------SET------");
        Screen.setCursor(0, 1);
        Screen.print("-TIME and DATE-");
        delay(2000);
        Screen.clear();
      }
    }
    else
    {
      Screen.clear();
      RealTimeClock.adjust(DateTime(Year, Month, Day, Hours, Minutes, 0));
      EEPROM.write(0, AlarmHour);
      EEPROM.write(1, AlarmMinutes);
      Screen.print("...Saving...");
      delay(2000);
      Screen.clear();
      bInSetupScreen = false;
      ButtonCount = 0;
    }
    delay(500);
  }
}

void  PrintScreen()
{
  String TimeDate = sHours + ":" + sMinutes + ":" + sSeconds + " |";
  Screen.setCursor(0,0);
  Screen.print(TimeDate);
  if (bAlarmSet)
  {
    Screen.setCursor(10, 0);
    Screen.write(1);
  }
  String line2 = aH + ":" + aM;
  Screen.setCursor(11, 0);
  Screen.print(line2);
  String line3;
  if (bSwitchDisplayMode)
  {
    line3 = sDay + "-" + sMonth + "-" + sYear + " | " + TempHumidity;
  }
  else
  {
    line3 = sDay + "-" + sMonth + "-" + sYear + " | " + TempC;
  }
  Screen.setCursor(0, 1);
  Screen.print(line3);
  Screen.setCursor(13, 1);
  if (bInSetupScreen)
  {
    Screen.print("");
  }
  else if (bSwitchDisplayMode)
  {
     Screen.print("%");
  }
  else
  {
    Screen.write(2);
  }
}

void  GetDateTime()
{
  if (!bInSetupScreen)
  {
    DateTime curr = RealTimeClock.now();
    Day = curr.day();
    Month = curr.month();
    Year = curr.year();
    Hours = curr.hour();
    Minutes = curr.minute();
    Seconds = curr.second();
  }
  sYear = String(Year - 2000);
  sDay = (Day < 10) ? '0' + String(Day) : String(Day);
  sMonth = (Month < 10) ? '0' + String(Month) : String(Month);
  sHours = (Hours < 10) ? '0' + String(Hours) : String(Hours);
  sMinutes = (Minutes < 10) ? '0' + String(Minutes) : String(Minutes);
  sSeconds = (Seconds < 10) ? '0' + String(Seconds) : String(Seconds);
  aH = (AlarmHour < 10) ? '0' + String(AlarmHour) : String(AlarmHour);
  aM = (AlarmMinutes < 10) ? '0' + String(AlarmMinutes) : String(AlarmMinutes);
}

static bool MeasureEnvironment(float *Temperature, float *Humidity)
{
  static unsigned long MeasureTimestamp = millis();

  if ((millis() - MeasureTimestamp) > 3000ul)
  {
    MeasureTimestamp = millis();
    if (DHT_Sensor.measure(Temperature, Humidity) == true)
    {
      bSwitchDisplayMode = !bSwitchDisplayMode;
      return true;
    }
  }
  return false;
}
