#include <sys/cdefs.h>
#include <stdio.h>
#include <sys/socket.h>
#include <esp_log.h>
#include "gdltask.h"
#include "preferences.h"
#include "../../main/events.h"
#include "wifi_prov.h"
#include "traffic.h"
#include "main.h"

#define MAX_AGE_MS (prefTrafficTimeout.currentValue.intValue*1000)      // consider traffic disconnected if nothing received for this many ms
#define GDL_QUEUE_LEN   20      // blocks to buffer in the queue

static const char *TAG = "GDL90";
static int listenPort = 4000;
static int listeningSocket;
static uint8_t rxBuffer[1600];

static gdl90Status_t gdl90Status;
static char gdl90StatusText[256];
static uint32_t lastTrafficMs;
static QueueHandle_t queue;

traffic_t ourPosition;
gdl90Heartbeat_t lastHeartbeat;

/**
 * Process a GDL 90 packet
 * @param packet The packet
 */
void processPacket(gdl90Data_t *packet) {
    switch (packet->id) {
        case GDL90_HEARTBEAT:
            lastHeartbeat = packet->heartbeat;
            snprintf(gdl90StatusText, sizeof(gdl90StatusText), "Heartbeat @%d GPSvalid %s", packet->heartbeat.timeStamp,
                     packet->heartbeat.gpsPosValid ? "true" : "false");
            break;

        case GDL90_OWNSHIP_REPORT:
            ourPosition.report = packet->positionReport;
            ourPosition.timestampMs = (uint32_t) (esp_timer_get_time() / 1000);
            break;
        case GDL90_TRAFFIC_REPORT:
            lastTrafficMs = (uint32_t) (esp_timer_get_time() / 1000);
            processTraffic(&packet->positionReport);
            break;

        case GDL90_OWNSHIP_GEO_ALT:
            snprintf(gdl90StatusText, sizeof(gdl90StatusText), "OwnGeoAlt @ %f",
                     packet->ownshipAltitude.geoAltitude / 0.3048f);
            break;

        case GDL90_FOREFLIGHT_ID:
            snprintf(gdl90StatusText, sizeof(gdl90StatusText), "Foreflight %s/%s %s",
                     packet->foreflightId.name, packet->foreflightId.longName, packet->foreflightId.wgs84 ? "WGS84" : "MSL");
            break;

        default:
            snprintf(gdl90StatusText, sizeof(gdl90StatusText), "Received packet type %d", packet->id);
            break;
    }
    //ESP_LOGI(TAG, "%s", gdl90StatusText);
    if (gdl90Status != GDL90_STATE_RX) {
        gdl90Status = GDL90_STATE_RX;
        postMessage(EVENT_GDL90_CHANGE, NULL, 0);
    }
}

/**
 * Receive a GDL90 packet from the queue and process it.
 * @param delayMs length of time to block waiting for a packet.
 * @return
 */
void dequeueGdl90Packet(uint32_t delayMs) {
    gdlDataPacket_t packet;
    gdl90Data_t data;
    if (xQueueReceive(queue, &packet, delayMs / portTICK_PERIOD_MS)) {
        if (packet.err != GDL90_ERR_NONE) {
            ESP_LOGI(TAG, "DecodeBlock failed - %s: id %d, len %d",
                     gdl90ErrorMessage(packet.err), packet.data[0], packet.len);
        } else {
            memset(&data, 0, sizeof(data));
            gdl90Err_t err = gdl90DecodeBlock(packet.data, packet.len, &data);
            if (err == GDL90_ERR_NONE) {
                processPacket(&data);
            } else
                ESP_LOGI(TAG, "Decode packet id %d failed with err %s", data.id, gdl90ErrorMessage(err));
        }
    }
}

static void gCallback(const gdlDataPacket_t *buffer) {
    if (!xQueueSend(queue, buffer, 0))
        ESP_LOGI(TAG, "Send to queue failed");
}


_Noreturn void gdlTask(void *param) {
    queue = xQueueCreate(GDL_QUEUE_LEN, sizeof(gdlDataPacket_t));
    for (;;) {
        ESP_LOGI(TAG, "UDP task restarts");
        vTaskDelay(2000 / portTICK_PERIOD_MS);      // retry after delay
        if(!isWifiConnected())
            continue;
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
                ESP_LOGI(TAG, "UDP task closed socket");
                break;
            }
        }
    }
}


bool isGpsConnected() {
    if (!lastHeartbeat.gpsPosValid || getNow() > ourPosition.timestampMs + MAX_AGE_MS) {
        lastHeartbeat.gpsPosValid = false;
    }
    return lastHeartbeat.gpsPosValid;
}

bool isTrafficConnected() {
    if (getNow() > lastTrafficMs + MAX_AGE_MS)
        gdl90Status = GDL90_STATE_WAITING;
    return gdl90Status == GDL90_STATE_RX;
}
