//
// Created by Clyde Stubbs on 24/10/2022.
//
#include <esp_err.h>
#include <esp_netif.h>
#include <esp_event.h>
#include <freertos/event_groups.h>
#include <wifi_provisioning/manager.h>
#include <esp_wifi_default.h>
#include <esp_wifi.h>
#include <wifi_provisioning/scheme_ble.h>
#include <esp_log.h>
#include <string.h>
#include <lvgl.h>
#include "main.h"
#include "ui.h"
#include "wifi_prov.h"
#include "events.h"

static const char *TAG = "WiFi";

wifiState_t wifiState;

static lv_obj_t *qrCodeImage;
static lv_obj_t *provLabel;
static lv_obj_t *connectLabel;
static lv_style_t titleStyle;

/* Signal Wi-Fi events on this event-group */

static void event_handler(void *arg, esp_event_base_t event_base,
                          int32_t event_id, void *event_data) {
    if (event_base == WIFI_PROV_EVENT) {
        switch (event_id) {
            case WIFI_PROV_START:
                ESP_LOGI(TAG, "Provisioning started");
                break;
            case WIFI_PROV_CRED_RECV: {
                wifi_sta_config_t *wifi_sta_cfg = (wifi_sta_config_t *) event_data;
                ESP_LOGI(TAG, "Received Wi-Fi credentials"
                              "\n\tSSID     : %s\n\tPassword : %s",
                         (const char *) wifi_sta_cfg->ssid,
                         (const char *) wifi_sta_cfg->password);
                wifiState = WIFI_CONNECTING;
                postMessage(EVENT_WIFI_CHANGE, NULL, 0);
                break;
            }
            case WIFI_PROV_CRED_FAIL: {
                wifi_prov_sta_fail_reason_t *reason = (wifi_prov_sta_fail_reason_t *) event_data;
                ESP_LOGE(TAG, "Provisioning failed!\n\tReason : %s"
                              "\n\tPlease reset to factory and retry provisioning",
                         (*reason == WIFI_PROV_STA_AUTH_ERROR) ?
                         "Wi-Fi station authentication failed" : "Wi-Fi access-point not found");
                break;
            }
            case WIFI_PROV_CRED_SUCCESS:
                ESP_LOGI(TAG, "Provisioning successful");
                break;
            case WIFI_PROV_END:
                /* De-initialize manager once provisioning is finished */
                wifi_prov_mgr_deinit();
                break;
            default:
                break;
        }
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t *event = (ip_event_got_ip_t *) event_data;
        ESP_LOGI(TAG, "Connected with IP Address:" IPSTR, IP2STR(&event->ip_info.ip));
        /* Signal main application to continue execution */
        wifiState = WIFI_CONNECTED;
        postMessage(EVENT_WIFI_CHANGE, NULL, 0);
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        ESP_LOGI(TAG, "Disconnected. Connecting to the AP again...");
        wifiState = WIFI_CONNECTING;
        esp_wifi_connect();
        postMessage(EVENT_WIFI_CHANGE, NULL, 0);
    }
}

static void get_device_service_name(char *service_name, size_t max) {
    uint8_t eth_mac[6];
    const char *ssid_prefix = "TFX_";
    esp_wifi_get_mac(WIFI_IF_STA, eth_mac);
    snprintf(service_name, max, "%s%02X%02X%02X",
             ssid_prefix, eth_mac[3], eth_mac[4], eth_mac[5]);
}

/**
 * One-time WiFi initialisation
 */
void initWiFi() {
    ESP_ERROR_CHECK(esp_netif_init());
    wifiState = WIFI_UNKNOWN;
    /* Register our event handler for Wi-Fi, IP and Provisioning related events */
    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_PROV_EVENT, ESP_EVENT_ANY_ID, &event_handler, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &event_handler, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &event_handler, NULL));

    /* Initialize Wi-Fi including netif with default config */
    esp_netif_create_default_wifi_sta();
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
}


static char payload[150];

/**
 * Provision and connect WiFi.
 *
 * If not already provisioned, display QR code.
 */

