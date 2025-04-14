#include "DS3231_CS.h"

DS3231_CS::DS3231_CS(TwoWire &wire) {
    _wire = &wire;
    _lastError = DS3231_SUCCESS;
}

uint8_t DS3231_CS::begin(bool checkDefaultTime) {
    _wire->begin();
    
    if (checkDefaultTime) {
        if (isDefaultTime()) {
            _lastError = DS3231_ERROR_CLOCK_HALTED;
            return DS3231_ERROR_CLOCK_HALTED;
        }
    }
    
    // Check if device is present
    _wire->beginTransmission(_address);
    if (_wire->endTransmission() != 0) {
        _lastError = DS3231_ERROR_I2C;
        return DS3231_ERROR_I2C;
    }
    
    // Enable oscillator if not already enabled
    uint8_t status = readRegister(0x0F);
    if (status & (1 << 7)) {
        writeRegister(0x0F, status & ~(1 << 7));
    }
    
    return DS3231_SUCCESS;
}

uint8_t DS3231_CS::setDateTime(uint8_t year, uint8_t month, uint8_t day, uint8_t hour, uint8_t minute, uint8_t second) {
    if (!isValidDate(year, month, day)) {
        _lastError = DS3231_ERROR_INVALID_DATE;
        return DS3231_ERROR_INVALID_DATE;
    }
    
    if (!isValidTime(hour, minute, second)) {
        _lastError = DS3231_ERROR_INVALID_TIME;
        return DS3231_ERROR_INVALID_TIME;
    }
    
    uint8_t weekDay = getWeekDay(year, month, day);
    
    _wire->beginTransmission(_address);
    _wire->write(0x00); // Start at register 0
    
    _wire->write(decToBcd(second));
    _wire->write(decToBcd(minute));
    _wire->write(decToBcd(hour));
    _wire->write(decToBcd(weekDay));
    _wire->write(decToBcd(day));
    _wire->write(decToBcd(month));
    _wire->write(decToBcd(year));
    
    if (_wire->endTransmission() != 0) {
        _lastError = DS3231_ERROR_I2C;
        return DS3231_ERROR_I2C;
    }
    
    // Clear OSF flag
    uint8_t status = readRegister(0x0F);
    writeRegister(0x0F, status & ~(1 << 7));
    
    return DS3231_SUCCESS;
}


uint8_t DS3231_CS::getDateTime(uint8_t &year, uint8_t &month, uint8_t &day,
    uint8_t &hour, uint8_t &minute, uint8_t &second,
    uint8_t &weekDay) {
_wire->beginTransmission(_address);
_wire->write(0x00);

if (_wire->endTransmission() != 0) {
_lastError = DS3231_ERROR_I2C;
return DS3231_ERROR_I2C;
}

if (_wire->requestFrom((uint8_t)_address, (uint8_t)7) != 7) {
_lastError = DS3231_ERROR_I2C;
return DS3231_ERROR_I2C;
}

second = bcdToDec(_wire->read() & 0x7F);
minute = bcdToDec(_wire->read());
hour = bcdToDec(_wire->read() & 0x3F);
weekDay = bcdToDec(_wire->read());
day = bcdToDec(_wire->read());
month = bcdToDec(_wire->read() & 0x1F);
year = bcdToDec(_wire->read());

return DS3231_SUCCESS;
}

/*uint8_t DS3231_CS::getDateTime(uint8_t &year, uint8_t &month, uint8_t &day, uint8_t &hour, uint8_t &minute, uint8_t &second, uint8_t &weekDay) {
    _wire->beginTransmission(_address);
    _wire->write(0x00);
    
    if (_wire->endTransmission() != 0) {
        _lastError = DS3231_ERROR_I2C;
        return DS3231_ERROR_I2C;
    }
    
    if (_wire->requestFrom(_address, 7) != 7) {
        _lastError = DS3231_ERROR_I2C;
        return DS3231_ERROR_I2C;
    }
    
    second = bcdToDec(_wire->read() & 0x7F);
    minute = bcdToDec(_wire->read());
    hour = bcdToDec(_wire->read() & 0x3F);
    weekDay = bcdToDec(_wire->read());
    day = bcdToDec(_wire->read());
    month = bcdToDec(_wire->read() & 0x1F);
    year = bcdToDec(_wire->read());
    
    return DS3231_SUCCESS;
}
*/

String DS3231_CS::getDayName(uint8_t weekDay) {
    const char* days[] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};
    if (weekDay >= 1 && weekDay <= 7) {
        return String(days[weekDay - 1]);
    }
    return "Invalid";
}

