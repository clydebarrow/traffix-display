//
// Created by Clyde Stubbs on 26/10/2022.
//

#ifndef TRAFFIX_DISPLAY_WIFI_PROV_H
#define TRAFFIX_DISPLAY_WIFI_PROV_H

#include <esp_netif.h>
typedef enum {
    WIFI_UNKNOWN,
    WIFI_PROVISIONING,
    WIFI_CONNECTING,
    WIFI_CONNECTED,
} wifiState_t;

extern in_addr_t broadcastAddr;
extern wifiState_t wifiState;

extern bool isWifiConnected();

extern void initWiFi();

extern void provisionWiFi(bool force);

#endif //TRAFFIX_DISPLAY_WIFI_PROV_H
