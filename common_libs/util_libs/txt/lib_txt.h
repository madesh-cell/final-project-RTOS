/**
 * @file    lib_txt.h
 * @author  Madesh
 * @brief   Public interface for the LIB_TXT bounded text buffer module.
 * @version 0.1
 * @date    2026-10-01
 *
 * @note
 * - Replaces snprintf, because MISRA C:2012 Rule 21.6 forbids stdio in production code.
 * - Overflow is latched in the buffer; check LIB_TXT_HasOverflowed() before using the text.
 *
 * @revision_history
 * - 0.1 2026-10-01 Madesh Initial creation.
 *
 * @copyright Copyright (c) 2026 CIOET
 */

#ifndef LIB_TXT_H
#define LIB_TXT_H

/* ── Includes ────────────────────────────────────────────────────────────── */
#include <stdint.h>
#include <stdbool.h>

/* ── Public Types ────────────────────────────────────────────────────────── */
typedef struct
{
    uint8_t *storage;
    uint16_t capacity;
    uint16_t length;
    bool     has_overflowed;
} LIB_TXT_Buffer_t;

/* ── Public Function Prototypes ──────────────────────────────────────────── */

/**
 * @name    LIB_TXT_Init
 * @brief   Attach a buffer to caller-owned storage and clear it.
 * @param   buffer   Buffer descriptor to initialise. Must not be NULL.
 * @param   storage  Caller-owned byte array. Must not be NULL.
 * @param   capacity Size of storage in bytes. Must be at least 2.
 * @return  true on success, false if an argument is invalid.
 */
bool LIB_TXT_Init(LIB_TXT_Buffer_t *buffer, uint8_t *storage, uint16_t capacity);

/**
 * @name    LIB_TXT_AppendString
 * @brief   Append a zero-terminated string.
 * @param   buffer Initialised buffer.
 * @param   text   String to append. Must not be NULL.
 * @return  true if everything fitted, false if the text was truncated.
 */
bool LIB_TXT_AppendString(LIB_TXT_Buffer_t *buffer, const char *text);

/**
 * @name    LIB_TXT_AppendInt32
 * @brief   Append a signed integer in decimal.
 * @param   buffer Initialised buffer.
 * @param   value  Value to append.
 * @return  true if everything fitted, false if the text was truncated.
 */
bool LIB_TXT_AppendInt32(LIB_TXT_Buffer_t *buffer, int32_t value);

/**
 * @name    LIB_TXT_AppendTenths
 * @brief   Append a value given in tenths as a decimal with one digit, e.g. 402 -> "40.2".
 * @param   buffer Initialised buffer.
 * @param   tenths Value in tenths of a unit.
 * @return  true if everything fitted, false if the text was truncated.
 */
bool LIB_TXT_AppendTenths(LIB_TXT_Buffer_t *buffer, int32_t tenths);

/**
 * @name    LIB_TXT_HasOverflowed
 * @brief   Check whether any append since LIB_TXT_Init was truncated.
 * @param   buffer Initialised buffer.
 * @return  true if text was lost, false otherwise.
 */
bool LIB_TXT_HasOverflowed(const LIB_TXT_Buffer_t *buffer);

/**
 * @name    LIB_TXT_GetLength
 * @brief   Read the number of bytes currently held (excluding the terminator).
 * @param   buffer Initialised buffer.
 * @return  Length in bytes.
 */
uint16_t LIB_TXT_GetLength(const LIB_TXT_Buffer_t *buffer);

#endif /* LIB_TXT_H */
