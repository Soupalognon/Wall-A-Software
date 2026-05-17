#ifndef APP_TASKS_EXTERNALCOMM_H
#define APP_TASKS_EXTERNALCOMM_H

#include "Interfaces/IBus.h"
#include "Interfaces/IActuatorManager.h"
#include "Interfaces/ICommChannel.h"
#include <FreeRTOS.h>
#include <queue.h>
#include <cstdarg>
#include <cstdint>
#include <cstring>

class ExternalComm : public IBus {
public:
    struct CommSnapshot {
        uint32_t rxCount{};
        uint32_t txCount{};
        char     lastCmd[32]{};
        uint32_t timestamp{};
    };
    static CommSnapshot latestSnapshot;

    enum class QueuePolicy : uint8_t { OVERWRITE, DROP_SILENT };
    struct BusConfig {
        Topic       topic;
        QueuePolicy policy;
    };
    static constexpr BusConfig BUS_CONFIG[] = {
        { Topic::TELEMETRY, QueuePolicy::OVERWRITE   },
        { Topic::ALERT,     QueuePolicy::OVERWRITE   },
        { Topic::HEALTH,    QueuePolicy::DROP_SILENT },
        { Topic::LOG,       QueuePolicy::DROP_SILENT },
    };

    ExternalComm(ICommChannel* uart,
                 ICommChannel* usb,
                 ICommChannel* eth,
                 IActuatorManager* actuatorMgr,
                 QueueHandle_t motionMailbox);

    void publish(Topic topic, const char* payload) override;
    static void log_info(const char* fmt, ...) __attribute__((format(printf, 1, 2)));
    static void log_warn(const char* fmt, ...) __attribute__((format(printf, 1, 2)));
    static void log_error(const char* fmt, ...) __attribute__((format(printf, 1, 2)));

    QueueHandle_t rxByteQueue() const { return _rxByteQueue; }

    static void rxTask(void* arg);
    static void txTask(void* arg);

private:
    struct TxEntry { char buf[150]; };

    struct RxAccum {
        char     buf[150]{};
        uint16_t itr{};
        uint32_t timer{};
    };
    RxAccum _uartAccum;
    RxAccum _usbAccum;

    ICommChannel*     _uart;
    ICommChannel*     _usb;
    ICommChannel*     _eth;
    IActuatorManager* _actuatorMgr;
    QueueHandle_t     _motionMailbox;

    QueueHandle_t    _rxByteQueue;
    QueueHandle_t    _telQueue;
    QueueHandle_t    _altQueue;
    QueueHandle_t    _hltQueue;
    QueueHandle_t    _logQueue;
    QueueSetHandle_t _txQueueSet;

    static ExternalComm* _instance;

    static void _logImpl(const char* prefix, const char* fmt, va_list args);
    QueueHandle_t _queueForTopic(Topic t) const;
    void _processRxLine(const char* line, bool uartSource);
    void _feedAccum(RxAccum& acc, const char* data, uint16_t len, bool uartSource);
    void _transmitAll(const char* msg, uint16_t len, bool includeUsb);
};

#endif // APP_TASKS_EXTERNALCOMM_H
