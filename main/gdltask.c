#include <sys/cdefs.h>
#include <stdio.h>
#include <sys/socket.h>
#include <esp_log.h>
#include <esp_websocket_client.h>
#include <cJSON.h>
#include "gdltask.h"
#include "preferences.h"
#include "events.h"
#include "wifi_prov.h"
#include "traffic.h"
#include "main.h"
#include "ownship.h"

#define MAX_AGE_MS (prefTrafficTimeout.currentValue.intValue*1000)      // consider traffic disconnected if nothing received for this many ms

static const char *TAG = "GDL90";
static int listenPort = 4000;
static int listeningSocket;
static uint8_t rxBuffer[1600];

static gdl90Status_t gdl90Status;
//static char gdl90StatusText[256];
static uint32_t lastTrafficMs;
static bool websocketOpen;
static esp_websocket_client_handle_t clientHandle;

static void rxJson(const char *data, size_t len) {
    cJSON *root = cJSON_ParseWithLength(data, len);

    if (root == NULL)
        return;
    cJSON *statusItem = cJSON_GetObjectItem(root, "status");
    if (statusItem == NULL) {
        cJSON_Delete(statusItem);
        return;
    }
    /*
     {
  "clientCount": 1,
  "status": {
    "icaoAddress": 8142385,
    "callsign": "MKJ",
    "latitude": -314210048,
    "longitude": 1527573248,
    "baroAltitude": -56028,
    "gnssAltitude": 43350,
    "gpsFix": 3,
    "gpsSatellites": 6,
    "NACp": 8,
    "NIC": 6
  }
}
     */
    gpsStatus_t status;
    cJSON *value;

    value = cJSON_GetObjectItem(statusItem, "gpsFix");
    status.gpsFix = value != NULL && cJSON_IsNumber(value) ? value->valueint : GPS_FIX_NONE;
    value = cJSON_GetObjectItem(statusItem, "gpsSatellites");
    status.satellitesUsed = value != NULL && cJSON_IsNumber(value) ? value->valueint : 0;
    value = cJSON_GetObjectItem(statusItem, "baroAltitude");
    status.baroAltitude = value != NULL && cJSON_IsNumber(value) ? value->valueint / 1000.0f : 0.0f;
    value = cJSON_GetObjectItem(statusItem, "gnssAltitude");
    status.geoAltitude = value != NULL && cJSON_IsNumber(value) ? value->valueint / 1000.0f : 0.0f;
    value = cJSON_GetObjectItem(statusItem, "NACp");
    status.NACp = value != NULL && cJSON_IsNumber(value) ? value->valueint : 0;
    cJSON_Delete(root);
    printf("Satellites: %d, gpsFix: %d, baroAltitude: %f, geoAltitude: %f, NACp: %d\n",
           status.satellitesUsed, status.gpsFix, status.baroAltitude, status.geoAltitude, status.NACp);
    setStatus(&status);
}

static void websocketHandler(
        void *event_handler_arg,
        esp_event_base_t event_base,
        int32_t event_id,
        void *event_data) {

    esp_websocket_event_data_t *data = (esp_websocket_event_data_t *) event_data;
    //printf("Websocket event %d\n", event_id);
    switch (event_id) {
        case WEBSOCKET_EVENT_CONNECTED:
            websocketOpen = true;
            break;

        case WEBSOCKET_EVENT_CLOSED:
            esp_websocket_client_destroy(clientHandle);
            clientHandle = NULL;
            break;

        case WEBSOCKET_EVENT_DATA:
            if (data->op_code == 1 || data->op_code == 2) {
                //printf("received %.*s from websocket\n", data->data_len, data->data_ptr);
                rxJson(data->data_ptr, data->data_len);
            }
            break;

        case WEBSOCKET_EVENT_DISCONNECTED:
            websocketOpen = false;
            break;
    }
}

static void openWebsocket(struct in_addr *srcAddr, int port) {
    char addr[40];
    char url[128];
    inet_ntop(AF_INET, srcAddr, addr, sizeof(addr));
    snprintf(url, sizeof url, "ws://%s:%d", addr, port);
    const esp_websocket_client_config_t ws_cfg = {
            .uri = url
    };
    websocketOpen = true;
    clientHandle = esp_websocket_client_init(&ws_cfg);
    esp_websocket_register_events(clientHandle, WEBSOCKET_EVENT_ANY, websocketHandler, NULL);
    ESP_ERROR_CHECK(esp_websocket_client_start(clientHandle));
}

/**
 * Process a GDL 90 packet
 * @param packet The packet
 */
