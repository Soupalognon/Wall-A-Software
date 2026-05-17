#ifndef APP_SERVICES_CONCRETEODOMHAL_H
#define APP_SERVICES_CONCRETEODOMHAL_H

#include "Interfaces/IOdomHAL.h"
#include "Interfaces/IEncoderHAL.h"
#include <cstdint>

class Odometry: public IOdomHAL {
public:
	Odometry(IEncoderHAL *encL, IEncoderHAL *encR);

	bool begin();

	void update() override;
	float getX() override;
	float getY() override;
	float getAngle() override;
	float getVLeft() override;
	float getVRight() override;
	float getV() override;
	float getW() override;
	float getDt() override;

private:
	IEncoderHAL *_encL;
	IEncoderHAL *_encR;
	int32_t _prevTicksL = 0;
	int32_t _prevTicksR = 0;
	float _x = 0.0f;		//In meter
	float _y = 0.0f;		//In meter
	float _angle = 0.0f;	//In radian
	float _vL = 0.0f;		//In m/s
	float _vR = 0.0f;		//In m/s
	float _v = 0.0f;		//In m/s
	float _w = 0.0f;		//In rad/s
	float _dt = 0.005f;		//In seconds
	uint32_t _lastMicros = 0;  // stores HAL_GetTick() value (ms); name kept for legacy, unit is ms

	uint32_t getDtFromTimer();
};

#endif // APP_SERVICES_CONCRETEODOMHAL_H