void provisionWiFi(bool force) {

    /* Configuration for the provisioning manager */
    wifi_prov_mgr_config_t config = {
            .scheme = wifi_prov_scheme_ble,
            .scheme_event_handler = WIFI_PROV_SCHEME_BLE_EVENT_HANDLER_FREE_BTDM
    };
    ESP_ERROR_CHECK(wifi_prov_mgr_init(config));
    bool provisioned = false;
    if (force)
        wifi_prov_mgr_reset_provisioning();
    else
        ESP_ERROR_CHECK(wifi_prov_mgr_is_provisioned(&provisioned));

    if (!provisioned) {
        ESP_LOGI(TAG, "Starting provisioning");

        char service_name[12];
        get_device_service_name(service_name, sizeof(service_name));

        /*
         *      - WIFI_PROV_SECURITY_1 is secure communication which consists of secure handshake
         */
        wifi_prov_security_t security = WIFI_PROV_SECURITY_1;

        /* Do we want a proof-of-possession (ignored if Security 0 is selected):
         *      - this should be a string with length > 0
         *      - NULL if not used
         */
        char pop[12];
        uint32_t rand = esp_random();
        snprintf(pop, sizeof(pop), "TFX%08X", rand);

        uint8_t custom_service_uuid[16] = {
                /* LSB <---------------------------------------
                 * ---------------------------------------> MSB */
                0xb4, 0xdf, 0x5a, 0x1c, 0x3f, 0x6b, 0xf4, 0xbf,
                0xea, 0x4a, 0x82, 0x03, 0x04, 0x90, 0x1a, 0x02,
        };

        snprintf(payload, sizeof(payload), "{\"ver\":\"%s\",\"name\":\"%s\"" \
                    ",\"pop\":\"%s\",\"transport\":\"%s\"}",
                 "v1", service_name, pop, "ble");
        /* If your build fails with linker errors at this point, then you may have
         * forgotten to enable the BT stack or BTDM BLE settings in the SDK (e.g. see
         * the sdkconfig.defaults in the example project) */
        wifi_prov_scheme_ble_set_service_uuid(custom_service_uuid);
        ESP_ERROR_CHECK(wifi_prov_mgr_start_provisioning(security, pop, service_name, NULL));
        wifiState = WIFI_PROVISIONING;
    } else {
        wifi_prov_mgr_deinit();
        ESP_LOGI(TAG, "WiFi already provisioned");
        wifi_config_t wifi_cfg;
        esp_wifi_get_config(WIFI_IF_STA, &wifi_cfg);
        ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
        ESP_ERROR_CHECK(esp_wifi_start());
        wifiState = WIFI_CONNECTING;
    }
    postMessage(EVENT_WIFI_CHANGE, NULL, 0);
}

static void initStyle() {
    lv_style_init(&titleStyle);
    lv_style_set_text_font(&titleStyle, &lv_font_montserrat_20);
    lv_style_set_bg_color(&titleStyle, lv_color_black());
    lv_style_set_text_color(&titleStyle, lv_color_white());
    lv_style_set_text_align(&titleStyle, LV_TEXT_ALIGN_CENTER);
}
/**
 * Show a QR code for provisioning
 * @param tv    The graphics context to show on
 */
static void qrShow(lv_obj_t *tv) {
    ESP_LOGI(TAG, "Show QR payload = %s", payload);
    qrCodeImage = lv_qrcode_create(tv, 150, lv_color_white(), lv_color_black());
    lv_obj_align(qrCodeImage, LV_ALIGN_LEFT_MID, 0, 0);
    lv_obj_set_style_border_color(qrCodeImage, lv_color_black(), 0);
    lv_obj_set_style_border_width(qrCodeImage, 5, 0);
    lv_qrcode_update(qrCodeImage, payload, strlen(payload));
    initStyle();
    provLabel = lv_label_create(tv);
    lv_obj_add_style(provLabel,&titleStyle, 0);
    lv_obj_set_style_border_color(provLabel, lv_color_black(), 0);
    lv_obj_set_style_border_width(provLabel, 2, 0);
    lv_obj_align(provLabel, LV_ALIGN_RIGHT_MID, 0, 0);
    lv_label_set_text(provLabel, "Provision\nWiFi with\nESP Ble App");
    ESP_LOGI(TAG, "qrShow done");
}

/**
 * Show a message "Connecting to wifi"
 * @param tv 
 */
static void connectShow(lv_obj_t *tv) {
    connectLabel = lv_label_create(tv);
    initStyle();
    lv_obj_center(connectLabel);
    lv_obj_add_style(connectLabel, &titleStyle, 0);

    wifi_config_t wifi_cfg;
    esp_wifi_get_config(WIFI_IF_STA, &wifi_cfg);
    lv_label_set_text_fmt(connectLabel, "Connecting to WiFi\nSSID: %.32s", wifi_cfg.sta.ssid);
}

const uiScreen_t uiWiFiProvision = {
        .name = "WiFi Provision",
        .setup = qrShow,
        .update = NULL,
        .teardown = NULL
};
const uiScreen_t uiWiFiConnect = {
        .name = "WiFi Connect",
        .setup = connectShow,
        .update = NULL,
        .teardown = NULL
};