uint8_t DS3231_CS::getWeekDay(uint8_t year, uint8_t month, uint8_t day) {
    // Tomohiko Sakamoto's algorithm - returns 0=Sunday
    // Convert to full year (2000-2099)
    uint16_t y = 2000 + year;
    
    static const uint8_t monthOffsets[] = {0, 3, 2, 5, 0, 3, 5, 1, 4, 6, 2, 4};
    
    if (month < 3) {
        y--;
    }
    
    uint8_t weekDay = (y + y/4 - y/100 + y/400 + monthOffsets[month-1] + day) % 7;
    
    // DS3231 uses 1=Sunday, 2=Monday...7=Saturday
    // Sakamoto returns 0=Sunday, so we just add 1
    return weekDay + 1;
}
/*uint8_t DS3231_CS::getWeekDay(uint8_t year, uint8_t month, uint8_t day) {
    // Zeller's Congruence algorithm for accurate day calculation
    if (month < 3) {
        month += 12;
        year--;
    }
    
    uint8_t century = year / 100;
    year = year % 100;
    
    uint8_t weekDay = (day + (13 * (month + 1) / 5) + year + (year / 4) + (century / 4) + (5 * century)) % 7;
    
    // Convert to DS3231 format (1=Sunday, 2=Monday, ..., 7=Saturday)
    return (weekDay + 5) % 7 + 1;
}*/

float DS3231_CS::getTemperature() {
    _wire->beginTransmission(_address);
    _wire->write(0x11);
    
    if (_wire->endTransmission() != 0) {
        _lastError = DS3231_ERROR_I2C;
        return -1000.0; // Error value
    }
    
    if (_wire->requestFrom((uint8_t)_address, (uint8_t)2) != 2) { // Explicit cast
        _lastError = DS3231_ERROR_I2C;
        return -1000.0; // Error value
    }
    
    int8_t tempMSB = _wire->read();
    uint8_t tempLSB = _wire->read() >> 6;
    
    return tempMSB + (tempLSB * 0.25f);
}

float DS3231_CS::getTemperatureC() {
    _wire->beginTransmission(_address);
    _wire->write(0x11);
    
    if (_wire->endTransmission() != 0) {
        _lastError = DS3231_ERROR_I2C;
        return -1000.0f; // Error value
    }
    
    if (_wire->requestFrom((uint8_t)_address, (uint8_t)2) != 2) {
        _lastError = DS3231_ERROR_I2C;
        return -1000.0f; // Error value
    }
    
    int8_t tempMSB = _wire->read();          // Whole degrees
    uint8_t tempLSB = _wire->read() >> 6;    // Fractional part (0.25°C resolution)
    
    return tempMSB + (tempLSB * 0.25f);
}

float DS3231_CS::getTemperatureF() {
    float celsius = getTemperatureC();
    if (celsius < -273.0f) { // If error reading Celsius
        return -1000.0f;     // Return error value
    }
    return celsius * 9.0f / 5.0f + 32.0f;
}

void DS3231_CS::getTemperatureBoth(float &celsius, float &fahrenheit) {
    celsius = getTemperatureC();
    if (celsius < -273.0f) { // If error reading
        fahrenheit = -1000.0f;
    } else {
        fahrenheit = celsius * 9.0f / 5.0f + 32.0f;
    }
}
/*float DS3231_CS::getTemperature() {
    _wire->beginTransmission(_address);
    _wire->write(0x11);
    
    if (_wire->endTransmission() != 0) {
        _lastError = DS3231_ERROR_I2C;
        return -1000.0; // Error value
    }
    
    if (_wire->requestFrom(_address, 2) != 2) {
        _lastError = DS3231_ERROR_I2C;
        return -1000.0; // Error value
    }
    
    int8_t tempMSB = _wire->read();
    uint8_t tempLSB = _wire->read() >> 6;
    
    return tempMSB + (tempLSB * 0.25f);
}*/

