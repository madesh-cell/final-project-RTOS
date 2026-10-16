/**
 * @file    lib_txt.c
 * @author  Madesh
 * @brief   Bounded text buffer (LIB_TXT) - integer and string formatting without stdio.
 * @version 0.1
 * @date    2026-10-01
 *
 * @note
 * - Uses <string.h> only for NULL, as permitted for the LIB_ layer.
 * - No MISRA C deviations in this file.
 *
 * @revision_history
 * - 0.1 2026-10-01 Madesh Initial creation.
 *
 * @copyright Copyright (c) 2026 CIOET
 */

/* ── Includes ────────────────────────────────────────────────────────────── */
#include "lib_txt.h"
#include <stdint.h>
#include <stdbool.h>
#include <string.h>   /* NULL */

/* ── Private Macros ──────────────────────────────────────────────────────── */
#define LIB_TXT_DECIMAL_BASE         (10U)
#define LIB_TXT_UINT32_MAX_DIGITS    (10U)
#define LIB_TXT_MIN_CAPACITY         (2U)
#define LIB_TXT_ASCII_ZERO           ('0')
#define LIB_TXT_ASCII_MINUS          ('-')
#define LIB_TXT_ASCII_POINT          ('.')
#define LIB_TXT_ASCII_NUL            (0U)

/* ── Private Function Prototypes ─────────────────────────────────────────── */
static bool LIB_TXT_AppendChar(LIB_TXT_Buffer_t *buffer, char character);
static bool LIB_TXT_AppendUint32(LIB_TXT_Buffer_t *buffer, uint32_t value);

/* ── Public Function Definitions ─────────────────────────────────────────── */

bool LIB_TXT_Init(LIB_TXT_Buffer_t *buffer, uint8_t *storage, uint16_t capacity)
{
    bool init_ok = false;

    if ((buffer != NULL) && (storage != NULL) && (capacity >= LIB_TXT_MIN_CAPACITY))
    {
        buffer->storage        = storage;
        buffer->capacity       = capacity;
        buffer->length         = 0U;
        buffer->has_overflowed = false;
        buffer->storage[0]     = (uint8_t)LIB_TXT_ASCII_NUL;
        init_ok                = true;
    }

    return init_ok;
}


bool LIB_TXT_AppendString(LIB_TXT_Buffer_t *buffer, const char *text)
{
    bool    all_fitted = false;
    uint16_t index     = 0U;

    if ((buffer != NULL) && (text != NULL))
    {
        all_fitted = true;

        while (text[index] != '\0')
        {
            if (!LIB_TXT_AppendChar(buffer, text[index]))
            {
                all_fitted = false;
            }

            index++;
        }
    }

    return all_fitted;
}


bool LIB_TXT_AppendInt32(LIB_TXT_Buffer_t *buffer, int32_t value)
{
    bool     all_fitted = false;
    uint32_t magnitude  = 0U;

    if (buffer != NULL)
    {
        all_fitted = true;

        if (value < 0)
        {
            /* Unsigned negation is well defined, including for the most negative value. */
            magnitude = 0U - (uint32_t)value;

            if (!LIB_TXT_AppendChar(buffer, LIB_TXT_ASCII_MINUS))
            {
                all_fitted = false;
            }
        }
        else
        {
            magnitude = (uint32_t)value;
        }

        if (!LIB_TXT_AppendUint32(buffer, magnitude))
        {
            all_fitted = false;
        }
    }

    return all_fitted;
}


bool LIB_TXT_AppendTenths(LIB_TXT_Buffer_t *buffer, int32_t tenths)
{
    bool     all_fitted = false;
    uint32_t magnitude  = 0U;

    if (buffer != NULL)
    {
        all_fitted = true;

        if (tenths < 0)
        {
            magnitude = 0U - (uint32_t)tenths;

            if (!LIB_TXT_AppendChar(buffer, LIB_TXT_ASCII_MINUS))
            {
                all_fitted = false;
            }
        }
        else
        {
            magnitude = (uint32_t)tenths;
        }

        if (!LIB_TXT_AppendUint32(buffer, magnitude / LIB_TXT_DECIMAL_BASE))
        {
            all_fitted = false;
        }

        if (!LIB_TXT_AppendChar(buffer, LIB_TXT_ASCII_POINT))
        {
            all_fitted = false;
        }

        if (!LIB_TXT_AppendUint32(buffer, magnitude % LIB_TXT_DECIMAL_BASE))
        {
            all_fitted = false;
        }
    }

    return all_fitted;
}


bool LIB_TXT_HasOverflowed(const LIB_TXT_Buffer_t *buffer)
{
    bool overflowed = true;

    if (buffer != NULL)
    {
        overflowed = buffer->has_overflowed;
    }

    return overflowed;
}


uint16_t LIB_TXT_GetLength(const LIB_TXT_Buffer_t *buffer)
{
    uint16_t length = 0U;

    if (buffer != NULL)
    {
        length = buffer->length;
    }

    return length;
}

/* ── Private Function Definitions ────────────────────────────────────────── */

/**
 * @brief  Append one character, always keeping room for the terminator.
 * @param  buffer    Initialised buffer.
 * @param  character Character to append.
 * @return true if stored, false if the buffer is full (overflow is latched).
 */
static bool LIB_TXT_AppendChar(LIB_TXT_Buffer_t *buffer, char character)
{
    bool appended = false;

    if (((uint32_t)buffer->length + 1U) < (uint32_t)buffer->capacity)
    {
        buffer->storage[buffer->length] = (uint8_t)character;
        buffer->length++;
        buffer->storage[buffer->length] = (uint8_t)LIB_TXT_ASCII_NUL;
        appended = true;
    }
    else
    {
        buffer->has_overflowed = true;
    }

    return appended;
}


/**
 * @brief  Append an unsigned integer in decimal.
 * @param  buffer Initialised buffer.
 * @param  value  Value to append.
 * @return true if everything fitted, false if truncated.
 */
static bool LIB_TXT_AppendUint32(LIB_TXT_Buffer_t *buffer, uint32_t value)
{
    char     digits[LIB_TXT_UINT32_MAX_DIGITS];
    uint8_t  digit_count = 0U;
    uint32_t remaining   = value;
    bool     all_fitted  = true;

    do
    {
        digits[digit_count] = (char)((uint32_t)LIB_TXT_ASCII_ZERO +
                                     (remaining % LIB_TXT_DECIMAL_BASE));
        digit_count++;
        remaining /= LIB_TXT_DECIMAL_BASE;
    }
    while (remaining > 0U);

    while (digit_count > 0U)
    {
        digit_count--;

        if (!LIB_TXT_AppendChar(buffer, digits[digit_count]))
        {
            all_fitted = false;
        }
    }

    return all_fitted;
}
