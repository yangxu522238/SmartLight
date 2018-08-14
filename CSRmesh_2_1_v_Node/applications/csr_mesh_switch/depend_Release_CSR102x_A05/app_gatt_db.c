/*
 * THIS FILE IS AUTOGENERATED, DO NOT EDIT!
 *
 * generated by gattdbgen from depend_Release_CSR102x_A05/app_gatt_db.db_
 */

#include "depend_Release_CSR102x_A05/app_gatt_db.h"

/* GATT database */
uint16 gattDatabase[] = {
    /* 0001: Primary Service 00001100-d102-11e1-9b23-00025b00a5a5 */
    0x0010, 0xa5a5, 0x005b, 0x0200, 0x239b, 0xe111, 0x02d1, 0x0011, 0x0000,
    /* 0002: Characteristic Declaration 00001101-d102-11e1-9b23-00025b00a5a5 */
    0x3013, 0x0803, 0x00a5, 0xa500, 0x5b02, 0x0023, 0x9be1, 0x1102, 0xd101, 0x1100, 0x0000,
    /* 0003: . */
    0xcc01, 0x0000,
    /* 0004: Characteristic Declaration 00001102-d102-11e1-9b23-00025b00a5a5 */
    0x3013, 0x1205, 0x00a5, 0xa500, 0x5b02, 0x0023, 0x9be1, 0x1102, 0xd102, 0x1100, 0x0000,
    /* 0005: . */
    0xcc01, 0x0000,
    /* 0006: Client Characteristic Configuration */
    0x6c00,
    /* 0007: Characteristic Declaration 00001103-d102-11e1-9b23-00025b00a5a5 */
    0x3013, 0x0a08, 0x00a5, 0xa500, 0x5b02, 0x0023, 0x9be1, 0x1102, 0xd103, 0x1100, 0x0000,
    /* 0008: . */
    0xcc01, 0x0000,
    /* 0009: Primary Service 1801 */
    0x0002, 0x0118,
    /* 000a: Characteristic Declaration 2a05 */
    0x3005, 0x220b, 0x0005, 0x2a00,
    /* 000b:  */
    0xd400,
    /* 000c: Client Characteristic Configuration */
    0x6402, 0x0000,
    /* 000d: Primary Service 1800 */
    0x0002, 0x0018,
    /* 000e: Characteristic Declaration 2a00 */
    0x3005, 0x0a0f, 0x0000, 0x2a00,
    /* 000f: . */
    0xd501, 0x0000,
    /* 0010: Characteristic Declaration 2a01 */
    0x3005, 0x0211, 0x0001, 0x2a00,
    /* 0011: . */
    0xd401, 0x0000,
    /* 0012: Characteristic Declaration 2a04 */
    0x3005, 0x0213, 0x0004, 0x2a00,
    /* 0013: H`..X. */
    0xd406, 0x4860, 0x0000, 0x5802,
    /* 0014: Primary Service fef1 */
    0x0002, 0xf1fe,
    /* 0015: Characteristic Declaration c4edc000-9daf-11e3-8000-00025b000b00 */
    0x3013, 0x0816, 0x0000, 0x0b00, 0x5b02, 0x0000, 0x80e3, 0x11af, 0x9d00, 0xc0ed, 0xc400,
    /* 0016: . */
    0xc501, 0x0000,
    /* 0017: Characteristic Declaration c4edc000-9daf-11e3-8001-00025b000b00 */
    0x3013, 0x0218, 0x0000, 0x0b00, 0x5b02, 0x0001, 0x80e3, 0x11af, 0x9d00, 0xc0ed, 0xc400,
    /* 0018: ................ */
    0xc010, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
    /* 0019: Characteristic Declaration c4edc000-9daf-11e3-8002-00025b000b00 */
    0x3013, 0x0a1a, 0x0000, 0x0b00, 0x5b02, 0x0002, 0x80e3, 0x11af, 0x9d00, 0xc0ed, 0xc400,
    /* 001a: . */
    0xc401, 0x0000,
    /* 001b: Characteristic Declaration c4edc000-9daf-11e3-8003-00025b000b00 */
    0x3013, 0x1c1c, 0x0000, 0x0b00, 0x5b02, 0x0003, 0x80e3, 0x11af, 0x9d00, 0xc0ed, 0xc400,
    /* 001c: . */
    0xc401, 0x0000,
    /* 001d: Client Characteristic Configuration */
    0x6402, 0x0000,
    /* 001e: Characteristic Declaration c4edc000-9daf-11e3-8004-00025b000b00 */
    0x3013, 0x1c1f, 0x0000, 0x0b00, 0x5b02, 0x0004, 0x80e3, 0x11af, 0x9d00, 0xc0ed, 0xc400,
    /* 001f: . */
    0xc401, 0x0000,
    /* 0020: Client Characteristic Configuration */
    0x6402, 0x0000,
    /* 0021: Characteristic Declaration c4edc000-9daf-11e3-8005-00025b000b00 */
    0x3013, 0x0a22, 0x0000, 0x0b00, 0x5b02, 0x0005, 0x80e3, 0x11af, 0x9d00, 0xc0ed, 0xc400,
    /* 0022: . */
    0xc401, 0x0000,
    /* 0023: Characteristic Declaration c4edc000-9daf-11e3-8006-00025b000b00 */
    0x3013, 0x0a24, 0x0000, 0x0b00, 0x5b02, 0x0006, 0x80e3, 0x11af, 0x9d00, 0xc0ed, 0xc400,
    /* 0024: . */
    0xc401, 0x0000,
};

uint16 *GattGetDatabase(uint16 *len)
{
    *len = sizeof(gattDatabase);
    return gattDatabase;
}

/* End-of-File */
