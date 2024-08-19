#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "isotp.h"

static struct {
    size_t count;
    struct {
        uint32_t txid;
        uint32_t rxid;
        uint8_t txext;
        uint8_t txpad;
        uint8_t rxext;
        uint8_t rxpad;
        uint8_t buf[256];
        int offset;
        int size;
        uint8_t ctr;
    } pairs[ISOTP_MAX_PAIRS];
    uint8_t bs;
    uint8_t stmin;
} isotp_addr_pairs = { 0 };

static void handle_write_msg(struct isotp_event* evt, int index, isotp_write_frame* write_frame) {
    assert(evt);
    assert(isotp_addr_pairs.count <= ISOTP_MAX_PAIRS);
    assert(index >= 0 && index < isotp_addr_pairs.count);

    int pad_sz = 0, msg_start = 4, sf_max = 11;
    if (isotp_addr_pairs.pairs[index].txid & 0x40000000) {
        pad_sz++;
        msg_start++;
        sf_max++;
    }
    if (evt->msg.size <= sf_max) {
        // single frame
        uint8_t dlc = evt->msg.size - 3;
        if (isotp_addr_pairs.pairs[index].txid & 0x20000000) {
            dlc = 8;
            for (int i = evt->msg.size; i < sf_max; i++) {
                evt->msg.data[i] = isotp_addr_pairs.pairs[index].txpad;
            }
        }
        evt->msg.data[3] = isotp_addr_pairs.pairs[index].txext;
        evt->msg.data[3 + pad_sz] = evt->msg.size - msg_start;
        write_frame(isotp_addr_pairs.pairs[index].txid & 0x9FFFFFFF, dlc, evt->msg.data + 3);
    }
    // TODO: the rest of the owl
}

static void send_flow_control(int index, isotp_write_frame* write_frame) {
    int start = 1;
    uint8_t dlc = 3;
    uint8_t buf[9];
    buf[0] = isotp_addr_pairs.pairs[index].txext;
    buf[1] = 0x30;
    buf[2] = isotp_addr_pairs.bs;
    buf[3] = isotp_addr_pairs.stmin;
    buf[4] = isotp_addr_pairs.pairs[index].txpad;
    buf[5] = isotp_addr_pairs.pairs[index].txpad;
    buf[6] = isotp_addr_pairs.pairs[index].txpad;
    buf[7] = isotp_addr_pairs.pairs[index].txpad;
    buf[8] = isotp_addr_pairs.pairs[index].txpad;
    if (isotp_addr_pairs.pairs[index].txid & 0x40000000) {
        start = 0;
        dlc = 4;
    }
    if (isotp_addr_pairs.pairs[index].txid & 0x20000000) {
        dlc = 8;
    }
    write_frame(isotp_addr_pairs.pairs[index].txid & 0x9FFFFFFF, dlc, buf + start);
}

