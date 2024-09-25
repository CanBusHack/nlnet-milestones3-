#include <stdint.h>
#include <string.h>

#include <esp_log.h>
#include <host/ble_gap.h>
#include <host/ble_hs.h>
#include <host/ble_hs_adv.h>
#include <host/ble_hs_id.h>
#include <host/ble_store.h>
#include <host/util/util.h>
#include <nimble/nimble_port.h>
#include <services/gap/ble_svc_gap.h>
#include <unity.h>

TEST_CASE("BLE hello endpoint", "[ble][hello]") {
    int connect_cb(struct ble_gap_event* event, void* arg) {
        TEST_FAIL_MESSAGE("unexpected connect CB");
        return 0;
    }

    int scan_cb(struct ble_gap_event* event, void* arg) {
        switch (event->type) {
        case BLE_GAP_EVENT_DISC:
            struct ble_hs_adv_fields fields;
            TEST_ASSERT_EQUAL(ble_hs_adv_parse_fields(&fields, event->disc.data, event->disc.length_data), 0);
            if (fields.name_len > 0 && !strncmp((const char*)fields.name, "BlinkCar v1.0", fields.name_len)) {
                ESP_LOGE("hi", "found blinkcar");
                TEST_ASSERT_EQUAL(ble_gap_disc_cancel(), 0);
                uint8_t own_addr_type;
                TEST_ASSERT_EQUAL(ble_hs_id_infer_auto(0, &own_addr_type), 0);
                TEST_ASSERT_EQUAL(ble_gap_connect(own_addr_type, &event->disc.addr, 30000, NULL, connect_cb, NULL), 0);
            }
            break;
        case BLE_GAP_EVENT_DISC_COMPLETE:
            TEST_FAIL_MESSAGE("scan timeout");
            break;
        default:
            TEST_FAIL_MESSAGE("unexpected scan CB");
            break;
        }

        return 0;
    }

    void scan(void) {
        uint8_t own_addr_type;
        TEST_ASSERT_EQUAL(ble_hs_id_infer_auto(0, &own_addr_type), 0);

        struct ble_gap_disc_params params = {
            .filter_duplicates = 1,
            .passive = 1,
        };
        TEST_ASSERT_EQUAL(ble_gap_disc(own_addr_type, 30000, &params, scan_cb, NULL), 0);
    }

    void reset_cb(int reason) {
        TEST_FAIL_MESSAGE("unexpected BLE reset");
    }

    void sync_cb(void) {
        TEST_ASSERT_EQUAL(ble_hs_util_ensure_addr(0), 0);
        scan();
    }

    ble_hs_cfg.reset_cb = reset_cb;
    ble_hs_cfg.sync_cb = sync_cb;
    ble_hs_cfg.store_status_cb = ble_store_util_status_rr;

    void ble_store_config_init(void);
    ble_store_config_init();

    nimble_port_run();
}
