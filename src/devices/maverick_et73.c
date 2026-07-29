/** @file
    Maverick ET-73.

    Copyright (C) 2018 Benjamin Larsson

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.
*/
/**
Maverick ET-73.

Based on TP12 code

    [00] {48} 68 00 01 0b 90 fc : 01101000 00000000 00000001 00001011 10010000 11111100
    [01] {48} 68 00 01 0b 90 fc : 01101000 00000000 00000001 00001011 10010000 11111100
    [02] {48} 68 00 01 0b 90 fc : 01101000 00000000 00000001 00001011 10010000 11111100
    [03] {48} 68 00 01 0b 90 fc : 01101000 00000000 00000001 00001011 10010000 11111100
    [04] {48} 68 00 01 0b 90 fc : 01101000 00000000 00000001 00001011 10010000 11111100
    [05] {48} 68 00 01 0b 90 fc : 01101000 00000000 00000001 00001011 10010000 11111100
    [06] {48} 68 00 01 0b 90 fc : 01101000 00000000 00000001 00001011 10010000 11111100
    [07] {48} 68 00 01 0b 90 fc : 01101000 00000000 00000001 00001011 10010000 11111100
    [08] {48} 68 00 01 0b 90 fc : 01101000 00000000 00000001 00001011 10010000 11111100
    [09] {48} 68 00 01 0b 90 fc : 01101000 00000000 00000001 00001011 10010000 11111100
    [10] {48} 68 00 01 0b 90 fc : 01101000 00000000 00000001 00001011 10010000 11111100
    [11] {48} 68 00 01 0b 90 fc : 01101000 00000000 00000001 00001011 10010000 11111100
    [12] {48} 68 00 01 0b 90 fc : 01101000 00000000 00000001 00001011 10010000 11111100
    [13] {48} 68 00 01 0b 90 fc : 01101000 00000000 00000001 00001011 10010000 11111100


Layout appears to be:

              II 11 12 22 XX XX
    [01] {48} 68 00 01 0b 90 fc : 01101000 00000000 00000001 00001011 10010000 11111100

- I = random id
- 1 = temperature sensor 1 12 bits
- 2 = temperature sensor 2 12 bits
- X = unknown, checksum maybe ?

*/

#include "decoder.h"

/**
 * Validate the Maverick ET-73 "unknown, checksum maybe?" bytes.
 * This is reverse-engineered from live logging (temperature sweeps from
 * -3.3C to +210.9C), not from any spec -- treat as a strong heuristic,
 * not a guaranteed-correct algorithm.
 *
 * Call this AFTER temp1_raw has been computed (using whatever sign
 * handling your decoder now uses past the 204.8C rollover point).
 *
 * Observed relationship:
 *   bytes[4] is constant (0x48 on the unit tested -- may be
 *            session/device-specific; verify before relying on it
 *            across different transmitter IDs)
 *   bytes[5] = (BASE - temp1_raw - crossings) mod 256
 *       BASE = 92, crossings = temp1_raw / 256   for temp1_raw >= 0
 *       BASE = 77, crossings = 0                  for temp1_raw <  0
 *       (negative-branch wraparound has not been tested below about -18C)
 */
