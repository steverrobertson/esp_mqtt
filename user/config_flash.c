#include "lwip/ip.h"
#include "config_flash.h"

/*     From the document 99A-SDK-Espressif IOT Flash RW Operation_v0.2      *
 * -------------------------------------------------------------------------*
 * Flash is erased sector by sector, which means it has to erase 4Kbytes one
 * time at least. When you want to change some data in flash, you have to
 * erase the whole sector, and then write it back with the new data.
 *--------------------------------------------------------------------------*/
void ICACHE_FLASH_ATTR config_load_default(sysconfig_p config) {
    uint8_t mac[6];

    os_memset(config, 0, sizeof(sysconfig_t));
    os_printf("Loading default configuration\r\n");
    config->magic_number = MAGIC_NUMBER;
    config->length = sizeof(sysconfig_t);
    os_sprintf(config->ssid, "%s", WIFI_SSID);
    os_sprintf(config->password, "%s", WIFI_PASSWORD);
    config->auto_connect = 0;
    os_sprintf(config->ap_ssid, "%s", WIFI_AP_SSID);
    os_sprintf(config->ap_password, "%s", WIFI_AP_PASSWORD);
    config->ap_open = 1;
    config->ap_on = 1;

    config->locked = 0;
    config->lock_password[0] = '\0';

    IP4_ADDR(&config->network_addr, 192, 168, 4, 1);
    config->dns_addr.addr = 0;	// use DHCP
    config->my_addr.addr = 0;	// use DHCP   
    config->my_netmask.addr = 0;	// use DHCP   
    config->my_gw.addr = 0;	// use DHCP

    config->system_output = SYSTEM_OUTPUT_INFO;
    config->bit_rate = 115200;  

    config->mdns_mode = 0;	// no mDNS

    config->clock_speed = 80;
    config->config_port = CONSOLE_SERVER_PORT;
    config->config_access = LOCAL_ACCESS | REMOTE_ACCESS;

    config->mqtt_broker_port = MQTT_PORT;
    config->max_subscriptions = 30;
    config->max_retained_messages = 30;
    config->max_clients = 0;
    config->auto_retained = 0;
    os_sprintf(config->mqtt_broker_user, "%s", "none");
    config->mqtt_broker_password[0] = 0;
    config->mqtt_broker_access = LOCAL_ACCESS | REMOTE_ACCESS;

#ifdef MQTT_CLIENT
    os_sprintf(config->mqtt_host, "%s", "none");
    config->mqtt_port = 1883;
    config->mqtt_ssl = false;
    os_sprintf(config->mqtt_user, "%s", "none");
    config->mqtt_password[0] = 0;
    wifi_get_macaddr(0, mac);
    os_sprintf(config->mqtt_id, "%s_%02x%02x%02x", MQTT_ID, mac[3], mac[4], mac[5]);
#endif
#ifdef NTP
    os_sprintf(config->ntp_server, "%s", "1.pool.ntp.org");
    config->ntp_interval = 300000000;
    config->ntp_timezone = 0;
#endif
#ifdef GPIO
#ifdef GPIO_PWM
    config->pwm_period = 5000;
#endif
#endif
}

int ICACHE_FLASH_ATTR config_load(sysconfig_p config) {
    if (config == NULL)
	return -1;
    uint16_t base_address = FLASH_BLOCK_NO;

    spi_flash_read(base_address * SPI_FLASH_SEC_SIZE, &config->magic_number, 4);

    if ((config->magic_number != MAGIC_NUMBER)) {
	os_printf("\r\nNo config found, saving default in flash\r\n");
	config_load_default(config);
	config_save(config);
	return -1;
    }

    os_printf("\r\nConfig found and loaded\r\n");
    spi_flash_read(base_address * SPI_FLASH_SEC_SIZE, (uint32 *) config, sizeof(sysconfig_t));
    if (config->length != sizeof(sysconfig_t)) {
	os_printf("Length Mismatch, probably old version of config, loading defaults\r\n");
	config_load_default(config);
	config_save(config);
	return -1;
    }
    return 0;
}

void ICACHE_FLASH_ATTR config_save(sysconfig_p config) {
    uint16_t base_address = FLASH_BLOCK_NO;
    os_printf("Saving configuration\r\n");
    spi_flash_erase_sector(base_address);
    spi_flash_write(base_address * SPI_FLASH_SEC_SIZE, (uint32 *) config, sizeof(sysconfig_t));
}

void ICACHE_FLASH_ATTR blob_save(uint8_t blob_no, uint32_t * data, uint16_t len) {
    uint16_t base_address = FLASH_BLOCK_NO + 1 + blob_no;
    spi_flash_erase_sector(base_address);
    spi_flash_write(base_address * SPI_FLASH_SEC_SIZE, data, len);
}

void ICACHE_FLASH_ATTR blob_load(uint8_t blob_no, uint32_t * data, uint16_t len) {
    uint16_t base_address = FLASH_BLOCK_NO + 1 + blob_no;
    spi_flash_read(base_address * SPI_FLASH_SEC_SIZE, data, len);
}

void ICACHE_FLASH_ATTR blob_zero(uint8_t blob_no, uint16_t len) {
    int i;
    uint8_t z[len];
    os_memset(z, 0, len);
    uint16_t base_address = FLASH_BLOCK_NO + 1 + blob_no;
    spi_flash_erase_sector(base_address);
    spi_flash_write(base_address * SPI_FLASH_SEC_SIZE, (uint32_t *) z, len);
}