static void handle_read_can(struct isotp_event* evt, int index, isotp_write_frame* write_frame, isotp_read_message_cb* read_message_cb) {
    assert(evt);
    assert(isotp_addr_pairs.count <= ISOTP_MAX_PAIRS);
    assert(index >= 0 && index < isotp_addr_pairs.count);
    assert(read_message_cb);

    size_t pci_byte = (isotp_addr_pairs.pairs[index].rxid & 0x40000000) ? 1 : 0;
    switch (evt->can.data[pci_byte] >> 4) {
    case 0:
        if (evt->can.data[pci_byte] <= (7 - pci_byte)) {
            read_message_cb(evt->can.data + pci_byte + 1, evt->can.data[pci_byte]);
        }
        break;
    case 1:
        if (evt->can.data[pci_byte] == 0x10 && evt->can.data[pci_byte + 1] <= (252 - pci_byte)) {
            isotp_addr_pairs.pairs[index].offset = 10;
            isotp_addr_pairs.pairs[index].size = evt->can.data[pci_byte + 1] + pci_byte + 4;
            isotp_addr_pairs.pairs[index].ctr = 1;
            assert(isotp_addr_pairs.pairs[index].size <= 256);
            isotp_addr_pairs.pairs[index].buf[0] = evt->can.id >> 24;
            isotp_addr_pairs.pairs[index].buf[1] = evt->can.id >> 16;
            isotp_addr_pairs.pairs[index].buf[2] = evt->can.id >> 8;
            isotp_addr_pairs.pairs[index].buf[3] = evt->can.id;
            isotp_addr_pairs.pairs[index].buf[4] = evt->can.data[0];
            memcpy(isotp_addr_pairs.pairs[index].buf + 4 + pci_byte, evt->can.data + pci_byte + 2, 6 - pci_byte);
            send_flow_control(index, write_frame);
        }
        break;
    case 2: {
        int offset = isotp_addr_pairs.pairs[index].offset;
        int size = isotp_addr_pairs.pairs[index].size;
        int max_sz = 7 - pci_byte;
        int rem = size - offset;
        if (rem > 0 && (evt->can.data[pci_byte] & 0xF) == isotp_addr_pairs.pairs[index].ctr) {
            isotp_addr_pairs.pairs[index].ctr = (isotp_addr_pairs.pairs[index].ctr + 1) & 0xF;
            memcpy(isotp_addr_pairs.pairs[index].buf + offset, evt->can.data + pci_byte + 1, (max_sz < size) ? max_sz : size);
            isotp_addr_pairs.pairs[index].offset += (max_sz < size) ? max_sz : size;
            rem -= max_sz;
            if (rem <= 0) {
                read_message_cb(isotp_addr_pairs.pairs[index].buf, size);
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
            assert(isotp_addr_pairs.count <= ISOTP_MAX_PAIRS);
            for (int i = 0; i < isotp_addr_pairs.count; i++) {
                memset(isotp_addr_pairs.pairs + i, 0, sizeof(isotp_addr_pairs.pairs[0]));
            }
            assert(evt.pairs.size % 12 == 0);
            isotp_addr_pairs.count = evt.pairs.size / 12;
            assert(isotp_addr_pairs.count <= ISOTP_MAX_PAIRS);
            for (int i = 0, j = 0; i + 11 < evt.pairs.size && j < ISOTP_MAX_PAIRS; i += 12, j++) {
                isotp_addr_pairs.pairs[j].txid = (evt.pairs.data[i] << 24) | (evt.pairs.data[i + 1] << 16) | (evt.pairs.data[i + 2] << 8) | evt.pairs.data[i + 3];
                isotp_addr_pairs.pairs[j].txext = evt.pairs.data[i + 4];
                isotp_addr_pairs.pairs[j].txpad = evt.pairs.data[i + 5];
                isotp_addr_pairs.pairs[j].rxid = (evt.pairs.data[i + 6] << 24) | (evt.pairs.data[i + 7] << 16) | (evt.pairs.data[i + 8] << 8) | evt.pairs.data[i + 9];
                isotp_addr_pairs.pairs[j].rxext = evt.pairs.data[i + 10];
                isotp_addr_pairs.pairs[j].rxpad = evt.pairs.data[i + 11];
            }
            break;
        }
        case EVENT_RECONFIGURE_BS_STMIN: {
            isotp_addr_pairs.bs = evt.bs_stmin.data[0];
            isotp_addr_pairs.stmin = evt.bs_stmin.data[1];
            break;
        }
        case EVENT_WRITE_MSG: {
            assert(evt.msg.size > 4);
            uint32_t id = (evt.msg.data[0] << 24) | (evt.msg.data[1] << 16) | (evt.msg.data[2] << 8) | evt.msg.data[3];
            bool matched = false;
            for (int i = 0; i < isotp_addr_pairs.count; i++) {
                bool id_match = (id & 0x9FFFFFFF) == (isotp_addr_pairs.pairs[i].txid & 0x9FFFFFFF);
                if (id_match && isotp_addr_pairs.pairs[i].txid & 0x40000000) {
                    assert(evt.msg.size > 5);
                }
                bool id_ext_match = id_match && (!(isotp_addr_pairs.pairs[i].txid & 0x40000000) || (isotp_addr_pairs.pairs[i].txext == evt.msg.data[4]));
                if (id_ext_match) {
                    matched = true;
                    handle_write_msg(&evt, i, write_frame);
                    break;
                }
            }
            if (!matched && evt.msg.size <= 11) {
                // allow anyways when it fits in a single frame
                for (int i = evt.msg.size; i < 11; i++) {
                    evt.msg.data[i] = 0xCC;
                }
                evt.msg.data[3] = evt.msg.size - 4;
                write_frame(id, 8, evt.msg.data + 3);
            }
            break;
        }
        case EVENT_INCOMING_CAN: {
            bool matched = false;
            for (int i = 0; i < isotp_addr_pairs.count; i++) {
                bool id_match = (evt.can.id & 0x9FFFFFFF) == (isotp_addr_pairs.pairs[i].rxid & 0x9FFFFFFF);
                bool id_ext_match = id_match && (!(isotp_addr_pairs.pairs[i].rxid & 0x40000000) || (isotp_addr_pairs.pairs[i].rxext == evt.can.data[0]));
                if (id_ext_match) {
                    matched = true;
                    handle_read_can(&evt, i, write_frame, read_message_cb);
                    break;
                }
            }
            if (!matched) {
                unmatched_frame(evt.can.frame);
            }
            break;
        }
        case EVENT_SHUTDOWN:
            return;
        }
    }
}