uint8_t DS3231_CS::setAlarm1(uint8_t alarmType, uint8_t dayOrDate, uint8_t hours, uint8_t minutes, uint8_t seconds, bool dayNotDate) {
    if (alarmType < 1 || alarmType > 6) {
        _lastError = DS3231_ERROR_INVALID_ALARM;
        return DS3231_ERROR_INVALID_ALARM;
    }
    
    if (!isValidTime(hours, minutes, seconds)) {
        _lastError = DS3231_ERROR_INVALID_TIME;
        return DS3231_ERROR_INVALID_TIME;
    }
    
    _wire->beginTransmission(_address);
    _wire->write(0x07); // Alarm 1 registers start at 0x07
    
    // Set seconds
    if (alarmType == ALARM_EVERY_SECOND) {
        _wire->write(0x80); // Set A1M1
    } else {
        _wire->write(decToBcd(seconds) & 0x7F);
    }
    
    // Set minutes
    if (alarmType == ALARM_EVERY_SECOND || alarmType == ALARM_MATCH_SECONDS) {
        _wire->write(0x80); // Set A1M2
    } else {
        _wire->write(decToBcd(minutes) & 0x7F);
    }
    
    // Set hours
    if (alarmType <= ALARM_MATCH_MINUTES_SECONDS) {
        _wire->write(0x80); // Set A1M3
    } else {
        _wire->write(decToBcd(hours) & 0x3F);
    }
    
    // Set day/date
    if (alarmType <= ALARM_MATCH_HOURS_MINUTES_SECONDS) {
        _wire->write(0x80); // Set A1M4
    } else {
        uint8_t value;
        if (dayNotDate) {
            value = decToBcd(dayOrDate) & 0x3F;
            value |= (1 << 6); // Set DY/DT bit for day
        } else {
            value = decToBcd(dayOrDate) & 0x3F;
        }
        _wire->write(value);
    }
    
    if (_wire->endTransmission() != 0) {
        _lastError = DS3231_ERROR_I2C;
        return DS3231_ERROR_I2C;
    }
    
    // Enable alarm interrupt
    uint8_t control = readRegister(0x0E);
    writeRegister(0x0E, control | 0x05); // Set INTCN and A1IE
    
    return DS3231_SUCCESS;
}

uint8_t DS3231_CS::setAlarm2(uint8_t alarmType, uint8_t dayOrDate, uint8_t hours, uint8_t minutes, bool dayNotDate) {
    if (alarmType < 1 || alarmType > 6) {
        _lastError = DS3231_ERROR_INVALID_ALARM;
        return DS3231_ERROR_INVALID_ALARM;
    }
    
    if (!isValidTime(hours, minutes, 0)) { // Seconds not used in Alarm 2
        _lastError = DS3231_ERROR_INVALID_TIME;
        return DS3231_ERROR_INVALID_TIME;
    }
    
    _wire->beginTransmission(_address);
    _wire->write(0x0B); // Alarm 2 registers start at 0x0B
    
    // Set minutes
    if (alarmType == ALARM_EVERY_SECOND || alarmType == ALARM_MATCH_SECONDS) {
        _wire->write(0x80); // Set A2M2
    } else {
        _wire->write(decToBcd(minutes) & 0x7F);
    }
    
    // Set hours
    if (alarmType <= ALARM_MATCH_MINUTES_SECONDS) {
        _wire->write(0x80); // Set A2M3
    } else {
        _wire->write(decToBcd(hours) & 0x3F);
    }
    
    // Set day/date
    if (alarmType <= ALARM_MATCH_HOURS_MINUTES_SECONDS) {
        _wire->write(0x80); // Set A2M4
    } else {
        uint8_t value;
        if (dayNotDate) {
            value = decToBcd(dayOrDate) & 0x3F;
            value |= (1 << 6); // Set DY/DT bit for day
        } else {
            value = decToBcd(dayOrDate) & 0x3F;
        }
        _wire->write(value);
    }
    
    if (_wire->endTransmission() != 0) {
        _lastError = DS3231_ERROR_I2C;
        return DS3231_ERROR_I2C;
    }
    
    // Enable alarm interrupt
    uint8_t control = readRegister(0x0E);
    writeRegister(0x0E, control | 0x06); // Set INTCN and A2IE
    
    return DS3231_SUCCESS;
}

uint8_t DS3231_CS::clearAlarmFlags() {
    uint8_t status = readRegister(0x0F);
    writeRegister(0x0F, status & 0xFC); // Clear alarm flags
    return DS3231_SUCCESS;
}

uint8_t DS3231_CS::disableAlarms() {
    uint8_t control = readRegister(0x0E);
    writeRegister(0x0E, control & 0xFC); // Disable alarm interrupts
    return clearAlarmFlags();
}

bool DS3231_CS::checkAlarm1() {
    uint8_t status = readRegister(0x0F);
    return (status & 0x01);
}

bool DS3231_CS::checkAlarm2() {
    uint8_t status = readRegister(0x0F);
    return (status & 0x02);
}

uint8_t DS3231_CS::enable32kHz(bool enable) {
    uint8_t status = readRegister(0x0F);
    if (enable) {
        writeRegister(0x0F, status | (1 << 3));
    } else {
        writeRegister(0x0F, status & ~(1 << 3));
    }
    return DS3231_SUCCESS;
}

