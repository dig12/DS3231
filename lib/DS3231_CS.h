#ifndef DS3231_CS_H
#define DS3231_CS_H

#include <Arduino.h>
#include <Wire.h>

// Error codes
#define DS3231_SUCCESS 0
#define DS3231_ERROR_I2C 1
#define DS3231_ERROR_INVALID_DATE 2
#define DS3231_ERROR_INVALID_TIME 3
#define DS3231_ERROR_INVALID_ALARM 4
#define DS3231_ERROR_CLOCK_HALTED 5

// Alarm types
#define ALARM_DISABLED 0
#define ALARM_EVERY_SECOND 1
#define ALARM_MATCH_SECONDS 2
#define ALARM_MATCH_MINUTES_SECONDS 3
#define ALARM_MATCH_HOURS_MINUTES_SECONDS 4
#define ALARM_MATCH_DAY_HOURS_MINUTES_SECONDS 5
#define ALARM_MATCH_DATE_HOURS_MINUTES_SECONDS 6

// Alarm masks
#define A1M1 7
#define A1M2 6
#define A1M3 5
#define A1M4 4
#define A1M5 3
#define DYDT 6

class DS3231_CS {
public:
    // Constructor
    DS3231_CS(TwoWire &wire = Wire);
    
    // Initialization
    uint8_t begin(bool checkDefaultTime = true);
    
    // Date and time handling
    uint8_t setDateTime(uint8_t year, uint8_t month, uint8_t day, uint8_t hour, uint8_t minute, uint8_t second);
    //uint8_t getDateTime(uint8_t &year, uint8_t &month, uint8_t &day, uint8_t &hour, uint8_t &minute, uint8_t &second, uint8_t &weekDay);
    uint8_t getDateTime(uint8_t &year, uint8_t &month, uint8_t &day, uint8_t &hour, uint8_t &minute, uint8_t &second, 
        uint8_t &weekDay);
    String getDayName(uint8_t weekDay);
    uint8_t getWeekDay(uint8_t year, uint8_t month, uint8_t day);
    
    // Temperature
    float getTemperature();
    float getTemperatureC();       // Get temperature in Celsius
    float getTemperatureF();       // Get temperature in Fahrenheit
    void getTemperatureBoth(float &celsius, float &fahrenheit); // Get both units

    // Alarm functions
    uint8_t setAlarm1(uint8_t alarmType, uint8_t dayOrDate, uint8_t hours, uint8_t minutes, uint8_t seconds, bool dayNotDate = false);
    uint8_t setAlarm2(uint8_t alarmType, uint8_t dayOrDate, uint8_t hours, uint8_t minutes, bool dayNotDate = false);
    uint8_t clearAlarmFlags();
    uint8_t disableAlarms();
    bool checkAlarm1();
    bool checkAlarm2();
    
    // Oscillator functions
    uint8_t enable32kHz(bool enable);
    uint8_t enableOscillator(bool enable, bool battery = true, uint8_t frequency = 0);
    bool isRunning();
    
    // Error handling
    uint8_t lastError();
    String errorText(uint8_t errorCode);
    
private:
    TwoWire *_wire;
    uint8_t _lastError;
    uint8_t _address = 0x68;
    
    // Helper functions
    uint8_t bcdToDec(uint8_t val);
    uint8_t decToBcd(uint8_t val);
    bool isValidDate(uint8_t year, uint8_t month, uint8_t day);
    bool isValidTime(uint8_t hour, uint8_t minute, uint8_t second);
    uint8_t readRegister(uint8_t reg);
    uint8_t writeRegister(uint8_t reg, uint8_t value);
    bool isDefaultTime();
    uint8_t daysInMonth(uint8_t year, uint8_t month);
    bool isLeapYear(uint8_t year);
};

#endif