#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include <omnitrix/libcan.h>

#include "isotp.h"

#define CAN_DEBUG 1
#define BLE_DEBUG 1

struct isotp_addr_pairs isotp_addr_pairs[ISOTP_MAX_PAIRS] = { 0 };

static struct {
    struct {
        uint8_t buf[256];
        int offset;
        int size;
        uint8_t ctr;
    } pairs[ISOTP_MAX_PAIRS];
    uint8_t bs;
    uint8_t stmin;
#ifdef CAN_DEBUG
    bool can_debug;
#endif
#ifdef BLE_DEBUG
    bool ble_debug;
#endif
} isotp_addr_pairs_extra = { 0 };

#ifdef CAN_DEBUG

#define debug_frame_log(write_frame, msg)                     \
    if (isotp_addr_pairs_extra.can_debug) {                   \
        debug_frame_log_n(write_frame, msg, sizeof(msg) - 1); \
    }

static void debug_frame_log_n(isotp_write_frame* write_frame, const char* msg, size_t size) {
    assert(write_frame);
    assert(msg);
    assert(size);

    uint8_t buf[8] = { 2 };

    for (; size > 7; size -= 7, msg += 7) {
        memcpy(buf + 1, msg, 7);
        write_frame(0x9FFFFFFE, 8, buf);
    }
    if (size) {
        memcpy(buf + 1, msg, size);
        write_frame(0x9FFFFFFE, size + 1, buf);
    }
}

static void debug_frame(struct isotp_event* evt, isotp_write_frame* write_frame) {
    assert(evt);
    assert(write_frame);
    assert(evt->type == EVENT_INCOMING_CAN);

    if (evt->can.dlc > 0) {
        switch (evt->can.data[0]) {
        case 0:
            isotp_addr_pairs_extra.can_debug = false;
            write_frame(0x9FFFFFFE, 1, evt->can.data);
            return;
        case 1:
            isotp_addr_pairs_extra.can_debug = true;
            write_frame(0x9FFFFFFE, 1, evt->can.data);
            return;
        default:
            break;
        }
    }
    uint8_t data = 0xFF;
    write_frame(0x9FFFFFFE, 1, &data);
}

#else

#define debug_frame_log(write_frame, msg)

#endif

#ifdef BLE_DEBUG

#define debug_msg_log(read_message_cb, msg)                     \
    if (isotp_addr_pairs_extra.ble_debug) {                     \
        debug_msg_log_n(read_message_cb, msg, sizeof(msg) - 1); \
    }

static void debug_msg_log_n(isotp_read_message_cb* read_message_cb, const char* msg, size_t size) {
    assert(read_message_cb);
    assert(msg);
    assert(size);

    uint8_t buf[256] = { 0xFF, 0xFF, 0xFF, 0xFE, 0x02 };

    for (; size > 251; size -= 251, msg += 251) {
        memcpy(buf + 5, msg, 251);
        read_message_cb(buf, 256, 0);
    }
    if (size) {
        memcpy(buf + 5, msg, size);
        read_message_cb(buf, size + 5, 0);
    }
}

static void debug_msg(struct isotp_event* evt, isotp_read_message_cb* read_message_cb) {
    assert(evt);
    assert(read_message_cb);
    assert(evt->type == EVENT_WRITE_MSG);

    if (evt->msg.size > 3) {
        evt->msg.data[3] -= 1;
        if (evt->msg.size > 4) {
            switch (evt->msg.data[4]) {
            case 0:
                isotp_addr_pairs_extra.ble_debug = false;
                read_message_cb(evt->msg.data, 5, 0);
                return;
            case 1:
                isotp_addr_pairs_extra.ble_debug = true;
                read_message_cb(evt->msg.data, 5, 0);
                return;
            default:
                break;
            }
            evt->msg.data[4] = 0xFF;
            read_message_cb(evt->msg.data, 5, 0);
        }
    }
}

#else

#define debug_msg_log(read_message_cb, msg)

#endif

static void handle_write_msg(struct isotp_event* evt, int index, isotp_write_frame* write_frame) {
    assert(evt);
    assert(index >= 0 && index < ISOTP_MAX_PAIRS);

    int pad_sz = 0, msg_start = 4, sf_max = 11;
    if (isotp_addr_pairs[index].txid & 0x40000000) {
        pad_sz++;
        msg_start++;
        sf_max++;
    }
    if (evt->msg.size <= sf_max) {
        // single frame
        uint8_t dlc = evt->msg.size - 3;
        if (isotp_addr_pairs[index].txid & 0x20000000) {
            dlc = 8;
            for (int i = evt->msg.size; i < sf_max; i++) {
                evt->msg.data[i] = isotp_addr_pairs[index].txpad;
            }
        }
        evt->msg.data[3] = isotp_addr_pairs[index].txext;
        evt->msg.data[3 + pad_sz] = evt->msg.size - msg_start;
        write_frame(isotp_addr_pairs[index].txid & 0x9FFFFFFF, dlc, evt->msg.data + 3);
    }
    // TODO: the rest of the owl
}

