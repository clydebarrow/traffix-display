#include <sys/cdefs.h>
//
// Created by Clyde Stubbs on 23/12/2022.
//

#include <hal/uart_types.h>
#include <driver/uart.h>
#include <string.h>
#include <math.h>
#include <lwip/sockets.h>
#include <esp_log.h>

#include "gdltask.h"
#include "flarm.h"
#include "ownship.h"
#include "main.h"
#include "wifi_prov.h"


#define FLARM_UART  UART_NUM_1
#define TAG "FLARM"

static int sock;

preference_t prefFlarmBaudRate = {
        // Discard traffic if no data after this many seconds
        "prefFlarmBaud",
        "Transmit baud rate",
        PREF_INT32,
        {.intValue = 19200},
        {},
        {}
};

preference_t prefUDPPort = {
        // Discard traffic if no data after this many seconds
        "prefUDPPort",
        "UDP Broadcast port",
        PREF_INT32,
        {.intValue = 4353},
        {},
        {}
};

#define UART_BUFFER_SIZE (1024 * 2)

static char nmeabuf[160];

/**
 * Add a checksum to the buffer. The first charcter ($) is ignored.
 * @param buffer
 */
int checksum() {
    uint8_t sum = 0;
    size_t len = strlen(nmeabuf);
    if (len == 0 || sizeof nmeabuf < len + 6)
        return false;
    for (size_t i = 1; i != len; i++)
        sum ^= nmeabuf[i];
    sprintf(nmeabuf + len, "*%02X\r\n", sum);
    return true;
}

static int accuracyMap[] = {
        -1,
        18520,
        7408,
        3704,
        1852,
        926,
        555,
        185,
        92,
        30,
        10,
        3,
};

static int getAccuracy(int nacP) {

    if (nacP < 0 || nacP > ARRAY_SIZE(accuracyMap))
        return -1;
    return accuracyMap[nacP];
}

void sendData() {
    checksum();
    struct sockaddr_in sendAddr;
    sendAddr.sin_addr.s_addr = broadcastAddr;
    sendAddr.sin_family = AF_INET;
    sendAddr.sin_port = htons(prefUDPPort.currentValue.intValue);
    ESP_LOGI(TAG, "FLARM data: %s", nmeabuf);
    uart_write_bytes(FLARM_UART, nmeabuf, strlen(nmeabuf));
    sendto(sock, nmeabuf, strlen(nmeabuf), 0, (struct sockaddr *) &sendAddr, sizeof(sendAddr));
}

void gpgga() {
    gdl90Heartbeat_t heartbeat;
    gpsStatus_t status;
    ownship_t ownship;
    bool valid = (int) getHeartbeat(&heartbeat) | getStatus(&status) | getOwnshipPosition(&ownship);

    float absLat = fabsf(ownship.report.latitude);
    int latDeg = (int) absLat;
    float absLon = fabsf(ownship.report.longitude);
    int lonDeg = (int) absLon;

    snprintf(nmeabuf, sizeof nmeabuf,
             "$GPGGA,%02u%02u%02u,%02d%05.2f,%c,%02d%05.2f,%c,%d,%02d,%d,%3.1f,M,%3.1f,M,,",
             (unsigned int) (heartbeat.timeStamp / 3600),
             (unsigned int) ((heartbeat.timeStamp / 60) % 60),
             (unsigned int) (heartbeat.timeStamp % 60),
             latDeg,
             (absLat - latDeg) * 60,
             ownship.report.latitude < 0 ? 'S' : 'N',
             lonDeg,
             (absLon - lonDeg) * 60,
             ownship.report.longitude < 0 ? 'W' : 'E',
             valid ? 1 : 0,
             status.satellitesUsed,
             getAccuracy(ownship.report.NACp),
             ownship.report.altitude,
             status.geoAltitude
    );
    sendData();
}

