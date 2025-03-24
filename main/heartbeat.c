#include <esp_log.h>
#include <string.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/timers.h>

#include <host/ble_gap.h>
#include <host/ble_gatt.h>
#include <host/ble_uuid.h>
#include <services/gatt/ble_svc_gatt.h>

#include <omnitrix/heartbeat.h>
#include <omnitrix/debug.h>

static const char tag[] = "omni_heartbeat";

static const ble_uuid128_t gatt_svr_svc_heartbeat_uuid =
    BLE_UUID128_INIT(0x25, 0x59, 0x42, 0xb5, 0xd3, 0xa9, 0xbc, 0xb2, 
                     0x76, 0x45, 0x08, 0x9f, 0x40, 0x3a, 0x2d, 0xa8);

static const ble_uuid128_t gatt_svr_chr_heartbeat_uuid =
    BLE_UUID128_INIT(0x12, 0x4c, 0xe5, 0x60, 0x3b, 0x72, 0x0d, 0x9a,
                     0xd5, 0x44, 0x96, 0x0f, 0x87, 0x0c, 0x76, 0xb0);

static uint16_t gatt_svr_chr_heartbeat_val_handle;
static uint32_t last_heartbeat_time = 0;
static bool connection_alive = false;
static TimerHandle_t heartbeat_timer = NULL;
static uint16_t current_conn_handle = BLE_HS_CONN_HANDLE_NONE;

static void heartbeat_timer_callback(TimerHandle_t xTimer) {
    ESP_LOGW(tag, "Heartbeat timer expired. Connection alive: %d, Last heartbeat: %lu ms ago",
             connection_alive, 
             (xTaskGetTickCount() * portTICK_PERIOD_MS) - last_heartbeat_time);
             
    if (connection_alive) {
        connection_alive = false;
        omni_debug_log("HEARTBEAT", "Connection timeout detected");
        
        if (current_conn_handle != BLE_HS_CONN_HANDLE_NONE) {
            ESP_LOGI(tag, "Initiating disconnection due to timeout");
            ble_gap_terminate(current_conn_handle, BLE_ERR_REM_USER_CONN_TERM);
        }
    }
}

static int gatt_svr_chr_access_heartbeat(uint16_t handle, uint16_t attr_handle,
                                       struct ble_gatt_access_ctxt *ctxt, void *arg) {
    ESP_LOGI(tag, "Heartbeat characteristic accessed, op: %d, handle: %d", 
             ctxt->op, handle);
    
    switch (ctxt->op) {
        case BLE_GATT_ACCESS_OP_WRITE_CHR: {
            uint8_t heartbeat_data[1];
            uint16_t len;
            
            int rc = ble_hs_mbuf_to_flat(ctxt->om, heartbeat_data, sizeof(heartbeat_data), &len);
            if (rc == 0) {
                last_heartbeat_time = xTaskGetTickCount() * portTICK_PERIOD_MS;
                connection_alive = true;
                
                ESP_LOGI(tag, "Heartbeat received, data[0]: %d, len: %d", 
                         heartbeat_data[0], len);
                
                // Reset timeout timer
                if (xTimerReset(heartbeat_timer, 0) != pdPASS) {
                    ESP_LOGE(tag, "Failed to reset heartbeat timer");
                }
                
                // Send response back to smartphone
                struct os_mbuf *om = ble_hs_mbuf_from_flat(heartbeat_data, sizeof(heartbeat_data));
                if (om) {
                    rc = ble_gattc_notify_custom(handle, gatt_svr_chr_heartbeat_val_handle, om);
                    if (rc != 0) {
                        ESP_LOGE(tag, "Failed to send notification: %d", rc);
                    } else {
                        ESP_LOGI(tag, "Notification sent successfully");
                    }
                } else {
                    ESP_LOGE(tag, "Failed to allocate memory for notification");
                }
                
                return 0;
            }
            ESP_LOGE(tag, "Error reading heartbeat data: %d", rc);
            return BLE_ATT_ERR_INVALID_ATTR_VALUE_LEN;
        }
        
        case BLE_GATT_ACCESS_OP_READ_CHR: {
            ESP_LOGI(tag, "Read request received");
            uint8_t heartbeat_status = connection_alive ? 1 : 0;
            int rc = os_mbuf_append(ctxt->om, &heartbeat_status, sizeof(heartbeat_status));
            if (rc == 0) {
                ESP_LOGI(tag, "Read response sent, status: %d", heartbeat_status);
            } else {
                ESP_LOGE(tag, "Failed to send read response: %d", rc);
            }
            return rc == 0 ? 0 : BLE_ATT_ERR_INSUFFICIENT_RES;
        }
    }
    
    return BLE_ATT_ERR_UNLIKELY;
}

const struct ble_gatt_svc_def omni_heartbeat_gatt_svr_svcs[] = {
    {
        .type = BLE_GATT_SVC_TYPE_PRIMARY,
        .uuid = &gatt_svr_svc_heartbeat_uuid.u,
        .characteristics = (struct ble_gatt_chr_def[]) {
            {
                .uuid = &gatt_svr_chr_heartbeat_uuid.u,
                .access_cb = gatt_svr_chr_access_heartbeat,
                .flags = BLE_GATT_CHR_F_WRITE_NO_RSP | 
                        BLE_GATT_CHR_F_READ |
                        BLE_GATT_CHR_F_NOTIFY,
                .val_handle = &gatt_svr_chr_heartbeat_val_handle,
            },
            {
                0,
            },
        },
    },
    {
        0,
    },
};

bool omni_heartbeat_is_alive(void) {
    return connection_alive;
}

uint32_t omni_heartbeat_last_time(void) {
    return last_heartbeat_time;
}

void omni_heartbeat_connection_update(uint16_t handle, bool connected) {
    ESP_LOGI(tag, "Connection update - handle: %d, connected: %d", handle, connected);
    
    current_conn_handle = connected ? handle : BLE_HS_CONN_HANDLE_NONE;
    
    if (!connected) {
        connection_alive = false;
        if (xTimerStop(heartbeat_timer, 0) != pdPASS) {
            ESP_LOGE(tag, "Failed to stop heartbeat timer");
        }
    } else {
        if (xTimerReset(heartbeat_timer, 0) != pdPASS) {
            ESP_LOGE(tag, "Failed to start heartbeat timer");
        }
    }
}

void omni_heartbeat_main(void) {
    ESP_LOGI(tag, "Initializing heartbeat service");
    
    // Create timeout timer
    heartbeat_timer = xTimerCreate(
        "heartbeat_timer",
        pdMS_TO_TICKS(CONFIG_OMNITRIX_HEARTBEAT_TIMEOUT_MS),
        pdFALSE,  // Auto reload = false
        NULL,
        heartbeat_timer_callback
    );
    
    if (heartbeat_timer == NULL) {
        ESP_LOGE(tag, "Failed to create heartbeat timer");
        return;
    }
    
    ESP_LOGI(tag, "Heartbeat service initialized with timeout of %d ms", 
             CONFIG_OMNITRIX_HEARTBEAT_TIMEOUT_MS);
    omni_debug_log("HEARTBEAT", "Service initialized");
}