void processPacket(gdl90Data_t *packet, struct in_addr *srcAddr) {
    switch (packet->id) {
        case GDL90_HEARTBEAT:
            setHeartbeat(&packet->heartbeat);
            //snprintf(gdl90StatusText, sizeof(gdl90StatusText), "Heartbeat @%d GPSvalid %s", packet->heartbeat.timeStamp, //packet->heartbeat.gpsPosValid ? "true" : "false");
            break;

        case GDL90_OWNSHIP_REPORT:
            setOwnshipPosition(&packet->positionReport);
            break;
        case GDL90_TRAFFIC_REPORT:
            lastTrafficMs = (uint32_t) (esp_timer_get_time() / 1000);
            processTraffic(&packet->positionReport);
            break;

        case GDL90_OWNSHIP_GEO_ALT:
            //snprintf(gdl90StatusText, sizeof(gdl90StatusText), "OwnGeoAlt @ %f", packet->ownshipAltitude.geoAltitude / 0.3048f);
            break;

        case GDL90_FOREFLIGHT_ID:
            if (!websocketOpen && strncmp(packet->foreflightId.name, "SkyEcho", 7) == 0)
                openWebsocket(srcAddr, 80);
            else if (!websocketOpen && strncmp(packet->foreflightId.name, "Sim:", 4) == 0) {
                int port = atoi(packet->foreflightId.name + 4);
                openWebsocket(srcAddr, port);
            }
            //snprintf(gdl90StatusText, sizeof(gdl90StatusText), "Foreflight %s/%s %s", packet->foreflightId.name, packet->foreflightId.longName, packet->foreflightId.wgs84 ? "WGS84" : "MSL");
            break;

        default:
            //printf("Received unhandled packet type %d", packet->id);
            break;
    }
    //ESP_LOGI(TAG, "%s", gdl90StatusText);
    if (gdl90Status != GDL90_STATE_RX) {
        gdl90Status = GDL90_STATE_RX;
        postMessage(EVENT_GDL90_CHANGE, NULL, 0);
    }
}

/**
 * Receive an unescaped GDL90 packet from the input and process it.
 * @param packet The packet to process
 * @return
 */

static void gCallback(const gdlDataPacket_t *packet, struct in_addr *srcAddr) {
    gdl90Data_t data;
    if (packet->err != GDL90_ERR_NONE) {
        ESP_LOGI(TAG, "DecodeBlock failed - %s: id %d, len %d",
                 gdl90ErrorMessage(packet->err), packet->data[0], packet->len);
    } else {
        memset(&data, 0, sizeof(data));
        gdl90Err_t err = gdl90DecodeBlock(packet->data, packet->len, &data);
        if (err == GDL90_ERR_NONE) {
            processPacket(&data, srcAddr);
        } else
            ESP_LOGI(TAG, "Decode packet id %d failed with err %s", data.id, gdl90ErrorMessage(err));
    }
}


_Noreturn void gdlTask(void *param) {
    for (;;) {
        ESP_LOGI(TAG, "UDP task restarts");
        vTaskDelay(2000 / portTICK_PERIOD_MS);      // retry after delay
        if (!isWifiConnected())
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
        struct sockaddr_in sourceAddr;
        socklen_t sourceAddrLen = sizeof(sourceAddr);

        for (;;) {
            // close websocket if disconnected
            if (!websocketOpen && clientHandle != NULL) {
                esp_websocket_client_close(clientHandle, 1);
            }
            ssize_t result = recvfrom(listeningSocket, rxBuffer, sizeof(rxBuffer), 0,
                                      (struct sockaddr *) &sourceAddr, &sourceAddrLen);
            if (result > 0) {
                //ESP_LOGI(TAG, "UDP received: %d bytes.", result);
                gdl90GetBlocks(rxBuffer, result, gCallback, &sourceAddr.sin_addr);
            } else if (!isWifiConnected()) {
                closesocket(listeningSocket);
                if (clientHandle != NULL && !websocketOpen) {
                    esp_websocket_client_close(clientHandle, 1);
                    websocketOpen = false;
                    clientHandle = NULL;
                }
                ESP_LOGI(TAG, "UDP task closed socket");
                break;
            }
        }
    }
}


bool isGdl90TrafficConnected() {
    if (getNow() > lastTrafficMs + MAX_AGE_MS)
        gdl90Status = GDL90_STATE_WAITING;
    return gdl90Status == GDL90_STATE_RX;
}

void gdlInit() {
    xTaskCreatePinnedToCore(gdlTask, "GDLrx", 3000, NULL, 4, NULL, 0);
}
