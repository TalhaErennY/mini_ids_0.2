#include <stdio.h>
#include <string.h>
#include <inttypes.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_wifi.h"
#include "esp_event.h"
#include "nvs_flash.h"
#include "esp_log.h"
#include "esp_timer.h"

static const char *TAG = "WIFI_IDS";

//sample rate
#define WINDOW_MS 5000
#define MAX_NETWORKS 32 // max 32 access points

//security types
typedef enum {
    SEC_OPEN,
    SEC_WEP_WPA1,
    SEC_WPA2_WPA3,
    SEC_UNKNOWN
} security_t;

typedef struct {
    bool used;
    uint8_t bssid[6];
    char ssid[33];
    security_t sec;
    
    uint32_t total;
    uint32_t beacon;
    uint32_t deauth;
} network_t;

static network_t networks[MAX_NETWORKS] = {0};

// is the same mac adress
bool mac_equal(const uint8_t *a, const uint8_t *b) {
    for(int i=0; i<6; i++) {
        if(a[i] != b[i]) {
            return false;
        }
    }

    return true;
}

//mac to string
void mac_to_str(const uint8_t *mac, char *buf) {
    sprintf(buf, "%02X:%02X:%02X:%02X:%02X:%02X",
            mac[0],mac[1],mac[2],mac[3],mac[4],mac[5]);
}

//beacon parse
void parse_ssid(const uint8_t *payload, char *ssid_out) {
    const uint8_t *ptr_ssid = payload + 36; //first 24 bytes are for header + fixed 12 bytes, 
                                            //means shift pointer for ssid for 36bytes
    while(1) {
        uint8_t tag = ptr_ssid[0];
        uint8_t len = ptr_ssid[1]; 

        if(tag==0) {
            memcpy(ssid_out, &ptr_ssid[2], len);
            ssid_out[len] = 0;
            return;
        }

        ptr_ssid += 2 + len;
    }
}

//security type
security_t parse_security(const uint8_t *payload) {
    const uint8_t *ptr_security = payload + 36;

    while(1) {
        uint8_t tag = ptr_security[0];
        uint8_t len = ptr_security[1];

        if(tag == 48) { //RSN tag (wpa2/wpa3)
            return SEC_WPA2_WPA3;
        }

        if(tag == 221) {
            const uint8_t *v = ptr_security+2;
            if(v[0] == 0x00 && v[1] == 0x50 && v[2] == 0XF2) {
                return SEC_WEP_WPA1;
            }
        }

        ptr_security += 2 + len;

        if(len ==0) break;
        if(ptr_security -(payload + 36) > 400) break;
    }

    return SEC_OPEN;
}

//table operations
int get_network_index(const uint8_t *bssid) {
    for(int i =0; i<MAX_NETWORKS; i++){
        if(networks[i].used && mac_equal(networks[i].bssid, bssid))
            return i;
    }
    return -1;
}

int alloc_network(const uint8_t *bssid, const char *ssid, security_t sec) {
    for(int i=0; i<MAX_NETWORKS; i++) {
        if(!networks[i].used) {
            networks[i].used = true;
            memcpy(networks[i].bssid, bssid, 6);
            strncpy(networks[i].ssid, ssid, 32);
            networks[i].sec = sec;
            return i;
        }
    }
    return -1;
}

//sniffer handler
static void wifi_sniffer_packet_handler(void *buf, wifi_promiscuous_pkt_type_t type) {
    //fuction variables
    const wifi_promiscuous_pkt_t *pkt = (wifi_promiscuous_pkt_t *)buf;
    const uint8_t * payload = pkt->payload;

    //total packet
    for(int i=0; i<MAX_NETWORKS; i++) {
        if(networks[i].used) networks[i].total++;
    }

    //management frames
    if(type == WIFI_PKT_MGMT) {
        uint8_t subtype = (payload[0] >> 4) & 0x0F; //is it a mgmt frame

        const uint8_t *bssid = payload + 16;

        int id = get_network_index(bssid);
        
        //0x08 beacon, 0x0c deauth (802.11 mgmt frame subtypes)
        if(subtype == 0x08) { // 1000 -> beacon
            char ssid[33] = {0};
            parse_ssid(payload, ssid);

            security_t sec = parse_security(payload);

            if(id < 0) id = alloc_network(bssid, ssid, sec);

            if(id >= 0) {
                networks[id].beacon++;
            }
        } else if(subtype == 0X0C) { //1100 -> deauth
            if(id >= 0) {
                networks[id].deauth++;
            }
        }
    }
}

//wifi sniffer init
static void wifi_sniffer_init(void) {
    //NVS (NECESSARY FOR WIFI DRIVER)
    esp_err_t ret = nvs_flash_init();
    if(ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ESP_ERROR_CHECK(nvs_flash_init());
    }

    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    //normally for STA/AP modes netif is created, not for promiscuous mode
    wifi_promiscuous_filter_t filt = {
        .filter_mask = WIFI_PROMIS_FILTER_MASK_MGMT | WIFI_PROMIS_FILTER_MASK_DATA
    };
    ESP_ERROR_CHECK(esp_wifi_set_promiscuous_filter(&filt));

    //callback connect, open promiscouous mode
    ESP_ERROR_CHECK(esp_wifi_set_promiscuous_rx_cb(&wifi_sniffer_packet_handler));
    ESP_ERROR_CHECK(esp_wifi_set_promiscuous(true));

    ESP_LOGI(TAG, "Wifi sniffer started (promiscuous mode)");
}

//main loop
void app_main(void) {

    wifi_sniffer_init();

    while(1){
        vTaskDelay(pdMS_TO_TICKS(WINDOW_MS));

        printf("\n----5 seconds overview-----");

        for(int i=0; i<MAX_NETWORKS; i++) {
            if(!networks[i].used) continue;

            char macbuf[32];
            mac_to_str(networks[i].bssid, macbuf);

            const char *secstr = 
                (networks[i].sec == SEC_OPEN) ? "OPEN" :
                (networks[i].sec == SEC_WEP_WPA1) ? "WEP/WPA1" :
                (networks[i].sec == SEC_WPA2_WPA3) ? "WPA2/WPA3" :
                "Bilinmiyor";

            printf("SSID=%s BSSID=%s SEC=%s TOTAL=%lu Beacon=%lu Deauth=%lu\n",
                networks[i].ssid,
                macbuf,
                secstr,
                networks[i].total,
                networks[i].beacon,
                networks[i].deauth
            );
        }

        printf("-------------------------\n");
    }
}

    
