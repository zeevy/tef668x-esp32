/* Implementation of the MAC read. */
#include "device_id.h"

#include <esp_mac.h>
#include <string.h>

bool deviceMacRead(uint8_t mac[6]) {
  memset(mac, 0, 6);
  return esp_read_mac(mac, ESP_MAC_WIFI_STA) == ESP_OK;
}
