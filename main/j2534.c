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

enum {
    CH_CAN_1 = 0x314e4143,
    CH_ISO15765_1 = 0x304f5349,
};

static bool channels[2] = { 0 };

struct mem {
    void* buf;
    uint16_t len;
};

#define PACK_AND_RETURN(name)                              \
    do {                                                   \
        size_t sz = name##_response__get_packed_size(res); \
        struct mem result = { 0 };                         \
        if (sz <= UINT16_MAX) {                            \
            result.buf = malloc(sz);                       \
            assert(result.buf);                            \
            result.len = sz;                               \
            name##_response__pack(res, result.buf);        \
        }                                                  \
        free(res);                                         \
        return result;                                     \
    } while (0)

static struct mem process_connect(uint8_t* inbuf, size_t insz) {
    assert(inbuf);
    struct ConnectRequest* req = connect_request__unpack(NULL, insz, inbuf);
    assert(req);
    assert(req->call == CALL__Connect);

    struct ConnectResponse* res = malloc(sizeof(struct ConnectResponse));
    connect_response__init(res);
    res->id = req->id;
    res->call = CALL__Connect;
    switch (req->protocol) {
    case CAN:
        if (!channels[0]) {
            res->code = STATUS_NOERROR;
            res->channel = CH_CAN_1;
            channels[0] = true;
        } else {
            res->code = ERR_CHANNEL_IN_USE;
        }
        break;
    case ISO15765:
        if (!channels[1]) {
            res->code = STATUS_NOERROR;
            res->channel = CH_ISO15765_1;
            channels[1] = true;
        } else {
            res->code = ERR_CHANNEL_IN_USE;
        }
        break;
    default:
        if (req->protocol && req->protocol < 11) {
            res->code = ERR_NOT_SUPPORTED;
        } else {
            res->code = ERR_INVALID_PROTOCOL_ID;
        }
        break;
    }
    connect_request__free_unpacked(req, NULL);

    PACK_AND_RETURN(connect);
}

static struct mem process_disconnect(uint8_t* inbuf, size_t insz) {
    assert(inbuf);
    struct DisconnectRequest* req = disconnect_request__unpack(NULL, insz, inbuf);
    assert(req);
    assert(req->call == CALL__Disconnect);

    struct BaseResponse* res = malloc(sizeof(struct BaseResponse));
    base_response__init(res);
    res->id = req->id;
    res->call = CALL__Disconnect;
    switch (req->channel) {
    case CH_CAN_1:
        if (channels[0]) {
            res->code = STATUS_NOERROR;
            channels[0] = false;
        } else {
            res->code = ERR_INVALID_CHANNEL_ID;
        }
        break;
    case CH_ISO15765_1:
        if (channels[1]) {
            res->code = STATUS_NOERROR;
            channels[1] = false;
        } else {
            res->code = ERR_INVALID_CHANNEL_ID;
        }
        break;
    default:
        res->code = ERR_INVALID_CHANNEL_ID;
        break;
    }
    disconnect_request__free_unpacked(req, NULL);

    PACK_AND_RETURN(base);
}

static struct mem process_read(uint8_t* inbuf, size_t insz) {
    assert(inbuf);
    struct BaseRequest* req = base_request__unpack(NULL, insz, inbuf);
    assert(req);
    assert(req->call == CALL__Read);

    struct BaseResponse* res = malloc(sizeof(struct BaseResponse));
    base_response__init(res);
    res->id = req->id;
    res->call = CALL__Read;
    res->code = ERR_NOT_SUPPORTED;
    base_request__free_unpacked(req, NULL);

    PACK_AND_RETURN(base);
}

static struct mem process_write(uint8_t* inbuf, size_t insz) {
    assert(inbuf);
    struct BaseRequest* req = base_request__unpack(NULL, insz, inbuf);
    assert(req);
    assert(req->call == CALL__Write);

    struct BaseResponse* res = malloc(sizeof(struct BaseResponse));
    base_response__init(res);
    res->id = req->id;
    res->call = CALL__Write;
    res->code = ERR_NOT_SUPPORTED;
    base_request__free_unpacked(req, NULL);

    PACK_AND_RETURN(base);
}

static struct mem process_start_periodic(uint8_t* inbuf, size_t insz) {
    assert(inbuf);
    struct BaseRequest* req = base_request__unpack(NULL, insz, inbuf);
    assert(req);
    assert(req->call == CALL__StartPeriodic);

    struct BaseResponse* res = malloc(sizeof(struct BaseResponse));
    base_response__init(res);
    res->id = req->id;
    res->call = CALL__StartPeriodic;
    res->code = ERR_NOT_SUPPORTED;
    base_request__free_unpacked(req, NULL);

    PACK_AND_RETURN(base);
}

static struct mem process_stop_periodic(uint8_t* inbuf, size_t insz) {
    assert(inbuf);
    struct BaseRequest* req = base_request__unpack(NULL, insz, inbuf);
    assert(req);
    assert(req->call == CALL__StopPeriodic);

    struct BaseResponse* res = malloc(sizeof(struct BaseResponse));
    base_response__init(res);
    res->id = req->id;
    res->call = CALL__StopPeriodic;
    res->code = ERR_NOT_SUPPORTED;
    base_request__free_unpacked(req, NULL);

    PACK_AND_RETURN(base);
}

static struct mem process_start_filter(uint8_t* inbuf, size_t insz) {
    assert(inbuf);
    struct BaseRequest* req = base_request__unpack(NULL, insz, inbuf);
    assert(req);
    assert(req->call == CALL__StartFilter);

