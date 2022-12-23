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

extern gdl90Heartbeat_t lastHeartbeat;
