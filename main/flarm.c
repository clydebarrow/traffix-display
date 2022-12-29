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

/**
 * GPGGA generation
 */

/*
         return "GPGGA,%02d%02d%02d,%02d%05.2f,%c,%02d%05.2f,%c,%d,%02d,%.1f,%.1f,M,,M,,".format(
            stamp.get(ChronoField.HOUR_OF_DAY),
            stamp.get(ChronoField.MINUTE_OF_HOUR),
            stamp.get(ChronoField.SECOND_OF_MINUTE),
            latDeg, (absLat - latDeg) * 60, if (location.latitude < 0) 'S' else 'N',
            lonDeg, (absLon - lonDeg) * 60, if (location.longitude > 0) 'E' else 'W',
            if (location.gpsFix >= GpsFix.ThreeD) 1 else 0,
            location.satellitesUsed,
            location.accuracyH / 5.0,       // estimate HDOP
            location.altitude,

 */

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

bool gpgga() {
    gdl90Heartbeat_t heartbeat;
    gpsStatus_t status;
    ownship_t ownship;
    bool valid = (int) getHeartbeat(&heartbeat) | getStatus(&status) | getOwnshipPosition(&ownship);

    float absLat = fabsf(ownship.report.latitude);
    int latDeg = (int) absLat;
    float absLon = fabsf(ownship.report.longitude);
    int lonDeg = (int) absLon;

    snprintf(nmeabuf, sizeof nmeabuf,
             "$GPGGA,%02d%02d%02d,%02d%05.2f,%c,%02d%05.2f,%c,%d,%02d,%d,%3.1f,M,%3.1f,M,,",
             heartbeat.timeStamp / 3600,
             (heartbeat.timeStamp / 60) % 60,
             heartbeat.timeStamp % 60,
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
    return true;
}

void gprmc() {
    /*
     *     private fun gprmc(location: Location): String {
        val absLat = abs(location.latitude)
        val latDeg = absLat.toInt()
        val absLon = abs(location.longitude)
        val lonDeg = absLon.toInt()
        val variation = location.magneticVariation
        val absVar = abs(variation)
        val stamp = location.timestamp.atOffset(ZoneOffset.UTC)
        return "GPRMC,%02d%02d%02d,%c,%02d%05.2f,%c,%03d%05.2f,%c,%05.1f,%05.1f,%02d%02d%02d,%05.1f,%c".format(
            stamp.get(ChronoField.HOUR_OF_DAY),
            stamp.get(ChronoField.MINUTE_OF_HOUR),
            stamp.get(ChronoField.SECOND_OF_MINUTE),
            if (location.valid) 'A' else 'V',
            latDeg, (absLat - latDeg) * 60, if (location.latitude < 0) 'S' else 'N',
            lonDeg, (absLon - lonDeg) * 60, if (location.longitude > 0) 'E' else 'W',
            location.speed,
            location.track,
            stamp.get(ChronoField.DAY_OF_MONTH),
            stamp.get(ChronoField.MONTH_OF_YEAR),
            stamp.get(ChronoField.YEAR) % 100,
            absVar, if (variation > 0) 'E' else 'W'
        ).checkSummed()

     */
    gdl90Heartbeat_t heartbeat;
    gpsStatus_t status;
    ownship_t ownship;
    bool valid = (int) getHeartbeat(&heartbeat) | getStatus(&status) | getOwnshipPosition(&ownship);

    float absLat = fabsf(ownship.report.latitude);
    int latDeg = (int) absLat;
    float absLon = fabsf(ownship.report.longitude);
    int lonDeg = (int) absLon;
    snprintf(nmeabuf, sizeof nmeabuf,
    "GPRMC,%02d%02d%02d,%c,%02d%05.2f,%c,%03d%05.2f,%c,%05.1f,%05.1f,%02d%02d%02d,%05.1f,%c".format(
            heartbeat.timeStamp / 3600,
            (heartbeat.timeStamp / 60) % 60,
            heartbeat.timeStamp % 60,
            valid? 'A' : 'V',
            latDeg,
            (absLat - latDeg) * 60,
            ownship.report.latitude < 0 ? 'S' : 'N',
            lonDeg,
            (absLon - lonDeg) * 60,
            ownship.report.longitude < 0 ? 'W' : 'E',
                ownship.report.groundSpeed,
                ownship.report.track,
                stamp.get(ChronoField.DAY_OF_MONTH),
                stamp.get(ChronoField.MONTH_OF_YEAR),
                stamp.get(ChronoField.YEAR) % 100,
                absVar, if (variation > 0) 'E' else 'W'

}

void sendData() {
    checksum();
    struct sockaddr_in sendAddr;
    sendAddr.sin_addr.s_addr = broadcastAddr;
    sendAddr.sin_family = AF_INET;
    sendAddr.sin_port = htons(prefUDPPort.currentValue.intValue);
    ESP_LOGI(TAG, "FLARM data: %s", nmeabuf);
    uart_write_bytes(FLARM_UART, nmeabuf, strlen(nmeabuf));
    int result = sendto(sock, nmeabuf, strlen(nmeabuf), 0, (struct sockaddr *) &sendAddr, sizeof(sendAddr));
    printf("sendto port = %d, result = %d, errno = %d\n", sendAddr.sin_port, result, errno);
}

_Noreturn static void flarmTask(__attribute__((unused)) void *param) {
    uint64_t nextUpdateMs = 0;
    uart_config_t uartConfig = {
            .baud_rate = prefFlarmBaudRate.currentValue.intValue,
            .data_bits = UART_DATA_8_BITS,
            .parity = UART_PARITY_DISABLE,
            .stop_bits = UART_STOP_BITS_1,
            .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
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
    for (;;) {
        int delay = (int) (nextUpdateMs - esp_timer_get_time() / 1000 )/ portTICK_PERIOD_MS;
        nextUpdateMs = esp_timer_get_time() / 1000  + 1000;
        if (delay > 0)
            vTaskDelay(delay);
        if (isWifiConnected()) {
            gpgga();
            sendData();
        }
    }
}

void flarmInit() {
    xTaskCreatePinnedToCore(flarmTask, "FLARM", 6000, NULL, 4, NULL, 1);
}