/**
 * @file    lib_fem.h
 * @author  Madesh
 * @brief   Public interface for the Fault and Event Manager (LIB_FEM) module.
 * @version 0.1
 * @date    2026-10-01
 *
 * @note
 * - Minimal local implementation: a ring log plus an optional callback.
 * - No CRC and no persistent storage. Replace with the shared common_libs version.
 * - Zero SDK includes, as required for the LIB_ layer.
 *
 * @revision_history
 * - 0.1 2026-10-01 Madesh Initial creation.
 *
 * @copyright Copyright (c) 2026 CIOET
 */

#ifndef LIB_FEM_H
#define LIB_FEM_H

/* ── Includes ────────────────────────────────────────────────────────────── */
#include <stdint.h>
#include <stdbool.h>
#include "time_if.h"

/* ── Public Macros ───────────────────────────────────────────────────────── */
#define LIB_FEM_MAX_ACTIVE_FAULTS     (32U)
#define LIB_FEM_PAYLOAD_SIZE_BYTES    (8U)

/* ── Public Types ────────────────────────────────────────────────────────── */
typedef enum
{
    LIB_FEM_SEV_DEBUG    = 0U,
    LIB_FEM_SEV_INFO     = 1U,
    LIB_FEM_SEV_WARNING  = 2U,
    LIB_FEM_SEV_ALARM    = 3U,
    LIB_FEM_SEV_FAULT    = 4U,
    LIB_FEM_SEV_CRITICAL = 5U
} LIB_FEM_Severity_e;

typedef enum
{
    LIB_FEM_EVT_NONE                 = 0U,
    LIB_FEM_EVT_SENSOR_INIT_FAILED   = 1U,
    LIB_FEM_EVT_SENSOR_NO_DATA       = 2U,
    LIB_FEM_EVT_PRESSURE_OVER_LIMIT  = 3U,
    LIB_FEM_EVT_PUMP_ON_TIMEOUT      = 4U,
    LIB_FEM_EVT_PUMP_DRIVER_FAILED   = 5U,
    LIB_FEM_EVT_GPIO_INVALID_PIN     = 6U,
    LIB_FEM_EVT_INVALID_STATE        = 7U,
    LIB_FEM_EVT_VENT_FAILED          = 8U,
    LIB_FEM_EVT_POT_READ_FAILED      = 9U,
    LIB_FEM_EVT_VALVE_DRIVER_FAILED  = 10U,
    LIB_FEM_EVT_PHASE_TIMEOUT        = 11U,
    LIB_FEM_EVT_PEAK_LOW             = 12U,
    LIB_FEM_EVT_INFLATE_FAILED       = 13U
} LIB_FEM_EventId_e;

typedef enum
{
    LIB_FEM_SRC_APP     = 0U,
    LIB_FEM_SRC_SENSOR1 = 1U,
    LIB_FEM_SRC_SENSOR2 = 2U,
    LIB_FEM_SRC_PUMP    = 3U,
    LIB_FEM_SRC_MW      = 4U,
    LIB_FEM_SRC_VALVE1  = 5U,
    LIB_FEM_SRC_VALVE2  = 6U,
    LIB_FEM_SRC_POT     = 7U
} LIB_FEM_Source_e;

typedef struct
{
    uint32_t           timestamp_ms;
    LIB_FEM_EventId_e  id;
    LIB_FEM_Severity_e severity;
    LIB_FEM_Source_e   source;
    uint8_t            node_id;
    uint8_t            payload_len;
    uint8_t            payload[LIB_FEM_PAYLOAD_SIZE_BYTES];
} LIB_FEM_Event_t;

typedef void (*LIB_FEM_Callback_Fn)(const LIB_FEM_Event_t *event);

typedef struct
{
    PIF_TIME_GetMsFn    get_ms;
    LIB_FEM_Callback_Fn on_event;
} LIB_FEM_Config_t;

/* ── Public Function Prototypes ──────────────────────────────────────────── */

/**
 * @name    LIB_FEM_Init
 * @brief   Initialise the fault and event manager.
 * @param   config Pointer to a valid LIB_FEM_Config_t. get_ms must not be NULL.
 * @return  true on success, false if the configuration is invalid.
 * @note    Must be called before any other LIB_FEM_ function.
 */
bool LIB_FEM_Init(const LIB_FEM_Config_t *config);

/**
 * @name    LIB_FEM_Report
 * @brief   Record an event and invoke the registered callback.
 * @param   id          Event identifier.
 * @param   severity    Event severity.
 * @param   source      Module that detected the event.
 * @param   node_id     Node identifier of the reporting device.
 * @param   payload     Optional data, or NULL when payload_len is 0.
 * @param   payload_len Number of payload bytes. Extra bytes are truncated.
 * @return  true if the event was recorded, false if not initialised or arguments are invalid.
 */
bool LIB_FEM_Report(LIB_FEM_EventId_e id,
                    LIB_FEM_Severity_e severity,
                    LIB_FEM_Source_e source,
                    uint8_t node_id,
                    const uint8_t *payload,
                    uint16_t payload_len);

/**
 * @name    LIB_FEM_GetEventCount
 * @brief   Read how many events have been recorded since initialisation.
 * @return  Total event count.
 */
uint32_t LIB_FEM_GetEventCount(void);

/**
 * @name    LIB_FEM_GetLatest
 * @brief   Copy out the most recent event.
 * @param   event Receives the event. Must not be NULL.
 * @return  true if an event exists, false if none has been recorded.
 */
bool LIB_FEM_GetLatest(LIB_FEM_Event_t *event);

#endif /* LIB_FEM_H */
