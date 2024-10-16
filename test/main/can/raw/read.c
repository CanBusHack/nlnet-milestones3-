#include <setjmp.h>
#include <stdint.h>
#include <string.h>

#include <driver/twai.h>
#include <host/ble_gap.h>
#include <host/ble_gatt.h>
#include <host/ble_hs.h>
#include <host/ble_hs_adv.h>
#include <host/ble_hs_id.h>
#include <host/ble_hs_mbuf.h>
#include <host/ble_store.h>
#include <host/util/util.h>
#include <nimble/nimble_port.h>
#include <services/gap/ble_svc_gap.h>
#include <unity.h>

static const ble_uuid128_t hello_svc = BLE_UUID128_INIT(0x46, 0x9a, 0x1b, 0xa2, 0xe8, 0xb6, 0xf6, 0x93, 0x33, 0x43, 0x3d, 0x4e, 0xa8, 0x1b, 0x94, 0x49);
static const ble_uuid128_t can_chr = BLE_UUID128_INIT(0x43, 0x17, 0x96, 0x20, 0x7c, 0xf2, 0x2c, 0x90, 0xce, 0x4b, 0xb0, 0x8a, 0x25, 0x9b, 0x59, 0x0b);

static const twai_message_t expected = { .identifier = 0x42B, .data_length_code = 4, .data = { 0xCA, 0xFE, 0xBA, 0xBE } };
static twai_message_t actual = { 0 };
static jmp_buf out;

static int read_cb(uint16_t conn_handle, const struct ble_gatt_error* error, struct ble_gatt_attr* attr, void* arg) {
    static int count = 0;
    TEST_ASSERT_EQUAL(0, count);
    count++;
    TEST_ASSERT_EQUAL_HEX(0, error->status);
    uint16_t len;
    TEST_ASSERT_EQUAL_HEX(0, ble_hs_mbuf_to_flat(attr->om, &actual, sizeof(actual), &len));
    TEST_ASSERT_EQUAL(sizeof(actual), len);
    longjmp(out, 1);
    return 0;
}

static int chr_cb(uint16_t conn_handle, const struct ble_gatt_error* error, const struct ble_gatt_chr* chr, void* arg) {
    static int count = 0;
    if (count && error->status == 0xe) {
        return 0;
    }
    TEST_ASSERT_EQUAL(0, count);
    count++;
    TEST_ASSERT_EQUAL_HEX(0, error->status);
    TEST_ASSERT(chr);
    TEST_ASSERT_EQUAL_HEX(0, ble_gattc_read(conn_handle, chr->val_handle, read_cb, NULL));
    return 0;
}

static int svc_cb(uint16_t conn_handle, const struct ble_gatt_error* error, const struct ble_gatt_svc* service, void* arg) {
    static int count = 0;
    if (count && error->status == 0xe) {
        return 0;
    }
    TEST_ASSERT_EQUAL(0, count);
    count++;
    TEST_ASSERT_EQUAL_HEX(0, error->status);
    TEST_ASSERT(service);
    TEST_ASSERT_EQUAL_HEX(0, ble_gattc_disc_chrs_by_uuid(conn_handle, service->start_handle, service->end_handle, &can_chr.u, chr_cb, NULL));
    return 0;
}

static int mtu_cb(uint16_t conn_handle, const struct ble_gatt_error* error, uint16_t mtu, void* arg) {
    static int count = 0;
    TEST_ASSERT_EQUAL(0, count);
    count++;
    TEST_ASSERT_EQUAL_HEX(0, error->status);
    TEST_ASSERT_EQUAL_HEX(0, ble_gattc_disc_svc_by_uuid(conn_handle, &hello_svc.u, svc_cb, NULL));
    return 0;
}

static int connect_cb(struct ble_gap_event* event, void* arg) {
    static int count = 0;
    switch (event->type) {
    case BLE_GAP_EVENT_CONNECT:
        TEST_ASSERT_EQUAL(0, count);
        count++;
        TEST_ASSERT_EQUAL_HEX(0, event->connect.status);
        struct ble_gap_conn_desc desc;
        TEST_ASSERT_EQUAL_HEX(0, ble_gap_conn_find(event->connect.conn_handle, &desc));
        int rc = ble_gattc_exchange_mtu(event->connect.conn_handle, mtu_cb, NULL);
        if (rc != 0xe) {
            TEST_ASSERT_EQUAL_HEX(0, rc);
        }
        break;
    case BLE_GAP_EVENT_MTU:
        break;
    default:
        TEST_FAIL_MESSAGE("unexpected connect CB");
        break;
    }

    return 0;
}

static int scan_cb(struct ble_gap_event* event, void* arg) {
    static int count = 0;
    struct ble_hs_adv_fields fields;
    switch (event->type) {
    case BLE_GAP_EVENT_DISC:
        TEST_ASSERT_EQUAL_HEX(0, ble_hs_adv_parse_fields(&fields, event->disc.data, event->disc.length_data));
        if (fields.name_len > 0 && !strncmp((const char*)fields.name, "BlinkCar v1.0", fields.name_len)) {
            TEST_ASSERT_EQUAL(0, count);
            count++;
            TEST_ASSERT_EQUAL_HEX(0, ble_gap_disc_cancel());
            uint8_t own_addr_type;
            TEST_ASSERT_EQUAL_HEX(0, ble_hs_id_infer_auto(0, &own_addr_type));
            TEST_ASSERT_EQUAL_HEX(0, ble_gap_connect(own_addr_type, &event->disc.addr, 30000, NULL, connect_cb, NULL));
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

static void scan(void) {
    static int count = 0;
    TEST_ASSERT_EQUAL(0, count);
    count++;
    uint8_t own_addr_type;
    TEST_ASSERT_EQUAL_HEX(0, ble_hs_id_infer_auto(0, &own_addr_type));

    struct ble_gap_disc_params params = {
        .filter_duplicates = 1,
        .passive = 1,
    };
    TEST_ASSERT_EQUAL_HEX(0, ble_gap_disc(own_addr_type, 30000, &params, scan_cb, NULL));
}

static void reset_cb(int reason) {
    TEST_FAIL_MESSAGE("unexpected BLE reset");
}

static void sync_cb(void) {
    static int count = 0;
    TEST_ASSERT_EQUAL(0, count);
    count++;
    TEST_ASSERT_EQUAL_HEX(0, ble_hs_util_ensure_addr(0));
    scan();
}

TEST_CASE("CAN raw endpoint - read", "[ble][can]") {
    memset(&actual, 0, sizeof(actual));

    ble_hs_cfg.reset_cb = reset_cb;
    ble_hs_cfg.sync_cb = sync_cb;
    ble_hs_cfg.store_status_cb = ble_store_util_status_rr;

    void ble_store_config_init(void);
    ble_store_config_init();

    TEST_ASSERT_EQUAL(ESP_OK, twai_transmit(&expected, pdMS_TO_TICKS(30000)));
    if (!setjmp(out)) {
        nimble_port_run();
    }
    TEST_ASSERT_EQUAL_MEMORY(&expected, &actual, sizeof(expected));
}