static void send_flow_control(int index, isotp_write_frame* write_frame) {
    int start = 1;
    uint8_t dlc = 3;
    uint8_t buf[9];
    buf[0] = isotp_addr_pairs[index].txext;
    buf[1] = 0x30;
    buf[2] = isotp_addr_pairs_extra.bs;
    buf[3] = isotp_addr_pairs_extra.stmin;
    buf[4] = isotp_addr_pairs[index].txpad;
    buf[5] = isotp_addr_pairs[index].txpad;
    buf[6] = isotp_addr_pairs[index].txpad;
    buf[7] = isotp_addr_pairs[index].txpad;
    buf[8] = isotp_addr_pairs[index].txpad;
    if (isotp_addr_pairs[index].txid & 0x40000000) {
        start = 0;
        dlc = 4;
    }
    if (isotp_addr_pairs[index].txid & 0x20000000) {
        dlc = 8;
    }
    write_frame(isotp_addr_pairs[index].txid & 0x9FFFFFFF, dlc, buf + start);
}

static void handle_read_can(struct isotp_event* evt, int index, isotp_write_frame* write_frame, isotp_read_message_cb* read_message_cb) {
    assert(evt);
    assert(index >= 0 && index < ISOTP_MAX_PAIRS);
    assert(read_message_cb);

    size_t pci_byte = (isotp_addr_pairs[index].rxid & 0x40000000) ? 1 : 0;
    switch (evt->can.data[pci_byte] >> 4) {
    case 0:
        if (evt->can.data[pci_byte] <= (7 - pci_byte)) {
            uint8_t buf[11];
            buf[0] = evt->can.id >> 24;
            buf[1] = evt->can.id >> 16;
            buf[2] = evt->can.id >> 8;
            buf[3] = evt->can.id;
            buf[4] = evt->can.data[0];
            memcpy(buf + 4 + pci_byte, evt->can.data + pci_byte + 1, 7 - pci_byte);
            read_message_cb(buf, evt->can.data[pci_byte] + pci_byte + 4, isotp_addr_pairs[index].channel);
        }
        break;
    case 1:
        if (evt->can.data[pci_byte] == 0x10 && evt->can.data[pci_byte + 1] <= (252 - pci_byte)) {
            isotp_addr_pairs_extra.pairs[index].offset = 10;
            isotp_addr_pairs_extra.pairs[index].size = evt->can.data[pci_byte + 1] + pci_byte + 4;
            isotp_addr_pairs_extra.pairs[index].ctr = 1;
            assert(isotp_addr_pairs_extra.pairs[index].size <= 256);
            isotp_addr_pairs_extra.pairs[index].buf[0] = evt->can.id >> 24;
            isotp_addr_pairs_extra.pairs[index].buf[1] = evt->can.id >> 16;
            isotp_addr_pairs_extra.pairs[index].buf[2] = evt->can.id >> 8;
            isotp_addr_pairs_extra.pairs[index].buf[3] = evt->can.id;
            isotp_addr_pairs_extra.pairs[index].buf[4] = evt->can.data[0];
            memcpy(isotp_addr_pairs_extra.pairs[index].buf + 4 + pci_byte, evt->can.data + pci_byte + 2, 6 - pci_byte);
            send_flow_control(index, write_frame);
        }
        break;
    case 2: {
        int offset = isotp_addr_pairs_extra.pairs[index].offset;
        int size = isotp_addr_pairs_extra.pairs[index].size;
        int max_sz = 7 - pci_byte;
        int rem = size - offset;
        if (rem > 0 && (evt->can.data[pci_byte] & 0xF) == isotp_addr_pairs_extra.pairs[index].ctr) {
            isotp_addr_pairs_extra.pairs[index].ctr = (isotp_addr_pairs_extra.pairs[index].ctr + 1) & 0xF;
            memcpy(isotp_addr_pairs_extra.pairs[index].buf + offset, evt->can.data + pci_byte + 1, (max_sz < size) ? max_sz : size);
            isotp_addr_pairs_extra.pairs[index].offset += (max_sz < size) ? max_sz : size;
            rem -= max_sz;
            if (rem <= 0) {
                read_message_cb(isotp_addr_pairs_extra.pairs[index].buf, size, isotp_addr_pairs[index].channel);
            }
        }
        break;
    }
    case 3:
        break;
    default:
        break;
    }
}

