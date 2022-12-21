//
// Created by Clyde Stubbs on 29/11/2022.
//

#include <lwip/sockets.h>
#include <tcpip_adapter.h>
#include <esp_log.h>
#include "udp.h"
#include "main.h"
#include "wifi_prov.h"

#define TAG "udp"
static int pingPorts[] = {63093, 47578};
static const char *pingPayload = "{\"App\": \"TraffiX\","
                                 "\"GDL90\": {\"port\": 4000}}";

void pingUdp() {
    tcpip_adapter_ip_info_t ipInfo;

// IP address.
    tcpip_adapter_get_ip_info(TCPIP_ADAPTER_IF_STA, &ipInfo);

    u32_t broadcastAddress = ipInfo.ip.addr | ~ipInfo.netmask.addr;
    ESP_LOGI(TAG, "addr = %x, mask=%x, broadcast=%x\n", ipInfo.ip.addr, ipInfo.netmask.addr, broadcastAddress);
    struct sockaddr_in dest_addr;
    dest_addr.sin_addr.s_addr = broadcastAddress;
    dest_addr.sin_family = AF_INET;
    int sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_IP);
    if (sock < 0) {
        ESP_LOGE(TAG, "socket() failed with code %d", sock);
        return;
    }
    int optval = 1;
    int err;
    /*
    err = setsockopt(sock, IPPROTO_UDP, SO_BROADCAST, &optval, sizeof(int));
    if (err < 0) {
        ESP_LOGE(TAG, "setsockopt() failed with code %d", sock);
        return;
    } */
    for (int i = 0; i != ARRAY_SIZE(pingPorts); i++) {
        dest_addr.sin_port = htons(pingPorts[i]);
        err = sendto(sock, pingPayload, strlen(pingPayload), 0, (struct sockaddr *) &dest_addr, sizeof(dest_addr));
        if (err < 0) {
            ESP_LOGE(TAG, "sendto() failed with code %d", err);
            return;
        }
        ESP_LOGI(TAG, "Sent to port %d", dest_addr.sin_port);
    }
    closesocket(sock);
}

void sendFlarm() {

}
