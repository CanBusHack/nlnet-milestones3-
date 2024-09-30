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
static const ble_uuid128_t isotp_pairs_chr = BLE_UUID128_INIT(0x39, 0x9d, 0x8e, 0x6a, 0x12, 0x6e, 0x8b, 0xb1, 0x57, 0x4d, 0xd7, 0xdf, 0x1d, 0x80, 0x31, 0x7e);
static const ble_uuid128_t isotp_msg_chr = BLE_UUID128_INIT(0x1e, 0x9a, 0x7a, 0x3f, 0x3f, 0x9e, 0x6f, 0x87, 0x3a, 0x42, 0x2b, 0xb9, 0xe1, 0xd4, 0x13, 0x28);

static const uint8_t pairs[] = { 0x20, 0x00, 0x07, 0xE8, 0x00, 0xCC, 0x20, 0x00, 0x07, 0xE0, 0x00, 0xCC };
static const uint8_t message[] = { 0x00, 0x00, 0x07, 0xE8, 0x49, 0x02, 0x01, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48, 0x49, 0x4A, 0x4B, 0x4C, 0x4D, 0x4E, 0x4F, 0x50, 0x51 };
static const twai_message_t flow_control = { .identifier = 0x7E0, .data_length_code = 8, .data = { 0x30, 0x00, 0x00, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC } };
static const twai_message_t expected[] = {
    { .identifier = 0x7E8, .data_length_code = 8, .data = { 0x10, 0x14, 0x49, 0x02, 0x01, 0x41, 0x42, 0x43 } },
    { .identifier = 0x7E8, .data_length_code = 8, .data = { 0x21, 0x44, 0x45, 0x46, 0x47, 0x48, 0x49, 0x4A } },
    { .identifier = 0x7E8, .data_length_code = 8, .data = { 0x22, 0x4B, 0x4C, 0x4D, 0x4E, 0x4F, 0x50, 0x51 } },
};
static twai_message_t actual[3] = { 0 };
static jmp_buf out;

static int write2_cb(uint16_t conn_handle, const struct ble_gatt_error* error, struct ble_gatt_attr* attr, void* arg) {
    static int count = 0;
    TEST_ASSERT_EQUAL(0, count);
    count++;
    TEST_ASSERT_EQUAL_HEX(0, error->status);
    longjmp(out, 1);
    return 0;
}

static int chr2_cb(uint16_t conn_handle, const struct ble_gatt_error* error, const struct ble_gatt_chr* chr, void* arg) {
    static int count = 0;
    if (count && error->status == 0xe) {
        return 0;
    }
    TEST_ASSERT_EQUAL(0, count);
    count++;
    TEST_ASSERT_EQUAL_HEX(0, error->status);
    TEST_ASSERT(chr);
    TEST_ASSERT_EQUAL_HEX(0, ble_gattc_write_flat(conn_handle, chr->val_handle, &message, sizeof(message), write2_cb, NULL));
    return 0;
}

uint16_t start_handle = 0;
uint16_t end_handle = 0;

static int write_cb(uint16_t conn_handle, const struct ble_gatt_error* error, struct ble_gatt_attr* attr, void* arg) {
    static int count = 0;
    TEST_ASSERT_EQUAL(0, count);
    count++;
    TEST_ASSERT_EQUAL_HEX(0, error->status);
    TEST_ASSERT_EQUAL_HEX(0, ble_gattc_disc_chrs_by_uuid(conn_handle, start_handle, end_handle, &isotp_msg_chr.u, chr2_cb, NULL));
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
    TEST_ASSERT_EQUAL_HEX(0, ble_gattc_write_flat(conn_handle, chr->val_handle, &pairs, sizeof(pairs), write_cb, NULL));
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
    start_handle = service->start_handle;
    end_handle = service->end_handle;
    TEST_ASSERT_EQUAL_HEX(0, ble_gattc_disc_chrs_by_uuid(conn_handle, service->start_handle, service->end_handle, &isotp_pairs_chr.u, chr_cb, NULL));
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

TEST_CASE("CAN ISO-TP endpoint - write multi", "[ble][can][isotp]") {
    TEST_IGNORE_MESSAGE("multi-frame writes are not yet implemented");
    memset(actual, 0, sizeof(actual));

    ble_hs_cfg.reset_cb = reset_cb;
    ble_hs_cfg.sync_cb = sync_cb;
    ble_hs_cfg.store_status_cb = ble_store_util_status_rr;

    void ble_store_config_init(void);
    ble_store_config_init();

    if (!setjmp(out)) {
        nimble_port_run();
    }
    TEST_ASSERT_EQUAL(ESP_OK, twai_receive(actual, pdMS_TO_TICKS(30000)));
    TEST_ASSERT_EQUAL(ESP_OK, twai_transmit(&flow_control, pdMS_TO_TICKS(30000)));
    TEST_ASSERT_EQUAL(ESP_OK, twai_receive(actual + 1, pdMS_TO_TICKS(30000)));
    TEST_ASSERT_EQUAL(ESP_OK, twai_receive(actual + 2, pdMS_TO_TICKS(30000)));
    TEST_ASSERT_EQUAL_MEMORY(&expected, &actual, sizeof(expected));
}
