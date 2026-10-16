/**
 * @file    lib_fem.c
 * @author  Madesh
 * @brief   Fault and Event Manager (LIB_FEM) - minimal ring-log implementation.
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
#include "lib_fem.h"
#include <stdint.h>
#include <stdbool.h>
#include <string.h>   /* NULL */

/* ── Private Variables ───────────────────────────────────────────────────── */
static LIB_FEM_Event_t     FEM_event_log[LIB_FEM_MAX_ACTIVE_FAULTS];
static uint32_t            FEM_event_count    = 0U;
static PIF_TIME_GetMsFn    FEM_get_ms         = NULL;
static LIB_FEM_Callback_Fn FEM_on_event       = NULL;
static bool                FEM_is_initialised = false;

/* ── Public Function Definitions ─────────────────────────────────────────── */

bool LIB_FEM_Init(const LIB_FEM_Config_t *config)
{
    bool init_ok = false;

    if ((config != NULL) && (config->get_ms != NULL))
    {
        FEM_get_ms         = config->get_ms;
        FEM_on_event       = config->on_event;
        FEM_event_count    = 0U;
        FEM_is_initialised = true;
        init_ok            = true;
    }

    return init_ok;
}


bool LIB_FEM_Report(LIB_FEM_EventId_e id,
                    LIB_FEM_Severity_e severity,
                    LIB_FEM_Source_e source,
                    uint8_t node_id,
                    const uint8_t *payload,
                    uint16_t payload_len)
{
    bool             report_ok = false;
    uint16_t         copy_len  = 0U;
    uint16_t         index     = 0U;
    LIB_FEM_Event_t *slot      = NULL;

    if (FEM_is_initialised && ((payload != NULL) || (payload_len == 0U)))
    {
        slot = &FEM_event_log[FEM_event_count % LIB_FEM_MAX_ACTIVE_FAULTS];

        slot->timestamp_ms = FEM_get_ms();
        slot->id           = id;
        slot->severity     = severity;
        slot->source       = source;
        slot->node_id      = node_id;

        copy_len = (payload_len > LIB_FEM_PAYLOAD_SIZE_BYTES) ?
                   (uint16_t)LIB_FEM_PAYLOAD_SIZE_BYTES : payload_len;

        for (index = 0U; index < copy_len; index++)
        {
            slot->payload[index] = payload[index];
        }

        slot->payload_len = (uint8_t)copy_len;
        FEM_event_count++;

        if (FEM_on_event != NULL)
        {
            FEM_on_event(slot);
        }

        report_ok = true;
    }

    return report_ok;
}


uint32_t LIB_FEM_GetEventCount(void)
{
    return FEM_event_count;
}


bool LIB_FEM_GetLatest(LIB_FEM_Event_t *event)
{
    bool has_event = false;

    if ((event != NULL) && FEM_is_initialised && (FEM_event_count > 0U))
    {
        *event    = FEM_event_log[(FEM_event_count - 1U) % LIB_FEM_MAX_ACTIVE_FAULTS];
        has_event = true;
    }

    return has_event;
}