uint8_t DS3231_CS::enableOscillator(bool enable, bool battery, uint8_t frequency) {
    uint8_t control = readRegister(0x0E);
    
    if (enable) {
        control &= ~(1 << 7); // Clear EOSC bit
        if (!battery) {
            control |= (1 << 6); // Set BBSQW bit
        }
        
        // Set frequency (only bits 3:0 are used)
        control = (control & 0xE7) | ((frequency & 0x03) << 3);
    } else {
        control |= (1 << 7); // Set EOSC bit to disable
    }
    
    writeRegister(0x0E, control);
    return DS3231_SUCCESS;
}

bool DS3231_CS::isRunning() {
    uint8_t status = readRegister(0x0F);
    return !(status & (1 << 7));
}

uint8_t DS3231_CS::lastError() {
    return _lastError;
}

String DS3231_CS::errorText(uint8_t errorCode) {
    switch (errorCode) {
        case DS3231_SUCCESS: return "Success";
        case DS3231_ERROR_I2C: return "I2C communication error";
        case DS3231_ERROR_INVALID_DATE: return "Invalid date";
        case DS3231_ERROR_INVALID_TIME: return "Invalid time";
        case DS3231_ERROR_INVALID_ALARM: return "Invalid alarm configuration";
        case DS3231_ERROR_CLOCK_HALTED: return "Clock was halted (possible default time)";
        default: return "Unknown error";
    }
}

// Private helper functions
uint8_t DS3231_CS::bcdToDec(uint8_t val) {
    return ((val / 16) * 10) + (val % 16);
}

uint8_t DS3231_CS::decToBcd(uint8_t val) {
    return ((val / 10) * 16) + (val % 10);
}

/*bool DS3231_CS::isValidDate(uint8_t year, uint8_t month, uint8_t day) {
    if (year > 99 || month < 1 || month > 12 || day < 1) {
        return false;
    }
    
    uint8_t daysInMonth = daysInMonth(year, month);
    return day <= daysInMonth;
}*/

bool DS3231_CS::isValidDate(uint8_t year, uint8_t month, uint8_t day) {
    if (year > 99 || month < 1 || month > 12 || day < 1) {
        return false;
    }
    
    uint8_t daysInThisMonth = daysInMonth(year, month); // Renamed variable
    return day <= daysInThisMonth;
}

bool DS3231_CS::isValidTime(uint8_t hour, uint8_t minute, uint8_t second) {
    return (hour < 24) && (minute < 60) && (second < 60);
}
/*
bool DS3231_CS::isValidTime(uint8_t hour, uint8_t minute, uint8_t second) {
    return (hour < 24) && (minute < 60) && (second < 60);
}*/

uint8_t DS3231_CS::readRegister(uint8_t reg) {
    _wire->beginTransmission(_address);
    _wire->write(reg);
    _wire->endTransmission();
    
    _wire->requestFrom((uint8_t)_address, (uint8_t)1); // Explicit cast
    if (_wire->available()) {
        return _wire->read();
    }
    return 0;
}

/*
uint8_t DS3231_CS::readRegister(uint8_t reg) {
    _wire->beginTransmission(_address);
    _wire->write(reg);
    _wire->endTransmission();
    
    _wire->requestFrom(_address, 1);
    if (_wire->available()) {
        return _wire->read();
    }
    return 0;
} */

uint8_t DS3231_CS::writeRegister(uint8_t reg, uint8_t value) {
    _wire->beginTransmission(_address);
    _wire->write(reg);
    _wire->write(value);
    return _wire->endTransmission();
}

bool DS3231_CS::isDefaultTime() {
    uint8_t year, month, day, hour, minute, second, weekDay;
    if (getDateTime(year, month, day, hour, minute, second, weekDay) != DS3231_SUCCESS) {
        return true;
    }
    
    // Check for common default values
    if (year == 0 && month == 1 && day == 1 && hour == 0 && minute == 0 && second == 0) {
        return true;
    }
    
    // Check OSF flag (Oscillator Stop Flag)
    uint8_t status = readRegister(0x0F);
    return (status & (1 << 7)) != 0;
}

uint8_t DS3231_CS::daysInMonth(uint8_t year, uint8_t month) {
    static const uint8_t daysPerMonth[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    
    if (month == 2 && isLeapYear(year)) {
        return 29;
    }
    
    if (month >= 1 && month <= 12) {
        return daysPerMonth[month - 1];
    }
    
    return 0;
}

bool DS3231_CS::isLeapYear(uint8_t year) {
    // Convert 2-digit year to full year (2000-2099)
    uint16_t fullYear = 2000 + year;
    
    if (fullYear % 4 != 0) {
        return false;
    } else if (fullYear % 100 != 0) {
        return true;
    } else {
        return (fullYear % 400 == 0);
    }
}