void isotp_event_loop(isotp_event_cb* get_next_event, isotp_unmatched_frame* unmatched_frame, isotp_write_frame* write_frame, isotp_read_message_cb* read_message_cb) {
    assert(get_next_event);
    assert(unmatched_frame);
    assert(write_frame);
    assert(read_message_cb);

    struct isotp_event evt;

    memset(&isotp_addr_pairs, 0, sizeof(isotp_addr_pairs));

    for (;;) {
        get_next_event(&evt);
        switch (evt.type) {
        case EVENT_RECONFIGURE_PAIRS: {
            for (int i = 0; i < ISOTP_MAX_PAIRS; i++) {
                memset(isotp_addr_pairs + i, 0, sizeof(isotp_addr_pairs[0]));
            }
            omni_libcan_clear_filter();
            assert(evt.pairs.size % 12 == 0);
            assert(evt.pairs.size / 12 <= ISOTP_MAX_PAIRS);
            for (int i = 0, j = 0; i + 11 < evt.pairs.size && j < ISOTP_MAX_PAIRS; i += 12, j++) {
                isotp_addr_pairs[j].active = true;
                isotp_addr_pairs[j].txid = (evt.pairs.data[i] << 24) | (evt.pairs.data[i + 1] << 16) | (evt.pairs.data[i + 2] << 8) | evt.pairs.data[i + 3];
                isotp_addr_pairs[j].txext = evt.pairs.data[i + 4];
                isotp_addr_pairs[j].txpad = evt.pairs.data[i + 5];
                isotp_addr_pairs[j].rxid = (evt.pairs.data[i + 6] << 24) | (evt.pairs.data[i + 7] << 16) | (evt.pairs.data[i + 8] << 8) | evt.pairs.data[i + 9];
                isotp_addr_pairs[j].rxext = evt.pairs.data[i + 10];
                isotp_addr_pairs[j].rxpad = evt.pairs.data[i + 11];
                omni_libcan_add_filter(isotp_addr_pairs[j].rxid & 0x1FFFFFFF, (isotp_addr_pairs[j].rxid & 0x80000000) != 0);
            }
            break;
        }
        case EVENT_RECONFIGURE_BS_STMIN: {
            isotp_addr_pairs_extra.bs = evt.bs_stmin.data[0];
            isotp_addr_pairs_extra.stmin = evt.bs_stmin.data[1];
            break;
        }
        case EVENT_WRITE_MSG: {
            debug_frame_log(write_frame, "Writing message...");
            assert(evt.msg.size > 4);
            uint32_t id = (evt.msg.data[0] << 24) | (evt.msg.data[1] << 16) | (evt.msg.data[2] << 8) | evt.msg.data[3];
            bool matched = false;
            for (int i = 0; i < ISOTP_MAX_PAIRS; i++) {
                bool is_active = isotp_addr_pairs[i].active;
                bool id_match = is_active && (id & 0x9FFFFFFF) == (isotp_addr_pairs[i].txid & 0x9FFFFFFF);
                if (id_match && isotp_addr_pairs[i].txid & 0x40000000) {
                    assert(evt.msg.size > 5);
                }
                bool id_ext_match = id_match && (!(isotp_addr_pairs[i].txid & 0x40000000) || (isotp_addr_pairs[i].txext == evt.msg.data[4]));
                if (id_ext_match) {
                    matched = true;
                    handle_write_msg(&evt, i, write_frame);
                    break;
                }
            }
#ifdef BLE_DEBUG
            if (!matched && id == 0xFFFFFFFF) {
                debug_msg(&evt, read_message_cb);
                break;
            }
#endif
            if (!matched && evt.msg.size <= 11) {
                // allow anyways when it fits in a single frame
                for (int i = evt.msg.size; i < 11; i++) {
                    evt.msg.data[i] = 0;
                }
                evt.msg.data[3] = evt.msg.size - 4;
                write_frame(id, 8, evt.msg.data + 3);
            }
            break;
        }
        case EVENT_INCOMING_CAN: {
            debug_msg_log(read_message_cb, "Incoming frame...");
            bool matched = false;
            for (int i = 0; i < ISOTP_MAX_PAIRS; i++) {
                bool is_active = isotp_addr_pairs[i].active;
                bool id_match = is_active && (evt.can.id & 0x9FFFFFFF) == (isotp_addr_pairs[i].rxid & 0x9FFFFFFF);
                bool id_ext_match = id_match && (!(isotp_addr_pairs[i].rxid & 0x40000000) || (isotp_addr_pairs[i].rxext == evt.can.data[0]));
                if (id_ext_match) {
                    matched = true;
                    handle_read_can(&evt, i, write_frame, read_message_cb);
                    break;
                }
            }
            if (!matched) {
#ifdef CAN_DEBUG
                if (evt.can.id == 0x9FFFFFFF) {
                    debug_frame(&evt, write_frame);
                    break;
                }
#endif
                unmatched_frame(evt.can.frame);
            }
            break;
        }
        case EVENT_SHUTDOWN:
            return;
        }
    }
}
