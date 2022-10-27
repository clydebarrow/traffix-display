//
// Created by Clyde Stubbs on 26/10/2022.
//

#ifndef TRAFFIX_DISPLAY_WIFI_PROV_H
#define TRAFFIX_DISPLAY_WIFI_PROV_H

typedef enum {
    WIFI_UNKNOWN,
    WIFI_PROVISIONING,
    WIFI_CONNECTING,
    WIFI_CONNECTED,
} wifiState_t;

extern wifiState_t wifiState;

#endif //TRAFFIX_DISPLAY_WIFI_PROV_H
