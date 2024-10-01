#include <sdkconfig.h>
#ifdef CONFIG_OMNITRIX_ENABLE_J2534

#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

#include <omnitrix/ble.h>
#include <omnitrix/uuid.gen.h>

#include "j2534.pb-c.h"

enum {
    J1850VPW = 1,
    J1850PWM = 2,
    ISO9141 = 3,
    ISO14230 = 4,
    CAN = 5,
    ISO15765 = 6,
    SCI_A_ENGINE = 7,
    SCI_A_TRANS = 8,
    SCI_B_ENGINE = 9,
    SCI_B_TRANS = 10,
};

enum {
    STATUS_NOERROR = 0,
    ERR_NOT_SUPPORTED = 1,
    ERR_INVALID_CHANNEL_ID = 2,
    ERR_INVALID_PROTOCOL_ID = 3,
    ERR_NULL_PARAMETER = 4,
    ERR_INVALID_IOCTL_VALUE = 5,
    ERR_INVALID_FLAGS = 6,
    ERR_FAILED = 7,
    ERR_DEVICE_NOT_CONNECTED = 8,
    ERR_TIMEOUT = 9,
    ERR_INVALID_MSG = 10,
    ERR_INVALID_TIME_INTERVAL = 11,
    ERR_EXCEEDED_LIMIT = 12,
    ERR_INVALID_MESSAGE_ID = 13,
    ERR_DEVICE_IN_USE = 14,
    ERR_INVALID_IOCTL_ID = 15,
    ERR_BUFFER_EMPTY = 16,
    ERR_BUFFER_FULL = 17,
    ERR_BUFFER_OVERFLOW = 18,
    ERR_PIN_INVALID = 19,
    ERR_CHANNEL_IN_USE = 20,
    ERR_MSG_PROTOCOL_ID = 21,
    ERR_INVALID_FILTER_ID = 22,
    ERR_NO_FLOW_CONTROL = 23,
    ERR_NOT_UNIQUE = 24,
    ERR_INVALID_BAUDRATE = 25,
    ERR_INVALID_DEVICE_ID = 26,
};

static bool channels[2] = { 0 };

struct mem {
    void* buf;
    uint16_t len;
};

static struct mem pack(struct Response* res) {
    assert(res);
    size_t sz = response__get_packed_size(res);
    struct mem result = { 0 };
    if (sz <= UINT16_MAX) {
        result.buf = malloc(sz);
        assert(result.buf);
        result.len = sz;
        response__pack(res, result.buf);
    }
    return result;
}

static struct mem process(uint8_t* inbuf, size_t insz) {
    assert(inbuf);
    struct Request* req = request__unpack(NULL, insz, inbuf);
    struct mem result = { 0 };
    if (req) {
        struct Response* res = malloc(sizeof(struct Response));
        assert(res);
        response__init(res);
        res->id = req->id;

        switch (req->call) {
        case CALL__Connect:
            assert(req->connect);
            switch (req->connect->protocol) {
            case CAN:
                if (!channels[0]) {
                    res->connect = malloc(sizeof(struct ConnectResult));
                    assert(res->connect);
                    connect_result__init(res->connect);
                    res->connect->channel = 0;
                    result = pack(res);
                    free(res->connect);
                    channels[0] = true;
                } else {
                    res->code = ERR_CHANNEL_IN_USE;
                    result = pack(res);
                }
                break;
            case ISO15765:
                if (!channels[1]) {
                    res->connect = malloc(sizeof(struct ConnectResult));
                    assert(res->connect);
                    connect_result__init(res->connect);
                    res->connect->channel = 1;
                    result = pack(res);
                    free(res->connect);
                    channels[1] = true;
                } else {
                    res->code = ERR_CHANNEL_IN_USE;
                    result = pack(res);
                }
                break;
            default:
                if (req->connect->protocol < 11) {
                    res->code = ERR_NOT_SUPPORTED;
                } else {
                    res->code = ERR_INVALID_PROTOCOL_ID;
                }
                result = pack(res);
                break;
            }
            break;
        case CALL__GetError:
            res->result_case = RESPONSE__RESULT_ERROR;
            res->error = malloc(sizeof(struct GetErrorResult));
            assert(res->error);
            get_error_result__init(res->error);
            res->error->error = "No error!";
            result = pack(res);
            free(res->error);
            break;
        default:
            break;
        }

        free(res);
    }
    request__free_unpacked(req, NULL);
    return result;
}

#ifdef CONFIG_OMNITRIX_ENABLE_BLE
#include <host/ble_hs_mbuf.h>
#include <os/os_mbuf.h>
static const ble_uuid128_t gatt_svr_svc_uuid = CONFIG_OMNITRIX_J2534_SERVICE_UUID_INIT;
static const ble_uuid128_t gatt_svr_chr_uuid = CONFIG_OMNITRIX_J2534_CHARACTERISTIC_UUID_INIT;
static uint16_t gatt_svr_chr_val_handle;

static int gatt_svr_chr_access_cb(uint16_t conn_handle, uint16_t attr_handle, struct ble_gatt_access_ctxt* ctxt, void* arg) {
    struct Request* req;
    static uint8_t buf[1024];
    static uint16_t size;
    if (ctxt->op == BLE_GATT_ACCESS_OP_WRITE_CHR) {
        size_t insz = OS_MBUF_PKTLEN(ctxt->om);
        if (insz) {
            uint8_t* inbuf = malloc(insz);
            assert(inbuf);
            uint16_t unused;
            assert(ble_hs_mbuf_to_flat(ctxt->om, inbuf, insz, &unused) == 0);
            struct mem outmem = process(inbuf, insz);
            free(inbuf);
            if (outmem.buf) {
                struct os_mbuf* om = ble_hs_mbuf_from_flat(outmem.buf, outmem.len);
                assert(om);
                free(outmem.buf);
                ble_gatts_notify_custom(conn_handle, attr_handle, om);
            }
        }
    }
    return 0;
}

const struct ble_gatt_svc_def omni_j2534_gatt_svr_svcs[] = {
    {
        .type = BLE_GATT_SVC_TYPE_PRIMARY,
        .uuid = &gatt_svr_svc_uuid.u,
        .characteristics = (struct ble_gatt_chr_def[]) {
            {
                .uuid = &gatt_svr_chr_uuid.u,
                .access_cb = gatt_svr_chr_access_cb,
                .flags = BLE_GATT_CHR_F_WRITE | BLE_GATT_CHR_F_NOTIFY,
                .val_handle = &gatt_svr_chr_val_handle,
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
#endif

#endif
