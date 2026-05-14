#ifndef TESTS_MOCKS_MOCKSENSOR_H
#define TESTS_MOCKS_MOCKSENSOR_H

#include "Interfaces/ISensor.h"

class MockSensor : public ISensor {
public:
    MockSensor(uint8_t id, const char* name, float value = 0.0f, bool alarm = false)
        : _id(id), _name(name), _value(value), _alarm(alarm) {}

    uint8_t     id()      const override { return _id; }
    const char* name()    const override { return _name; }
    float       read()          override { return _value; }
    bool        isAlarm()       override { return _alarm; }

    void setValue(float v) { _value = v; }
    void setAlarm(bool a)  { _alarm = a; }

private:
    uint8_t     _id;
    const char* _name;
    float       _value;
    bool        _alarm;
};

#endif // TESTS_MOCKS_MOCKSENSOR_H
