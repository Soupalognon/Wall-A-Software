#ifndef TESTS_MOCKS_MOCKACTUATOR_H
#define TESTS_MOCKS_MOCKACTUATOR_H

#include "Interfaces/IActuator.h"
#include <cstring>
#include <cstdint>

class MockActuator : public IActuator {
public:
    MockActuator(uint8_t id, const char* name)
        : _id(id), _name(name) {}

    uint8_t     id()   const override { return _id; }
    const char* name() const override { return _name; }
    void command(const char* cmd) override {
        strncpy(_lastCmd, cmd ? cmd : "", sizeof(_lastCmd) - 1);
        _lastCmd[sizeof(_lastCmd) - 1] = '\0';
        ++_commandCallCount;
    }

    const char* lastCmd()          const { return _lastCmd; }
    uint32_t    commandCallCount() const { return _commandCallCount; }
    void        reset()                  { _lastCmd[0] = '\0'; _commandCallCount = 0; }

private:
    uint8_t     _id;
    const char* _name;
    char        _lastCmd[32]{};
    uint32_t    _commandCallCount{};
};

#endif // TESTS_MOCKS_MOCKACTUATOR_H
