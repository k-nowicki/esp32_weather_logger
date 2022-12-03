/* Common functions to establish Wi-Fi or Ethernet connection.

   This code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
 */

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "esp_err.h"
#include "esp_netif.h"

#ifdef CONFIG_KK_CONNECT_ETHERNET
#define KK_INTERFACE get_esp_netif()
#endif

#ifdef CONFIG_KK_CONNECT_WIFI
#define KK_INTERFACE get_esp_netif()
#endif

#if !defined (CONFIG_KK_CONNECT_ETHERNET) && !defined (CONFIG_KK_CONNECT_WIFI)
// This is useful for some tests which do not need a network connection
#define KK_INTERFACE NULL
#endif

/**
 * @brief Configure Wi-Fi or Ethernet, connect, wait for IP
 *
 * @return ESP_OK on successful connection
 */
esp_err_t network_connect(void);

/**
 * Counterpart to network_connect, de-initializes Wi-Fi or Ethernet
 */
esp_err_t network_disconnect(void);

/**
 * @brief Configure stdin and stdout to use blocking I/O
 *
 * This helper function is used in ASIO examples. It wraps installing the
 * UART driver and configuring VFS layer to use UART driver for console I/O.
 */
esp_err_t net_configure_stdin_stdout(void);

/**
 * @brief Returns esp-netif pointer created by network_connect()
 *
 * @note If multiple interfaces active at once, this API return NULL
 * In that case the get_esp_netif_from_desc() should be used
 * to get esp-netif pointer based on interface description
 */
esp_netif_t *get_esp_netif(void);

/**
 * @brief Returns esp-netif pointer created by network_connect() described by
 * the supplied desc field
 *
 * @param desc Textual interface of created network interface, for example "sta"
 * indicate default WiFi station, "eth" default Ethernet interface.
 *
 */
esp_netif_t *get_esp_netif_from_desc(const char *desc);

#ifdef __cplusplus
}
#endif
