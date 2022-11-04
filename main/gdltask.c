#include <sys/cdefs.h>
#include <stdio.h>
#include <sys/socket.h>
#include <esp_log.h>
#include "gdltask.h"
#include "../../main/events.h"
#include "wifi_prov.h"
#include "traffic.h"

#define MAX_AGE_MS 10000      // consider traffic disconnected if nothing received for this many ms
#define GDL_QUEUE_LEN   20      // blocks to buffer in the queue

static const char *TAG = "GDL90";
static int listenPort = 4000;
static int listeningSocket;
static uint8_t rxBuffer[1600];

static gdl90Status_t gdl90Status;
static char gdl90StatusText[256];
static uint32_t lastTrafficSecs;
static QueueHandle_t queue;

char *getGdl90StatusText() {
    return gdl90StatusText;
}

traffic_t ourPosition;
gdl90Heartbeat_t lastHeartbeat;


void processPacket(gdl90Data_t *data) {
    switch (data->id) {
        case GDL90_HEARTBEAT:
            lastHeartbeat = data->heartbeat;
            snprintf(gdl90StatusText, sizeof(gdl90StatusText), "Heartbeat @%d GPSvalid %s", data->heartbeat.timeStamp,
                     data->heartbeat.gpsPosValid ? "true" : "false");
            break;

        case GDL90_OWNSHIP_REPORT:
            ourPosition.report = data->positionReport;
            ourPosition.timestamp = (uint32_t) (esp_timer_get_time() / 1000);
            break;
        case GDL90_TRAFFIC_REPORT:
            lastTrafficSecs = (uint32_t) (esp_timer_get_time() / 1000);
            processTraffic(&data->positionReport);
            break;

        case GDL90_OWNSHIP_GEO_ALT:
            snprintf(gdl90StatusText, sizeof(gdl90StatusText), "OwnGeoAlt @ %f",
                     data->ownshipAltitude.geoAltitude / 0.3048f);
            break;

        case GDL90_FOREFLIGHT_ID:
            snprintf(gdl90StatusText, sizeof(gdl90StatusText), "Foreflight %s/%s %s",
                     data->foreflightId.name, data->foreflightId.longName, data->foreflightId.wgs84 ? "WGS84" : "MSL");
            break;

        default:
            snprintf(gdl90StatusText, sizeof(gdl90StatusText), "Received data type %d", data->id);
            break;
    }
    //ESP_LOGI(TAG, "%s", gdl90StatusText);
    if (gdl90Status != GDL90_STATE_RX) {
        gdl90Status = GDL90_STATE_RX;
        postMessage(EVENT_GDL90_CHANGE, NULL, 0);
    }
}

bool getNextBlock(void *buffer, uint32_t delayMs) {
    return xQueueReceive(queue, buffer, delayMs / portTICK_PERIOD_MS);
}

static void gCallback(const gdlDataPacket_t *buffer) {
    if (!xQueueSend(queue, buffer, 0))
        ESP_LOGI(TAG, "Send to queue failed");
}


_Noreturn void gdlTask(void *param) {
    queue = xQueueCreate(GDL_QUEUE_LEN, sizeof(gdlDataPacket_t));
    for (;;) {
        vTaskDelay(1000 / portTICK_PERIOD_MS);      // retry after delay
        listeningSocket = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP);
        if (listeningSocket <= 0) {
            ESP_LOGI(TAG, "Error: listenForPackets - socket() failed.");
            continue;
        }
        struct timeval timeV;
        timeV.tv_sec = 2;
        timeV.tv_usec = 0;
        if (setsockopt(listeningSocket, SOL_SOCKET, SO_RCVTIMEO, &timeV, sizeof(timeV)) == -1) {
            ESP_LOGI(TAG, "Error: setsockopt failed.");
            close(listeningSocket);
            continue;
        }
        struct sockaddr_in sockaddr;
        memset(&sockaddr, 0, sizeof(sockaddr));

        sockaddr.sin_len = sizeof(sockaddr);
        sockaddr.sin_family = AF_INET;
        sockaddr.sin_port = htons(listenPort);
        sockaddr.sin_addr.s_addr = htonl(INADDR_ANY);
        int status = bind(listeningSocket, (struct sockaddr *) &sockaddr, sizeof(sockaddr));
        if (status == -1) {
            close(listeningSocket);
            ESP_LOGI(TAG, "Error: bind failed.");
            continue;
        }
        struct sockaddr_in receiveSockaddr;
        socklen_t receiveSockaddrLen = sizeof(receiveSockaddr);

        for (;;) {
            ssize_t result = recvfrom(listeningSocket, rxBuffer, sizeof(rxBuffer), 0,
                                      (struct sockaddr *) &receiveSockaddr, &receiveSockaddrLen);
            if (result > 0) {
                //ESP_LOGI(TAG, "UDP received: %d bytes.", result);
                gdl90GetBlocks(rxBuffer, result, gCallback);
            } else if (!isWifiConnected()) {
                closesocket(listeningSocket);
                break;
            }
        }
    }
}

bool isGpsConnected() {
    uint32_t now = (uint32_t) (esp_timer_get_time() / 1000);
    if (!lastHeartbeat.gpsPosValid || now > ourPosition.timestamp + MAX_AGE_MS) {
        lastHeartbeat.gpsPosValid = false;
    }
    return lastHeartbeat.gpsPosValid;
}

bool isTrafficConnected() {
    uint32_t now = (uint32_t) (esp_timer_get_time() / 1000);
    if (now > lastTrafficSecs + MAX_AGE_MS)
        gdl90Status = GDL90_STATE_WAITING;
    return gdl90Status == GDL90_STATE_RX;
}
