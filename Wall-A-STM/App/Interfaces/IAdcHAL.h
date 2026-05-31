#ifndef APP_INTERFACES_IADCHAL_H
#define APP_INTERFACES_IADCHAL_H

#include <cstdint>

class IAdcHAL {
public:
	virtual ~IAdcHAL() = default;

	virtual void start() = 0;              // démarre une conversion (non-bloquant)
	virtual uint16_t rawValue() = 0;       // dernière valeur brute convertie
	virtual bool isActive() const = 0;     // true tant que la conversion est en cours
};

#endif // APP_INTERFACES_IADCHAL_H
