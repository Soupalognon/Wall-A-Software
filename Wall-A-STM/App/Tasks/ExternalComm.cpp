#include "Tasks/ExternalComm.h"
#include "Tasks/MotionPlanner.h"
#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <stm32f4xx_hal.h>

ExternalComm* ExternalComm::_instance = nullptr;

ExternalComm::ExternalComm(ICommChannel *uart, ICommChannel *usb, ICommChannel *eth,
	IActuatorManager *actuatorMgr, QueueHandle_t motionMailbox) :
	_uart(uart), _usb(usb), _eth(eth), _actuatorMgr(actuatorMgr), _motionMailbox(motionMailbox) {
	_rxByteQueue = xQueueCreate(64, sizeof(uint8_t));
	_telQueue    = xQueueCreate(1, sizeof(TxEntry));
	_altQueue    = xQueueCreate(1, sizeof(TxEntry));
	_hltQueue    = xQueueCreate(2, sizeof(TxEntry));
	_logQueue    = xQueueCreate(4, sizeof(TxEntry));

	_txQueueSet = xQueueCreateSet(1 + 1 + 2 + 4);
	xQueueAddToSet(_telQueue, _txQueueSet);
	xQueueAddToSet(_altQueue, _txQueueSet);
	xQueueAddToSet(_hltQueue, _txQueueSet);
	xQueueAddToSet(_logQueue, _txQueueSet);

	_instance = this;
}

void ExternalComm::_logImpl(const char* prefix, const char* fmt, va_list args)
{
	if (_instance == nullptr) return;
	TxEntry entry{};
	size_t plen = strlen(prefix);
	memcpy(entry.buf, prefix, plen);
	std::vsnprintf(entry.buf + plen, sizeof(entry.buf) - plen - 1, fmt, args);
	size_t len = strnlen(entry.buf, sizeof(entry.buf) - 2);  // max 78 so [len]='\n' and [len+1]='\0' are in-bounds
	if (len == 0 || entry.buf[len - 1] != '\n') {
		entry.buf[len]     = '\n';
		entry.buf[len + 1] = '\0';
	}
	_instance->publish(Topic::LOG, entry.buf);
}

void ExternalComm::log_info(const char* fmt, ...) { va_list a; va_start(a, fmt); _logImpl("I ", fmt, a); va_end(a); }
void ExternalComm::log_warn(const char* fmt, ...) { va_list a; va_start(a, fmt); _logImpl("W ", fmt, a); va_end(a); }
void ExternalComm::log_error(const char* fmt, ...) { va_list a; va_start(a, fmt); _logImpl("E ", fmt, a); va_end(a); }

QueueHandle_t ExternalComm::_queueForTopic(Topic t) const {
	switch (t) {
	case Topic::TELEMETRY:
		return _telQueue;
	case Topic::ALERT:
		return _altQueue;
	case Topic::HEALTH:
		return _hltQueue;
	case Topic::LOG:
		return _logQueue;
	default:
		return nullptr;
	}
}

void ExternalComm::publish(Topic topic, const char *payload) {
	TxEntry entry;
	strncpy(entry.buf, payload, sizeof(entry.buf) - 1);
	entry.buf[sizeof(entry.buf) - 1] = '\0';

	QueueHandle_t q = _queueForTopic(topic);
	if (q == nullptr)
		return;

	for (const auto &cfg : BUS_CONFIG) {
		if (cfg.topic == topic) {
			if (cfg.policy == QueuePolicy::OVERWRITE) {
				TxEntry dummy;
				xQueueReceive(q, &dummy, 0); // drain si pleine (xQueueOverwrite interdit sur queue set)
				xQueueSend(q, &entry, 0);
			} else {
				xQueueSend(q, &entry, 0);
			}
			return;
		}
	}
}

void ExternalComm::txTask(void *arg) {
	ExternalComm *self = static_cast<ExternalComm*>(arg);
	TxEntry entry;

	ExternalComm::log_info("txTask: Init OK");
	for (;;) {
		QueueHandle_t q = xQueueSelectFromSet(self->_txQueueSet, portMAX_DELAY);
		if (q != nullptr && xQueueReceive(q, &entry, 0) == pdTRUE) {
			uint16_t len = static_cast<uint16_t>(strlen(entry.buf));
			self->_transmitAll(entry.buf, len);
		}
	}
}

void ExternalComm::rxTask(void *arg) {
	ExternalComm *self = static_cast<ExternalComm*>(arg);
	uint8_t byte;

	ExternalComm::log_info("rxTask: Init OK");
	for (;;) {
		if (xQueueReceive(self->_rxByteQueue, &byte, portMAX_DELAY) == pdTRUE) {
			char c = static_cast<char>(byte);
			self->_feedAccum(self->_uartAccum, &c, 1, true);
		}
	}
}

void ExternalComm::_feedAccum(RxAccum &acc, const char *data, uint16_t len, bool uartSource) {
	if (HAL_GetTick() - acc.timer > 100) {
		acc.itr = 0;
		memset(acc.buf, 0, sizeof(acc.buf));
	}
	acc.timer = HAL_GetTick();

	if (len >= sizeof(acc.buf) - acc.itr) {
		acc.itr = 0;
		publish(Topic::LOG, "Error input too big. flush all\n");
		memset(acc.buf, 0, sizeof(acc.buf));
		return;
	}
	memcpy(acc.buf + acc.itr, data, len);
	acc.itr += len;

	if (acc.buf[acc.itr - 1] == '\n') {
		acc.buf[acc.itr - 1] = '\0';
		log_info("echo: %s", acc.buf);
		_processRxLine(acc.buf, uartSource);
		acc.itr = 0;
		memset(acc.buf, 0, sizeof(acc.buf));
	}
}
#include <string>
void ExternalComm::_processRxLine(const char *line, bool uartSource) {
	(void) uartSource;
	char cmdToken[16] { };
	char verb[16] { };

//	std::string echo = std::string("echo: ") + line + std::string("\n");
//	publish(Topic::LOG, echo.c_str());

	if (sscanf(line, "%15s %15s", cmdToken, verb) < 2)
		return;
	if (strncmp(cmdToken, "CMD", 3) != 0)
		return;

	if (strncmp(verb, "MOVE", 4) == 0) {
		MoveCmd cmd{};
		sscanf(line, "%*s %*s %f %f %f", &cmd.x, &cmd.y, &cmd.angle);
		xQueueOverwrite(_motionMailbox, &cmd);
	} else if (strncmp(verb, "STOP", 4) == 0) {
		xQueueReset(_motionMailbox);
	} else if (strncmp(verb, "ACTUATOR", 8) == 0) {
		char actorId[16] { }, actCmd[32] { };
		sscanf(line, "%*s %*s %15s %31s", actorId, actCmd);
		const char *underscore = strrchr(actorId, '_');
		uint8_t id = underscore ? static_cast<uint8_t>(atoi(underscore + 1)) : 0;
		if (_actuatorMgr)
			_actuatorMgr->commandById(id, actCmd);
	} else {
		publish(Topic::LOG, line);
	}

	latestSnapshot.rxCount++;
	strncpy(latestSnapshot.lastCmd, line, sizeof(latestSnapshot.lastCmd) - 1);
	latestSnapshot.lastCmd[sizeof(latestSnapshot.lastCmd) - 1] = '\0';
	latestSnapshot.timestamp = HAL_GetTick();
}

void ExternalComm::_transmitAll(const char *msg, uint16_t len) {
	if (_usb)
		_usb->transmit(msg, len);
	if (_eth)
		_eth->transmit(msg, len);
	if (_uart)
		_uart->transmit(msg, len);
}
