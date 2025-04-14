#include <DS3231_CS.h>
#include <Wire.h>

DS3231_CS rtc;

void setup() {
  Serial.begin(9600);
  while (!Serial); // Wait for serial monitor
  
  // Initialize RTC
  uint8_t result = rtc.begin(true); // true = check for default time
  if (result != DS3231_SUCCESS) {
    Serial.print("RTC init failed: ");
    Serial.println(rtc.errorText(result));
    if (result == DS3231_ERROR_CLOCK_HALTED) {
      // Set default time if clock was halted
      rtc.setDateTime(23, 1, 1, 12, 0, 0); // 2023-Jan-1 12:00:00
      Serial.println("Default time set");
    }
    return;
  }

  // Set Alarm 1 to trigger every minute at 00 seconds
  rtc.setAlarm1(ALARM_MATCH_SECONDS, 0, 0, 0, 0);
  Serial.println("Alarm1 set to trigger every minute");
}

void loop() {
  // Read and display date/time
  uint8_t year, month, day, hour, minute, second, weekDay;
  if (rtc.getDateTime(year, month, day, hour, minute, second, weekDay) == DS3231_SUCCESS) {
    Serial.println("\nCurrent Date/Time:");
    Serial.print("20");
    if (year < 10) Serial.print("0");
    Serial.print(year);
    Serial.print("-");
    if (month < 10) Serial.print("0");
    Serial.print(month);
    Serial.print("-");
    if (day < 10) Serial.print("0");
    Serial.print(day);
    Serial.print(" (");
    Serial.print(rtc.getDayName(weekDay));
    Serial.print(") ");
    if (hour < 10) Serial.print("0");
    Serial.print(hour);
    Serial.print(":");
    if (minute < 10) Serial.print("0");
    Serial.print(minute);
    Serial.print(":");
    if (second < 10) Serial.print("0");
    Serial.println(second);
  }

  // Read and display temperature in both units
  float tempC, tempF;
  rtc.getTemperatureBoth(tempC, tempF);
  Serial.print("Temperature: ");
  Serial.print(tempC);
  Serial.print("°C (");
  Serial.print(tempF);
  Serial.println("°F)");

  // Check for alarms
  if (rtc.checkAlarm1()) {
    Serial.println("ALARM1 TRIGGERED!");
    rtc.clearAlarmFlags();
  }

  if (rtc.checkAlarm2()) {
    Serial.println("ALARM2 TRIGGERED!");
    rtc.clearAlarmFlags();
  }

  delay(1000); // Update every second
}