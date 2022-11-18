#include <sys/cdefs.h>
#include "gdl90.h"
#include "traffic.h"


extern void gdlTask(void *);

typedef enum {
    GDL90_STATE_UNKNOWN,
    GDL90_STATE_INIT,
    GDL90_STATE_WAITING,
    GDL90_STATE_RX,
} gdl90Status_t;

extern char *getGdl90StatusText();

extern bool isTrafficConnected();

extern bool isGpsConnected();

extern void dequeueGdl90Packet(uint32_t delayMs);

extern void processPacket(gdl90Data_t *packet);

extern traffic_t ourPosition;
extern gdl90Heartbeat_t lastHeartbeat;