    struct BaseResponse* res = malloc(sizeof(struct BaseResponse));
    base_response__init(res);
    res->id = req->id;
    res->call = CALL__StartFilter;
    res->code = ERR_NOT_SUPPORTED;
    base_request__free_unpacked(req, NULL);

    PACK_AND_RETURN(base);
}

static struct mem process_stop_filter(uint8_t* inbuf, size_t insz) {
    assert(inbuf);
    struct BaseRequest* req = base_request__unpack(NULL, insz, inbuf);
    assert(req);
    assert(req->call == CALL__StopFilter);

    struct BaseResponse* res = malloc(sizeof(struct BaseResponse));
    base_response__init(res);
    res->id = req->id;
    res->call = CALL__StopFilter;
    res->code = ERR_NOT_SUPPORTED;
    base_request__free_unpacked(req, NULL);

    PACK_AND_RETURN(base);
}

static struct mem process_set_voltage(uint8_t* inbuf, size_t insz) {
    assert(inbuf);
    struct SetVoltageRequest* req = set_voltage_request__unpack(NULL, insz, inbuf);
    assert(req);
    assert(req->call == CALL__SetVoltage);
    base_request__free_unpacked(req, NULL);

    struct BaseResponse* res = malloc(sizeof(struct BaseResponse));
    base_response__init(res);
    res->id = req->id;
    res->call = CALL__SetVoltage;
    res->code = ERR_NOT_SUPPORTED;

    PACK_AND_RETURN(base);
}

static struct mem process_read_version(uint8_t* inbuf, size_t insz) {
    assert(inbuf);
    struct BaseRequest* req = base_request__unpack(NULL, insz, inbuf);
    assert(req);
    assert(req->call == CALL__ReadVersion);
    base_request__free_unpacked(req, NULL);

    struct ReadVersionResponse* res = malloc(sizeof(struct ReadVersionResponse));
    read_version_response__init(res);
    res->id = req->id;
    res->call = CALL__Connect;
    res->code = STATUS_NOERROR;
    res->version = "00.01";

    PACK_AND_RETURN(read_version);
}

static struct mem process_get_error(uint8_t* inbuf, size_t insz) {
    assert(inbuf);
    struct BaseRequest* req = base_request__unpack(NULL, insz, inbuf);
    assert(req);
    assert(req->call == CALL__GetError);
    base_request__free_unpacked(req, NULL);

    struct GetErrorResponse* res = malloc(sizeof(struct GetErrorResponse));
    get_error_response__init(res);
    res->id = req->id;
    res->call = CALL__GetError;
    res->code = STATUS_NOERROR;
    res->error = "PassThruGetLastError is not set supported!";

    PACK_AND_RETURN(get_error);
}

static struct mem process_ioctl_read_vbatt(uint8_t* inbuf, size_t insz) {
    assert(inbuf);
    struct IoctlRequest* req = ioctl_request__unpack(NULL, insz, inbuf);
    assert(req);
    assert(req->call == CALL__Ioctl);
    assert(req->ioctl == IOCTL_ID__ReadVbatt);
    ioctl_request__free_unpacked(req, NULL);

    struct IoctlReadVbattResponse* res = malloc(sizeof(struct IoctlReadVbattResponse));
    ioctl_read_vbatt_response__init(res);
    res->id = req->id;
    res->call = CALL__Ioctl;
    res->code = STATUS_NOERROR;
    res->ioctl = IOCTL_ID__ReadVbatt;
    res->voltage = 13000;

    PACK_AND_RETURN(ioctl_read_vbatt);
}

static struct mem process_ioctl(uint8_t* inbuf, size_t insz) {
    assert(inbuf);
    struct IoctlRequest* req = ioctl_request__unpack(NULL, insz, inbuf);
    assert(req);
    assert(req->call == CALL__Ioctl);
    IoctlId ioctl = req->ioctl;
    ioctl_request__free_unpacked(req, NULL);

    switch (ioctl) {
    case IOCTL_ID__ReadVbatt:
        return process_ioctl_read_vbatt(inbuf, insz);
    default:
        break;
    }

    struct IoctlResponse* res = malloc(sizeof(struct IoctlResponse));
    ioctl_response__init(res);
    res->id = req->id;
    res->call = CALL__Ioctl;
    res->code = ERR_INVALID_IOCTL_ID;
    res->ioctl = req->ioctl;

    PACK_AND_RETURN(ioctl);
}

static struct mem process(uint8_t* inbuf, size_t insz) {
    assert(inbuf);
    struct BaseRequest* req = base_request__unpack(NULL, insz, inbuf);
    if (req) {
        Call call = req->call;
        base_request__free_unpacked(req, NULL);
        switch (call) {
        case CALL__Connect:
            return process_connect(inbuf, insz);
        case CALL__Disconnect:
            return process_disconnect(inbuf, insz);
        case CALL__Read:
            return process_read(inbuf, insz);
        case CALL__Write:
            return process_write(inbuf, insz);
        case CALL__StartPeriodic:
            return process_start_periodic(inbuf, insz);
        case CALL__StopPeriodic:
            return process_stop_periodic(inbuf, insz);
        case CALL__StartFilter:
            return process_start_filter(inbuf, insz);
        case CALL__StopFilter:
            return process_stop_filter(inbuf, insz);
        case CALL__SetVoltage:
            return process_set_voltage(inbuf, insz);
        case CALL__ReadVersion:
            return process_read_version(inbuf, insz);
        case CALL__GetError:
            return process_get_error(inbuf, insz);
        case CALL__Ioctl:
            return process_ioctl(inbuf, insz);
        default:
            break;
        }
    }
    return (struct mem) { 0 };
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