static int maverick_et73_checksum_valid(uint8_t const *bytes, int temp1_raw)
{
    int id = bytes[0];
    int base;
    int crossings;

    if (temp1_raw >= 0) {
        base      = 439 - id;   // was: 92, now id-adjusted (verified against id=92 and id=56)
        crossings = temp1_raw / 256;
        fprintf(stderr, "if (base:%d,crossings:%d)\n", base, crossings);        
    } else {
        base      = 439 - id - 315; // untested for this id -- see note below
        crossings = 0;
        fprintf(stderr, "else (base:%d,crossings:%d)\n", base, crossings);        
    }

    int expected = base - temp1_raw - crossings;
    fprintf(stderr, "expected: %d\n", expected);
    expected = ((expected % 256) + 256) % 256;
    fprintf(stderr, "expected: %d\n", expected);
    fprintf(stderr, "bytes[4]: %02x, bytes[5]: %02x\n", bytes[4], bytes[5]);
    fprintf(stderr, "checksum: %s\n", (bytes[4] == 0x48) && (bytes[5] == (uint8_t)expected) ? "PASS" : "FAIL");

    return (bytes[4] == 0x48) && (bytes[5] == (uint8_t)expected);
}
static int maverick_et73_decode(r_device *decoder, bitbuffer_t *bitbuffer)
{
  int temp1_raw, temp2_raw, row;
    float temp1_c, temp2_c, temp1_f;
    uint8_t *bytes;
    unsigned int device;
    data_t *data;
    char raw_str[16];
    int checksum_ok;
    char *checksum_string= "OK";

    // The device transmits many rows, let's check for 3 matching.
    row = bitbuffer_find_repeated_row(bitbuffer, 3, 48);
    if (row < 0) {
        return DECODE_ABORT_EARLY;
    }

    bytes = bitbuffer->bb[row];
    if ((!bytes[0] && !bytes[1] && !bytes[2] && !bytes[3]) || (bytes[0] == 0xFF && bytes[1] == 0xFF && bytes[2] == 0xFF && bytes[3] == 0xFF))  {
        fprintf(stderr, "%02x ", bytes[0]);
        fprintf(stderr, "%03x ", (bytes[1] << 8 | (bytes[2] & 0xf0)));
        fprintf(stderr, "%03x ", (((bytes[2] & 0x0f) << 12) | bytes[3] << 4));
        fprintf(stderr, "%02x ", bytes[4]);
        fprintf(stderr, "%02x ", bytes[5]);
        fprintf(stderr, "ABORT: all zeros or all ones");
        fprintf(stderr, "\n");
        fflush(stderr);
        return DECODE_ABORT_EARLY; // reduce false positives
    }

    if (bitbuffer->bits_per_row[row] != 48)
    {
        fprintf(stderr, "%02x ", bytes[0]);
        fprintf(stderr, "%03x ", (bytes[1] << 8 | (bytes[2] & 0xf0)));
        fprintf(stderr, "%03x ", (((bytes[2] & 0x0f) << 12) | bytes[3] << 4));
        fprintf(stderr, "%02x ", bytes[4]);
        fprintf(stderr, "%02x ", bytes[5]);
        fprintf(stderr, "ABORT: bits != 48 %d", bitbuffer->bits_per_row[row]);
        fprintf(stderr, "\n");
        fflush(stderr);
        return DECODE_ABORT_LENGTH;
    }
    device = bytes[0];

    decoder_log_bitrow(decoder, 1, __func__, bytes, 48, "");

    // Repack the nibbles to form a 12-bit field representing the 2's-complement temperatures,
    //   then right shift by 4 to sign-extend the 12-bit field to a 16-bit integer for float conversion
    temp1_raw = (uint16_t)(bytes[1] << 8 | (bytes[2] & 0xf0)); // uses sign-extend
    checksum_ok = maverick_et73_checksum_valid(bytes, temp1_raw);
    if (checksum_ok == 0) {
        checksum_string = "FAIL";
    }

    temp1_c   = (temp1_raw >> 4) * 0.1f;
    temp1_f   = temp1_c * 9.0f / 5.0f + 32.0f;
    temp2_raw = (uint16_t)(((bytes[2] & 0x0f) << 12) | bytes[3] << 4); // uses sign-extend
    temp2_c   = (temp2_raw >> 4) * 0.1f;

    /* clang-format off */
    data = data_make(
            "model",            "",                        DATA_STRING, "Maverick-ET73",
            "id",               "Random Id",               DATA_INT, device,
            "temperature_1_F",  "Temperature 1 Computed",  DATA_FORMAT, "%.1f F", DATA_DOUBLE, temp1_f,
            "temperature_1_C",  "Temperature 1",           DATA_FORMAT, "%.1f C", DATA_DOUBLE, temp1_c,
            "temperature_2_C",  "Temperature 2",           DATA_FORMAT, "%.1f C", DATA_DOUBLE, temp2_c,
            "checksum",         "Checksum",                DATA_STRING, checksum_string,
            NULL);
    /* clang-format on */

    data = data_hex(data, "raw_msg", "Raw Message", NULL, bytes, 6, raw_str);

    fprintf(stderr, "%02x ", bytes[0]);
    fprintf(stderr, "%03x ", (bytes[1] << 8 | (bytes[2] & 0xf0)));
    fprintf(stderr, "%03x ", (((bytes[2] & 0x0f) << 12) | bytes[3] << 4));
    fprintf(stderr, "%02x ", bytes[4]);
    fprintf(stderr, "%02x ", bytes[5]);
    fprintf(stderr, "%.1fF", temp1_f);
    fprintf(stderr, "\n");
    fflush(stderr);

    decoder_output_data(decoder, data);
    return 1;
}

static char const *const output_fields[] = {
        "model",
        "id",
        "temperature_1_F",
        "temperature_1_C",
        "temperature_2_C",
        "checksum",
        "raw_msg",
        NULL,
};

r_device const maverick_et73 = {
        .name        = "Maverick ET73",
        .modulation  = OOK_PULSE_PPM,
        .short_width = 1050,
        .long_width  = 2050,
        .gap_limit   = 2200,
        .reset_limit = 4400, // 4050 us nominal packet gap
        .decode_fn   = &maverick_et73_decode,
        .fields      = output_fields,
};