void gprmc() {
    gdl90Heartbeat_t heartbeat;
    gpsStatus_t status;
    ownship_t ownship;
    bool valid = (int) getHeartbeat(&heartbeat) | getStatus(&status) | getOwnshipPosition(&ownship);

    float absLat = fabsf(ownship.report.latitude);
    int latDeg = (int) absLat;
    float absLon = fabsf(ownship.report.longitude);
    int lonDeg = (int) absLon;
    struct tm ts;
    time_t utc = status.gpsTime + (315964800 + 18);
    ts = *gmtime(&utc);
    snprintf(nmeabuf, sizeof nmeabuf,
             "$GPRMC,%02u%02u%02u,%c,%02d%05.2f,%c,%03d%05.2f,%c,%05.1f,%05.1f,%02d%02d%02d,%05.1f,%c",
             (unsigned int) (heartbeat.timeStamp / 3600),
             (unsigned int) ((heartbeat.timeStamp / 60) % 60),
             (unsigned int) (heartbeat.timeStamp % 60),
             valid ? 'A' : 'V',
             latDeg,
             (absLat - latDeg) * 60,
             ownship.report.latitude < 0 ? 'S' : 'N',
             lonDeg,
             (absLon - lonDeg) * 60,
             ownship.report.longitude < 0 ? 'W' : 'E',
             ownship.report.groundSpeed,
             ownship.report.track,
             ts.tm_mday,
             ts.tm_mon + 1,
             ts.tm_year % 100,
             0.0f, 'E' //if (variation > 0) 'E' else 'W'
    );
    sendData();
}

void pgrmz() {
    gpsStatus_t status;
    getStatus(&status);
    snprintf(nmeabuf, sizeof nmeabuf, "$PGRMZ,%d,F,2", (int) (status.baroAltitude / .3048));
    sendData();
}

void pflaa() {
    traffic_t traffic[MAX_TARGETS_SHOWN];
    ownship_t ourPosition;
    getOwnshipPosition(&ourPosition);
    getTraffic(traffic, MAX_TARGETS_SHOWN);
    for (int i = 0; i != MAX_TARGETS_SHOWN; i++) {
        traffic_t *target = traffic + i;
        if (!target->active)
            continue;

        //PFLAA,<AlarmLevel>,<RelativeNorth>,<RelativeEast>, <RelativeVertical>,<IDType>,<ID>,<Track>,<TurnRate>,<GroundSpeed>, <ClimbRate>,<AcftType>

        snprintf(nmeabuf, sizeof nmeabuf,
                 "$PFLAA,%d,%d,%d,%d,1,%06X,%d,,%d,%d,%X",
                 0,  // alarm level
                 (int) trafficNorthing(&ourPosition.report, &target->report),
                 (int) trafficEasting(&ourPosition.report, &target->report),
                 (int) (target->report.altitude - ourPosition.report.altitude),
                 (unsigned int)target->report.address,
                 (int) target->report.track,
                 (int) target->report.groundSpeed,
                 (int) target->report.verticalSpeed,
                 target->report.category
        );
        sendData();
    }
}

_Noreturn static void flarmTask(__attribute__((unused)) void *param) {
    uart_config_t uartConfig = {
            .baud_rate = prefFlarmBaudRate.currentValue.intValue,
            .data_bits = UART_DATA_8_BITS,
            .parity = UART_PARITY_DISABLE,
            .stop_bits = UART_STOP_BITS_1,
            .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
            .source_clk = UART_SCLK_DEFAULT
    };
    ESP_ERROR_CHECK(uart_param_config(FLARM_UART, &uartConfig));
    ESP_ERROR_CHECK(uart_set_pin(FLARM_UART, 17, 18, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE));
    QueueHandle_t uart_queue;
    ESP_ERROR_CHECK(uart_driver_install(FLARM_UART, UART_BUFFER_SIZE,
                                        UART_BUFFER_SIZE, ESP_INTR_FLAG_SHARED | ESP_INTR_FLAG_LEVEL2, &uart_queue, 0));

// IP address.
    sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_IP);

    if (sock < 0) {
        ESP_LOGE(TAG, "socket() failed with code %d", sock);
    }
    /*
    int optval = 1;
    err = setsockopt(sock, IPPROTO_UDP, SO_BROADCAST, &optval, sizeof(int));
    if (err < 0) {
        ESP_LOGE(TAG, "setsockopt() failed with code %d", sock);
        return;
    } */
    ESP_LOGI(TAG, "Flarm task started");
    for (;;) {
        int64_t started = esp_timer_get_time();
        if (isWifiConnected()) {
            gpgga();
            gprmc();
            pgrmz();
            pflaa();
        }
        TickType_t delay = (started + 1000000ll - esp_timer_get_time()) / 1000 / portTICK_PERIOD_MS;
        vTaskDelay(delay);
    }
}

void flarmInit() {
    xTaskCreatePinnedToCore(flarmTask, "FLARM", 6000, NULL, 4, NULL, 1);
}