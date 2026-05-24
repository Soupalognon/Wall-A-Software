#ifndef APP_INTERFACES_IANALOGSOURCE_H
#define APP_INTERFACES_IANALOGSOURCE_H

#include <cstdint>

class IAnalogSource {
public:
	virtual float read(uint8_t channel) = 0;
	virtual ~IAnalogSource() = default;
};

#endif // APP_INTERFACES_IANALOGSOURCE_H
