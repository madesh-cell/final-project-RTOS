/**
 * @file    app_pad.c
 * @author  Madesh
 * @brief   Smart Pneumatic Support Pad, 9-pillow test: group A (1) and group B (2), one pump,
 *          two 3/2 valves.
 *
 * @note
 * - FILL A: group A fills to 50 mmHg, group B vents. HOLD: 5 minutes, pump off (topped up
 *   below 49 mmHg), the other group keeps venting (the non-contact space).
 * - TRANSFER: the other group's valve is also put on the pump line, pump off, so air flows
 *   from the full group into it. At 15 mmHg (30% of full, liver safety) the old group's valve
 *   is released: it vents quickly and does not have to reach 0 mmHg.
 * - REFILL: the pump fills the new group to 50 mmHg, then HOLD again with the roles swapped.
 * - The potentiometer sets the pump speed (80 to 100%).
 * - Safety: lost samples, 60 mmHg stop (proposed), fill and transfer time limits.
 * - Button: a short press while running HOLDS (pump off, valves frozen, pressure kept, timers
 *   paused); a short press again resumes. Hold the button 3 s to stop and vent both groups.
 * - Faults: the pump stops and the pad goes to HELD with the pressure kept, and the LED blinks
 *   the fault code. Over-pressure is the exception: both groups vent, then IDLE.
 *   A short press after a fault restarts the cycle.
 * - A missing baseline latches FAULT with both groups venting.
 * - RTOS: five task steps (sensors, button, safety, control, display). Each takes the shared-data
 *   lock for its whole run. The sensor bit-banging runs with task switching stopped.
  * - No MISRA C deviations in this file.
 *
 * @revision_history
 * - 0.1 2026-10-01 Madesh Initial creation.
 * - 0.3 2026-10-03 Madesh Two valves, fail-safe venting, pump speed by PWM.
 * - 0.4 2026-10-09 Madesh 9-pillow test: air-transfer swap, 50 mmHg full, 5 min hold.
 *
 * @copyright Copyright (c) 2026 CIOET
 */

/* ── Includes ────────────────────────────────────────────────────────────── */
#include "app_pad.h"
#include <stdint.h>
#include <stdbool.h>
#include <string.h>   /* NULL */
#include "lib_txt.h"

/* ── Private Macros ──────────────────────────────────────────────────────── */
#define APP_PAD_CUFF_1    (0U)
#define APP_PAD_CUFF_2    (1U)

/* ── Private Types ───────────────────────────────────────────────────────── */
typedef enum
{
    PAD_BUTTON_NONE  = 0U,
    PAD_BUTTON_SHORT = 1U,
    PAD_BUTTON_LONG  = 2U
} PAD_Button_e;

typedef struct
{
    APP_Pad_Config_t config;
    APP_Pad_State_e  state;
    APP_Pad_Phase_e  phase;
    APP_Pad_Step_e   step;
    int32_t          baseline_counts[APP_PAD_CUFF_COUNT];
    int32_t          pressure_tenths[APP_PAD_CUFF_COUNT];
    uint32_t         last_sample_ms[APP_PAD_CUFF_COUNT];
    bool             valve_on_pump[APP_PAD_CUFF_COUNT];
    uint32_t         step_start_ms;
    uint32_t         last_status_ms;
    uint32_t         last_pot_ms;
    uint32_t         button_change_ms;
    uint32_t         button_press_start_ms;
    uint32_t         held_start_ms;
    uint32_t         led_phase_start_ms;
    uint16_t         knob_speed_permille;
    uint16_t         pump_speed_permille;
    bool             is_initialised;
    bool             baseline_is_valid;
    bool             is_topping_up;
    bool             pot_error_reported;
    bool             button_raw_pressed;
    bool             button_stable_pressed;
    bool             button_long_fired;
    PAD_Button_e     pending_button;
    bool             held_by_fault;
    uint8_t          fault_code;
} APP_Pad_Context_t;

/* ── Private Variables ───────────────────────────────────────────────────── */
static APP_Pad_Context_t PAD_context =
{
    .state = APP_PAD_STATE_IDLE,
    .phase = APP_PAD_PHASE_INFLATE_1,
    .step  = APP_PAD_STEP_PREFILL
};

/* ── Private Function Prototypes ─────────────────────────────────────────── */
static bool                 PAD_IsConfigValid(const APP_Pad_Config_t *config);
static DRV_HX710B_Handle_t *PAD_GetSensor(uint8_t cuff);
static DRV_VALVE_Handle_t  *PAD_GetValve(uint8_t cuff);
static LIB_FEM_Source_e     PAD_GetSensorSource(uint8_t cuff);
static LIB_FEM_Source_e     PAD_GetValveSource(uint8_t cuff);
static int32_t              PAD_GetScale(uint8_t cuff);
static uint8_t              PAD_GetActiveCuff(void);
static uint8_t              PAD_GetOtherCuff(void);
static bool                 PAD_EnergiseLevel(bool on_pump);
static void                 PAD_WaitMs(uint32_t duration_ms);
static bool                 PAD_MeasureBaseline(DRV_HX710B_Handle_t *sensor, int32_t *baseline);
static int32_t              PAD_CountsToTenths(int32_t delta_counts, int32_t counts_per_mmhg);
static void                 PAD_UpdateSensors(uint32_t now_ms);
static bool                 PAD_IsSensorFresh(uint8_t cuff, uint32_t now_ms);
static void                 PAD_UpdatePot(uint32_t now_ms);
static PAD_Button_e         PAD_ReadButton(uint32_t now_ms);
static void                 PAD_HandleButton(PAD_Button_e button_event, uint32_t now_ms);
static void                 PAD_CheckSafety(uint32_t now_ms);
static void                 PAD_RunStep(uint32_t now_ms);
static bool                 PAD_BuildStatus(uint32_t now_ms, LIB_TXT_Buffer_t *text, uint8_t *storage);
static void                 PAD_Hold(uint32_t now_ms, bool by_fault);
static void                 PAD_Resume(uint32_t now_ms);
static uint8_t              PAD_FaultCode(LIB_FEM_EventId_e event_id);
static void                 PAD_UpdateLed(uint32_t now_ms);
static uint16_t             PAD_ComputeSpeed(int32_t pressure_tenths);
static void                 PAD_SetPumpSpeed(uint16_t speed_permille);
static void                 PAD_SetValve(uint8_t cuff, bool on_pump);
static bool                 PAD_VentAll(void);
static void                 PAD_StartRun(uint32_t now_ms);
static void                 PAD_StopRun(void);
static bool                 PAD_FindOverLimit(LIB_FEM_Source_e *source);
static void                 PAD_RunPrefill(uint32_t now_ms);
static void                 PAD_RunInflate(uint32_t now_ms);
static void                 PAD_RunHold(uint32_t now_ms);
static void                 PAD_RunTransfer(uint32_t now_ms);
static void                 PAD_EnterFault(LIB_FEM_EventId_e event_id, LIB_FEM_Source_e source);
static void                 PAD_Print(const LIB_TXT_Buffer_t *text);
static void                 PAD_PrintMessage(const char *message);
static void                 PAD_PrintBaselines(void);
static void                 PAD_PrintTransfer(int32_t new_tenths);
static const char          *PAD_StepLetter(void);
static const char          *PAD_ValveLetter(uint8_t cuff);

/* ── Public Function Definitions ─────────────────────────────────────────── */

bool APP_Pad_Init(const APP_Pad_Config_t *config)
{
    bool     init_ok     = false;
    bool     pump_off_ok = false;
    bool     vent_ok     = false;
    bool     sensor1_ok  = false;
    bool     sensor2_ok  = false;
    uint32_t now_ms      = 0U;

    if (PAD_IsConfigValid(config))
    {
        PAD_context.config              = *config;
        PAD_context.knob_speed_permille = APP_PAD_SPEED_MIN_PERMILLE;

        pump_off_ok = DRV_PUMP_SetSpeed(PAD_context.config.pump, 0U);
        PAD_context.pump_speed_permille = 0U;
        PAD_context.config.gpio_set(PAD_context.config.led_pin_id, false);

        vent_ok = PAD_VentAll();

        /* Both chambers are open to air now. Give them time to empty, so the zero points
         * are taken at ambient pressure without anyone having to remove the tubes. */
        PAD_WaitMs(APP_PAD_VENT_SETTLE_MS);

        sensor1_ok = PAD_MeasureBaseline(PAD_GetSensor(APP_PAD_CUFF_1),
                                         &PAD_context.baseline_counts[APP_PAD_CUFF_1]);

        if (!sensor1_ok)
        {
            (void)LIB_FEM_Report(LIB_FEM_EVT_SENSOR_INIT_FAILED, LIB_FEM_SEV_FAULT,
                                 LIB_FEM_SRC_SENSOR1, APP_PAD_NODE_ID, NULL, 0U);
        }

        sensor2_ok = PAD_MeasureBaseline(PAD_GetSensor(APP_PAD_CUFF_2),
                                         &PAD_context.baseline_counts[APP_PAD_CUFF_2]);

        if (!sensor2_ok)
        {
            (void)LIB_FEM_Report(LIB_FEM_EVT_SENSOR_INIT_FAILED, LIB_FEM_SEV_FAULT,
                                 LIB_FEM_SRC_SENSOR2, APP_PAD_NODE_ID, NULL, 0U);
        }

        if (!pump_off_ok)
        {
            (void)LIB_FEM_Report(LIB_FEM_EVT_PUMP_DRIVER_FAILED, LIB_FEM_SEV_CRITICAL,
                                 LIB_FEM_SRC_PUMP, APP_PAD_NODE_ID, NULL, 0U);
        }

        if (!vent_ok)
        {
            (void)LIB_FEM_Report(LIB_FEM_EVT_VALVE_DRIVER_FAILED, LIB_FEM_SEV_CRITICAL,
                                 LIB_FEM_SRC_APP, APP_PAD_NODE_ID, NULL, 0U);
        }

        now_ms = PAD_context.config.get_ms();

        PAD_context.baseline_is_valid = sensor1_ok && sensor2_ok;
        PAD_context.last_sample_ms[APP_PAD_CUFF_1] = now_ms;
        PAD_context.last_sample_ms[APP_PAD_CUFF_2] = now_ms;
        PAD_context.last_status_ms                 = now_ms;
        PAD_context.last_pot_ms                    = now_ms - APP_PAD_POT_PERIOD_MS;
        PAD_context.is_initialised                 = true;

        if (PAD_context.baseline_is_valid && pump_off_ok && vent_ok)
        {
            PAD_context.state = APP_PAD_STATE_IDLE;
            PAD_PrintBaselines();
            init_ok = true;
        }
        else
        {
            PAD_context.state = APP_PAD_STATE_FAULT;
        }
    }

    return init_ok;
}


void APP_Pad_RunSensors(void)
{
    uint32_t now_ms = 0U;

    if (PAD_context.is_initialised && PAD_context.config.os_lock())
    {
        now_ms = PAD_context.config.get_ms();
        PAD_UpdateSensors(now_ms);
        PAD_UpdatePot(now_ms);
        PAD_context.config.os_unlock();
    }
}


void APP_Pad_RunButton(void)
{
    uint32_t     now_ms = 0U;
    PAD_Button_e event  = PAD_BUTTON_NONE;

    if (PAD_context.is_initialised && PAD_context.config.os_lock())
    {
        now_ms = PAD_context.config.get_ms();
        event  = PAD_ReadButton(now_ms);

        /* A long press is never replaced by a short one that arrives before it is handled. */
        if (event == PAD_BUTTON_LONG)
        {
            PAD_context.pending_button = PAD_BUTTON_LONG;
        }
        else if ((event == PAD_BUTTON_SHORT) && (PAD_context.pending_button == PAD_BUTTON_NONE))
        {
            PAD_context.pending_button = PAD_BUTTON_SHORT;
        }
        else
        {
            /* No new press. */
        }

        PAD_context.config.os_unlock();
    }
}


void APP_Pad_RunSafety(void)
{
    if (PAD_context.is_initialised && PAD_context.config.os_lock())
    {
        PAD_CheckSafety(PAD_context.config.get_ms());
        PAD_context.config.os_unlock();
    }
}


void APP_Pad_RunControl(void)
{
    uint32_t     now_ms = 0U;
    PAD_Button_e event  = PAD_BUTTON_NONE;

    if (PAD_context.is_initialised && PAD_context.config.os_lock())
    {
        now_ms = PAD_context.config.get_ms();
        event  = PAD_context.pending_button;
        PAD_context.pending_button = PAD_BUTTON_NONE;

        PAD_HandleButton(event, now_ms);

        if (PAD_context.state == APP_PAD_STATE_RUNNING)
        {
            PAD_RunStep(now_ms);
        }
        else if (PAD_context.state == APP_PAD_STATE_HELD)
        {
            /* Held: pump off, valves stay where they are. */
            PAD_SetPumpSpeed(0U);
        }
        else
        {
            /* Idle or fault: pump off and both chambers open to exhaust. */
            PAD_SetPumpSpeed(0U);
            PAD_SetValve(APP_PAD_CUFF_1, false);
            PAD_SetValve(APP_PAD_CUFF_2, false);
        }

        PAD_context.config.os_unlock();
    }
}


void APP_Pad_RunDisplay(void)
{
    uint8_t          storage[APP_PAD_TEXT_BUFFER_SIZE_BYTES];
    LIB_TXT_Buffer_t text;
    bool             line_ready = false;
    uint32_t         now_ms     = 0U;

    if (PAD_context.is_initialised && PAD_context.config.os_lock())
    {
        now_ms     = PAD_context.config.get_ms();
        PAD_UpdateLed(now_ms);
        line_ready = PAD_BuildStatus(now_ms, &text, storage);
        PAD_context.config.os_unlock();

        /* The slow UART write happens after the lock is released. */
        if (line_ready)
        {
            PAD_Print(&text);
        }
    }
}


void APP_Pad_Run(void)
{
    APP_Pad_RunSensors();
    APP_Pad_RunButton();
    APP_Pad_RunSafety();
    APP_Pad_RunControl();
    APP_Pad_RunDisplay();
}


APP_Pad_State_e APP_Pad_GetState(void)
{
    return PAD_context.state;
}


APP_Pad_Phase_e APP_Pad_GetPhase(void)
{
    return PAD_context.phase;
}


APP_Pad_Step_e APP_Pad_GetStep(void)
{
    return PAD_context.step;
}


void APP_Pad_OnFemEvent(const LIB_FEM_Event_t *event)
{
    uint8_t          storage[APP_PAD_TEXT_BUFFER_SIZE_BYTES];
    LIB_TXT_Buffer_t text;

    if ((event != NULL) &&
        LIB_TXT_Init(&text, storage, (uint16_t)APP_PAD_TEXT_BUFFER_SIZE_BYTES))
    {
        (void)LIB_TXT_AppendString(&text, "EVENT id=");
        (void)LIB_TXT_AppendInt32(&text, (int32_t)event->id);
        (void)LIB_TXT_AppendString(&text, " sev=");
        (void)LIB_TXT_AppendInt32(&text, (int32_t)event->severity);
        (void)LIB_TXT_AppendString(&text, " src=");
        (void)LIB_TXT_AppendInt32(&text, (int32_t)event->source);
        (void)LIB_TXT_AppendString(&text, " t=");
        (void)LIB_TXT_AppendInt32(&text, (int32_t)event->timestamp_ms);
        (void)LIB_TXT_AppendString(&text, "\r\n");
        PAD_Print(&text);
    }
}

/* ── Private Function Definitions ────────────────────────────────────────── */

/**
 * @brief  Check that a configuration is complete and the calibration constants are usable.
 * @param  config Configuration to check, may be NULL.
 * @return true if usable, false otherwise.
 */
static bool PAD_IsConfigValid(const APP_Pad_Config_t *config)
{
    bool is_valid = false;

    if (config != NULL)
    {
        is_valid = (config->sensor1 != NULL) && (config->sensor2 != NULL) &&
                   (config->pump != NULL) && (config->valve1 != NULL) &&
                   (config->valve2 != NULL) && (config->pot != NULL) &&
                   (config->gpio_set != NULL) && (config->gpio_read != NULL) &&
                   (config->get_ms != NULL) && (config->uart_write != NULL) &&
                   (config->os_lock != NULL) && (config->os_unlock != NULL) &&
                   (config->os_sched_lock != NULL) && (config->os_sched_unlock != NULL) &&
                   (APP_PAD_COUNTS_PER_MMHG_SENSOR1 > 0) &&
                   (APP_PAD_COUNTS_PER_MMHG_SENSOR2 > 0);
    }

    return is_valid;
}


/**
 * @brief  Select the sensor that belongs to a chamber.
 * @param  cuff APP_PAD_CUFF_1 or APP_PAD_CUFF_2.
 * @return Sensor handle.
 */
static DRV_HX710B_Handle_t *PAD_GetSensor(uint8_t cuff)
{
    return (cuff == APP_PAD_CUFF_1) ? PAD_context.config.sensor1 : PAD_context.config.sensor2;
}


/**
 * @brief  Select the valve that belongs to a chamber.
 * @param  cuff APP_PAD_CUFF_1 or APP_PAD_CUFF_2.
 * @return Valve handle.
 */
static DRV_VALVE_Handle_t *PAD_GetValve(uint8_t cuff)
{
    return (cuff == APP_PAD_CUFF_1) ? PAD_context.config.valve1 : PAD_context.config.valve2;
}


/**
 * @brief  Event source identifier of a chamber's sensor.
 * @param  cuff APP_PAD_CUFF_1 or APP_PAD_CUFF_2.
 * @return LIB_FEM source identifier.
 */
static LIB_FEM_Source_e PAD_GetSensorSource(uint8_t cuff)
{
    return (cuff == APP_PAD_CUFF_1) ? LIB_FEM_SRC_SENSOR1 : LIB_FEM_SRC_SENSOR2;
}


/**
 * @brief  Event source identifier of a chamber's valve.
 * @param  cuff APP_PAD_CUFF_1 or APP_PAD_CUFF_2.
 * @return LIB_FEM source identifier.
 */
static LIB_FEM_Source_e PAD_GetValveSource(uint8_t cuff)
{
    return (cuff == APP_PAD_CUFF_1) ? LIB_FEM_SRC_VALVE1 : LIB_FEM_SRC_VALVE2;
}


/**
 * @brief  Calibrated scale of a chamber's sensor.
 * @param  cuff APP_PAD_CUFF_1 or APP_PAD_CUFF_2.
 * @return Raw counts per mmHg.
 */
static int32_t PAD_GetScale(uint8_t cuff)
{
    return (cuff == APP_PAD_CUFF_1) ? APP_PAD_COUNTS_PER_MMHG_SENSOR1 :
                                      APP_PAD_COUNTS_PER_MMHG_SENSOR2;
}


/**
 * @brief  The chamber that inflates (or holds at the maximum) in the current phase.
 * @return APP_PAD_CUFF_1 or APP_PAD_CUFF_2.
 */
static uint8_t PAD_GetActiveCuff(void)
{
    uint8_t cuff = APP_PAD_CUFF_2;

    if (PAD_context.phase == APP_PAD_PHASE_INFLATE_1)
    {
        cuff = APP_PAD_CUFF_1;
    }

    return cuff;
}


/**
 * @brief  The chamber that vents in the current phase.
 * @return APP_PAD_CUFF_1 or APP_PAD_CUFF_2.
 */
static uint8_t PAD_GetOtherCuff(void)
{
    uint8_t cuff = APP_PAD_CUFF_1;

    if (PAD_context.phase == APP_PAD_PHASE_INFLATE_1)
    {
        cuff = APP_PAD_CUFF_2;
    }

    return cuff;
}


/**
 * @brief  Coil state that produces a given air path.
 * @param  on_pump true for chamber-to-pump-line, false for chamber-to-exhaust.
 * @return true if the coil must be energised for that path.
 */
static bool PAD_EnergiseLevel(bool on_pump)
{
    return (on_pump != APP_PAD_VALVE_VENTS_WHEN_ENERGISED);
}


/**
 * @brief Blocking wait, used only during start-up.
 * @param duration_ms Time to wait in milliseconds.
 */
static void PAD_WaitMs(uint32_t duration_ms)
{
    uint32_t start_ms = PAD_context.config.get_ms();

    while ((PAD_context.config.get_ms() - start_ms) < duration_ms)
    {
        /* Nothing to do: the chambers are emptying through the open valves. */
    }
}


/**
 * @brief  Average resting samples of one sensor to find its zero point.
 * @param  sensor   Initialised sensor instance.
 * @param  baseline Receives the averaged raw reading on success.
 * @return true if enough samples arrived in time, false otherwise.
 * @note   The first sample after power-up can be stale and is discarded.
 */
static bool PAD_MeasureBaseline(DRV_HX710B_Handle_t *sensor, int32_t *baseline)
{
    int64_t             sum          = 0;
    uint32_t            sample_count = 0U;
    uint32_t            start_ms     = PAD_context.config.get_ms();
    int32_t             raw_counts   = 0;
    bool                has_failed   = false;
    bool                success      = false;
    DRV_HX710B_Status_e status       = DRV_HX710B_STATUS_NOT_READY;

    while ((sample_count < (APP_PAD_BASELINE_SAMPLES + 1U)) && !has_failed)
    {
        status = DRV_HX710B_Poll(sensor, &raw_counts);

        if (status == DRV_HX710B_STATUS_OK)
        {
            if (sample_count > 0U)
            {
                sum += raw_counts;
            }

            sample_count++;
        }
        else if (status == DRV_HX710B_STATUS_ERROR)
        {
            has_failed = true;
        }
        else
        {
            /* No sample yet: keep waiting until the timeout below. */
        }

        if ((PAD_context.config.get_ms() - start_ms) > APP_PAD_BASELINE_TIMEOUT_MS)
        {
            has_failed = true;
        }
    }

    if (sample_count >= (APP_PAD_BASELINE_SAMPLES + 1U))
    {
        *baseline = (int32_t)(sum / (int64_t)APP_PAD_BASELINE_SAMPLES);
        success   = true;
    }

    return success;
}


/**
 * @brief  Convert a raw count difference to tenths of mmHg.
 * @param  delta_counts    Rise above the baseline in raw counts.
 * @param  counts_per_mmhg Calibrated scale of the sensor, must be greater than zero.
 * @return Pressure in tenths of mmHg.
 */
static int32_t PAD_CountsToTenths(int32_t delta_counts, int32_t counts_per_mmhg)
{
    return (int32_t)(((int64_t)delta_counts * (int64_t)APP_PAD_TENTHS_PER_MMHG) /
                     (int64_t)counts_per_mmhg);
}


/**
 * @brief Poll both sensors and keep the latest pressure of each chamber.
 * @param now_ms Current time in milliseconds.
 */
static void PAD_UpdateSensors(uint32_t now_ms)
{
    uint8_t             cuff       = 0U;
    int32_t             raw_counts = 0;
    DRV_HX710B_Status_e status     = DRV_HX710B_STATUS_NOT_READY;

    for (cuff = 0U; cuff < APP_PAD_CUFF_COUNT; cuff++)
    {
        /* The sensor is read bit by bit. If another task ran in the middle of a clock pulse,
         * the chip could power down, so task switching is stopped for the read. */
        PAD_context.config.os_sched_lock();
        status = DRV_HX710B_Poll(PAD_GetSensor(cuff), &raw_counts);
        PAD_context.config.os_sched_unlock();

        if (status == DRV_HX710B_STATUS_OK)
        {
            PAD_context.pressure_tenths[cuff] =
                PAD_CountsToTenths(raw_counts - PAD_context.baseline_counts[cuff],
                                   PAD_GetScale(cuff));
            PAD_context.last_sample_ms[cuff] = now_ms;
        }
    }
}


/**
 * @brief  Check that a chamber's sensor delivered a sample recently.
 * @param  cuff   APP_PAD_CUFF_1 or APP_PAD_CUFF_2.
 * @param  now_ms Current time in milliseconds.
 * @return true if the latest sample is recent enough to trust.
 */
static bool PAD_IsSensorFresh(uint8_t cuff, uint32_t now_ms)
{
    return ((now_ms - PAD_context.last_sample_ms[cuff]) <= APP_PAD_SENSOR_TIMEOUT_MS);
}


/**
 * @brief Read the potentiometer at a fixed interval and convert it to the pump speed.
 * @param now_ms Current time in milliseconds.
 * @note  A failed read keeps the last speed and is reported once per failure episode.
 */
static void PAD_UpdatePot(uint32_t now_ms)
{
    uint16_t permille = 0U;
    uint32_t span     = APP_PAD_SPEED_MAX_PERMILLE - APP_PAD_SPEED_MIN_PERMILLE;

    if ((now_ms - PAD_context.last_pot_ms) >= APP_PAD_POT_PERIOD_MS)
    {
        PAD_context.last_pot_ms = now_ms;

        if (DRV_POT_ReadPermille(PAD_context.config.pot, &permille))
        {
            PAD_context.knob_speed_permille =
                (uint16_t)(APP_PAD_SPEED_MIN_PERMILLE +
                           ((span * (uint32_t)permille) / DRV_POT_PERMILLE_MAX));
            PAD_context.pot_error_reported = false;
        }
        else if (!PAD_context.pot_error_reported)
        {
            PAD_context.pot_error_reported = true;
            (void)LIB_FEM_Report(LIB_FEM_EVT_POT_READ_FAILED, LIB_FEM_SEV_WARNING,
                                 LIB_FEM_SRC_POT, APP_PAD_NODE_ID, NULL, 0U);
        }
        else
        {
            /* Already reported: keep the last speed until a read succeeds again. */
        }
    }
}


/**
 * @brief  Debounce the button and classify a press.
 * @param  now_ms Current time in milliseconds.
 * @return PAD_BUTTON_SHORT when a press shorter than APP_PAD_LONG_PRESS_MS is released,
 *         PAD_BUTTON_LONG once, as soon as the button has been held that long.
 */
static PAD_Button_e PAD_ReadButton(uint32_t now_ms)
{
    bool         is_pressed = !PAD_context.config.gpio_read(PAD_context.config.button_pin_id);
    PAD_Button_e event      = PAD_BUTTON_NONE;

    if (is_pressed != PAD_context.button_raw_pressed)
    {
        PAD_context.button_raw_pressed = is_pressed;
        PAD_context.button_change_ms   = now_ms;
    }
    else if ((now_ms - PAD_context.button_change_ms) >= APP_PAD_BUTTON_DEBOUNCE_MS)
    {
        if (is_pressed && !PAD_context.button_stable_pressed)
        {
            PAD_context.button_press_start_ms = PAD_context.button_change_ms;
            PAD_context.button_long_fired     = false;
        }
        else if (!is_pressed && PAD_context.button_stable_pressed &&
                 !PAD_context.button_long_fired)
        {
            event = PAD_BUTTON_SHORT;
        }
        else
        {
            /* No edge. */
        }

        PAD_context.button_stable_pressed = is_pressed;
    }
    else
    {
        /* Still inside the debounce window. */
    }

    if (PAD_context.button_stable_pressed && !PAD_context.button_long_fired &&
        ((now_ms - PAD_context.button_press_start_ms) >= APP_PAD_LONG_PRESS_MS))
    {
        PAD_context.button_long_fired = true;
        event                         = PAD_BUTTON_LONG;
    }

    return event;
}


/**
 * @brief Apply a confirmed button press to the state machine.
 * @param button_event Result of PAD_ReadButton.
 * @param now_ms       Current time in milliseconds.
 */
static void PAD_HandleButton(PAD_Button_e button_event, uint32_t now_ms)
{
    if (button_event == PAD_BUTTON_LONG)
    {
        if ((PAD_context.state == APP_PAD_STATE_RUNNING) ||
            (PAD_context.state == APP_PAD_STATE_HELD))
        {
            PAD_context.fault_code = 0U;
            PAD_StopRun();
            PAD_PrintMessage("STOP (long press): pump off, both groups venting\r\n");
        }
    }
    else if (button_event == PAD_BUTTON_SHORT)
    {
        switch (PAD_context.state)
        {
            case APP_PAD_STATE_IDLE:
                PAD_context.fault_code = 0U;
                PAD_StartRun(now_ms);
                PAD_PrintMessage("START: group A fills, group B vents\r\n");
                break;

            case APP_PAD_STATE_RUNNING:
                PAD_Hold(now_ms, false);
                PAD_PrintMessage("HOLD: pump off, pressure kept. Press again to resume\r\n");
                break;

            case APP_PAD_STATE_HELD:
                if (PAD_context.held_by_fault)
                {
                    PAD_context.fault_code = 0U;
                    PAD_StartRun(now_ms);
                    PAD_PrintMessage("RESTART after fault: group A fills, group B vents\r\n");
                }
                else
                {
                    PAD_Resume(now_ms);
                    PAD_PrintMessage("RESUME\r\n");
                }
                break;

            case APP_PAD_STATE_FAULT:
                if (PAD_context.baseline_is_valid)
                {
                    PAD_context.state = APP_PAD_STATE_IDLE;
                    PAD_PrintMessage("FAULT CLEARED\r\n");
                }
                else
                {
                    PAD_PrintMessage("NO BASELINE: reset the board\r\n");
                }
                break;

            default:
                PAD_EnterFault(LIB_FEM_EVT_INVALID_STATE, LIB_FEM_SRC_APP);
                break;
        }
    }
    else
    {
        /* No button event. */
    }
}


/**
 * @brief Freeze the pad: pump off, valves keep their positions, timers stop counting.
 * @param now_ms   Current time in milliseconds.
 * @param by_fault true if a fault caused the hold, so the next press restarts the cycle.
 */
static void PAD_Hold(uint32_t now_ms, bool by_fault)
{
    PAD_SetPumpSpeed(0U);
    PAD_context.is_topping_up  = false;
    PAD_context.held_start_ms  = now_ms;
    PAD_context.held_by_fault  = by_fault;
    PAD_context.state          = APP_PAD_STATE_HELD;
}


/**
 * @brief Continue after a user hold: the step timer carries on from where it stopped.
 * @param now_ms Current time in milliseconds.
 */
static void PAD_Resume(uint32_t now_ms)
{
    PAD_context.step_start_ms += (now_ms - PAD_context.held_start_ms);
    PAD_context.state          = APP_PAD_STATE_RUNNING;
}


/**
 * @brief  Blink code for a fault event.
 * @param  event_id Event that caused the fault.
 * @return 1 sensor lost, 2 over-pressure, 3 fill too slow, 4 transfer too slow, 5 other.
 */
static uint8_t PAD_FaultCode(LIB_FEM_EventId_e event_id)
{
    uint8_t code = 5U;

    if (event_id == LIB_FEM_EVT_SENSOR_NO_DATA)
    {
        code = 1U;
    }
    else if (event_id == LIB_FEM_EVT_PRESSURE_OVER_LIMIT)
    {
        code = 2U;
    }
    else if ((event_id == LIB_FEM_EVT_PHASE_TIMEOUT) || (event_id == LIB_FEM_EVT_PUMP_ON_TIMEOUT))
    {
        code = 3U;
    }
    else if (event_id == LIB_FEM_EVT_VENT_FAILED)
    {
        code = 4U;
    }
    else
    {
        /* Driver failure or invalid state: code 5. */
    }

    return code;
}


/**
 * @brief Show the last fault on the LED as a blink count while the pad is not running.
 * @param now_ms Current time in milliseconds.
 * @note  The LED shows the pump while running. The blink code stops at the next START.
 */
static void PAD_UpdateLed(uint32_t now_ms)
{
    uint32_t period   = ((uint32_t)PAD_context.fault_code *
                         (APP_PAD_LED_BLINK_ON_MS + APP_PAD_LED_BLINK_OFF_MS)) + APP_PAD_LED_PAUSE_MS;
    uint32_t phase    = 0U;
    uint32_t slot     = 0U;
    bool     led_on   = false;

    if ((PAD_context.state != APP_PAD_STATE_RUNNING) && (PAD_context.fault_code > 0U))
    {
        phase = (now_ms - PAD_context.led_phase_start_ms) % period;
        slot  = APP_PAD_LED_BLINK_ON_MS + APP_PAD_LED_BLINK_OFF_MS;

        if (phase < ((uint32_t)PAD_context.fault_code * slot))
        {
            led_on = ((phase % slot) < APP_PAD_LED_BLINK_ON_MS);
        }

        PAD_context.config.gpio_set(PAD_context.config.led_pin_id, led_on);
    }
}


/**
 * @brief  Pump speed for an inflating chamber: the knob speed, reduced near the maximum.
 * @param  pressure_tenths Pressure of the chamber being filled.
 * @return Speed in permille, 0 once the maximum is reached.
 */
static uint16_t PAD_ComputeSpeed(int32_t pressure_tenths)
{
    uint32_t speed = PAD_context.knob_speed_permille;
    uint32_t room  = 0U;
    uint32_t band  = (uint32_t)(APP_PAD_MAX_TENTHS_MMHG - APP_PAD_SLOW_START_TENTHS_MMHG);

    if (pressure_tenths >= APP_PAD_MAX_TENTHS_MMHG)
    {
        speed = 0U;
    }
    else if (pressure_tenths > APP_PAD_SLOW_START_TENTHS_MMHG)
    {
        room  = (uint32_t)(APP_PAD_MAX_TENTHS_MMHG - pressure_tenths);
        speed = (speed * room) / band;

        if (speed < APP_PAD_SPEED_MIN_PERMILLE)
        {
            speed = APP_PAD_SPEED_MIN_PERMILLE;
        }
    }
    else
    {
        /* Far from the maximum: run at the knob speed. */
    }

    return (uint16_t)speed;
}


/**
 * @brief Command the pump speed and mirror running/stopped on the LED.
 * @param speed_permille 0 to stop, otherwise the PWM duty in permille.
 */
static void PAD_SetPumpSpeed(uint16_t speed_permille)
{
    if (speed_permille != PAD_context.pump_speed_permille)
    {
        if (DRV_PUMP_SetSpeed(PAD_context.config.pump, speed_permille))
        {
            PAD_context.pump_speed_permille = speed_permille;
            PAD_context.config.gpio_set(PAD_context.config.led_pin_id, (speed_permille > 0U));
        }
        else
        {
            PAD_EnterFault(LIB_FEM_EVT_PUMP_DRIVER_FAILED, LIB_FEM_SRC_PUMP);
        }
    }
}


/**
 * @brief Connect a chamber to the pump line or to exhaust, changing the valve only if needed.
 * @param cuff    APP_PAD_CUFF_1 or APP_PAD_CUFF_2.
 * @param on_pump true for the pump line, false for exhaust.
 */
static void PAD_SetValve(uint8_t cuff, bool on_pump)
{
    if ((cuff < APP_PAD_CUFF_COUNT) && (on_pump != PAD_context.valve_on_pump[cuff]))
    {
        if (DRV_VALVE_SetEnergised(PAD_GetValve(cuff), PAD_EnergiseLevel(on_pump)))
        {
            PAD_context.valve_on_pump[cuff] = on_pump;
        }
        else
        {
            PAD_EnterFault(LIB_FEM_EVT_VALVE_DRIVER_FAILED, PAD_GetValveSource(cuff));
        }
    }
}


/**
 * @brief  Command both valves to exhaust, regardless of what they were doing.
 * @return true if both writes worked.
 * @note   Writes to the drivers directly, so it can be used inside PAD_EnterFault.
 */
static bool PAD_VentAll(void)
{
    bool    all_ok = true;
    uint8_t cuff   = 0U;

    for (cuff = 0U; cuff < APP_PAD_CUFF_COUNT; cuff++)
    {
        if (DRV_VALVE_SetEnergised(PAD_GetValve(cuff), PAD_EnergiseLevel(false)))
        {
            PAD_context.valve_on_pump[cuff] = false;
        }
        else
        {
            all_ok = false;
        }
    }

    return all_ok;
}


/**
 * @brief Begin a run: group A on the pump line, group B venting.
 * @param now_ms Current time in milliseconds.
 */
static void PAD_StartRun(uint32_t now_ms)
{
    PAD_context.state         = APP_PAD_STATE_RUNNING;
    PAD_context.held_by_fault = false;
    PAD_context.phase         = APP_PAD_PHASE_INFLATE_1;
    PAD_context.step          = APP_PAD_STEP_PREFILL;
    PAD_context.step_start_ms = now_ms;
    PAD_context.is_topping_up = false;

    PAD_SetValve(APP_PAD_CUFF_1, true);
    PAD_SetValve(APP_PAD_CUFF_2, false);
}


/**
 * @brief End a run: pump off and both chambers to exhaust.
 */
static void PAD_StopRun(void)
{
    PAD_context.state = APP_PAD_STATE_IDLE;
    PAD_SetPumpSpeed(0U);
    PAD_SetValve(APP_PAD_CUFF_1, false);
    PAD_SetValve(APP_PAD_CUFF_2, false);
}


/**
 * @brief  Look for a pressure at or above the safety limit.
 * @param  source Receives the sensor that exceeded the limit.
 * @return true if the limit is exceeded.
 */
static bool PAD_FindOverLimit(LIB_FEM_Source_e *source)
{
    bool    found = false;
    uint8_t cuff  = 0U;

    for (cuff = 0U; cuff < APP_PAD_CUFF_COUNT; cuff++)
    {
        if (PAD_context.pressure_tenths[cuff] >= APP_PAD_LIMIT_TENTHS_MMHG)
        {
            found   = true;
            *source = PAD_GetSensorSource(cuff);
        }
    }

    return found;
}


/**
 * @brief Safety checks: a lost sensor (while running) and over-pressure (running or held).
 * @param now_ms Current time in milliseconds.
 */
static void PAD_CheckSafety(uint32_t now_ms)
{
    LIB_FEM_Source_e source = LIB_FEM_SRC_APP;

    if (PAD_context.state == APP_PAD_STATE_RUNNING)
    {
        if (!PAD_IsSensorFresh(APP_PAD_CUFF_1, now_ms))
        {
            PAD_EnterFault(LIB_FEM_EVT_SENSOR_NO_DATA, LIB_FEM_SRC_SENSOR1);
        }
        else if (!PAD_IsSensorFresh(APP_PAD_CUFF_2, now_ms))
        {
            PAD_EnterFault(LIB_FEM_EVT_SENSOR_NO_DATA, LIB_FEM_SRC_SENSOR2);
        }
        else if (PAD_FindOverLimit(&source))
        {
            PAD_EnterFault(LIB_FEM_EVT_PRESSURE_OVER_LIMIT, source);
        }
        else
        {
            /* All safe. */
        }
    }
    else if (PAD_context.state == APP_PAD_STATE_HELD)
    {
        if (PAD_FindOverLimit(&source))
        {
            PAD_EnterFault(LIB_FEM_EVT_PRESSURE_OVER_LIMIT, source);
        }
    }
    else
    {
        /* Idle or fault: nothing is pressurised by the pad. */
    }
}


/**
 * @brief Run the current step of the cycle (only while RUNNING).
 * @param now_ms Current time in milliseconds.
 */
static void PAD_RunStep(uint32_t now_ms)
{
    switch (PAD_context.step)
    {
        case APP_PAD_STEP_PREFILL:
            PAD_RunPrefill(now_ms);
            break;

        case APP_PAD_STEP_INFLATE:
            PAD_RunInflate(now_ms);
            break;

        case APP_PAD_STEP_HOLD:
            PAD_RunHold(now_ms);
            break;

        case APP_PAD_STEP_TRANSFER:
            PAD_RunTransfer(now_ms);
            break;

        default:
            PAD_EnterFault(LIB_FEM_EVT_INVALID_STATE, LIB_FEM_SRC_APP);
            break;
    }
}


/**
 * @brief FILL A: the first group (A) fills to full while group B vents.
 * @param now_ms Current time in milliseconds.
 */
static void PAD_RunPrefill(uint32_t now_ms)
{
    int32_t p_fill = PAD_context.pressure_tenths[APP_PAD_CUFF_1];

    if (p_fill >= APP_PAD_MAX_TENTHS_MMHG)
    {
        PAD_SetPumpSpeed(0U);
        PAD_context.step          = APP_PAD_STEP_HOLD;
        PAD_context.step_start_ms = now_ms;
        PAD_context.is_topping_up = false;
        PAD_PrintMessage("CYCLE: group A full, holding 5 min, group B vents\r\n");
    }
    else if ((now_ms - PAD_context.step_start_ms) > APP_PAD_FILL_TIMEOUT_MS)
    {
        PAD_EnterFault(LIB_FEM_EVT_PHASE_TIMEOUT, LIB_FEM_SRC_APP);
    }
    else
    {
        PAD_SetPumpSpeed(PAD_ComputeSpeed(p_fill));
    }
}


/**
 * @brief REFILL: the new group (active) fills to full. The old group just vents.
 * @param now_ms Current time in milliseconds.
 */
static void PAD_RunInflate(uint32_t now_ms)
{
    int32_t p_fill = PAD_context.pressure_tenths[PAD_GetActiveCuff()];

    if (p_fill >= APP_PAD_MAX_TENTHS_MMHG)
    {
        PAD_SetPumpSpeed(0U);
        PAD_context.step          = APP_PAD_STEP_HOLD;
        PAD_context.step_start_ms = now_ms;
        PAD_context.is_topping_up = false;
        PAD_PrintMessage("CYCLE: new group full, holding 5 min\r\n");
    }
    else if ((now_ms - PAD_context.step_start_ms) > APP_PAD_FILL_TIMEOUT_MS)
    {
        PAD_EnterFault(LIB_FEM_EVT_PUMP_ON_TIMEOUT, LIB_FEM_SRC_PUMP);
    }
    else
    {
        PAD_SetPumpSpeed(PAD_ComputeSpeed(p_fill));
    }
}


/**
 * @brief HOLD: keep the full group at full for 5 minutes. The other group keeps venting.
 * @param now_ms Current time in milliseconds.
 * @note  A full group that sags below APP_PAD_TOPUP_START_TENTHS_MMHG is topped up. When the
 *        time is over the other group is joined to the full one and the pump is stopped.
 */
static void PAD_RunHold(uint32_t now_ms)
{
    int32_t p_hold = PAD_context.pressure_tenths[PAD_GetActiveCuff()];

    if ((now_ms - PAD_context.step_start_ms) >= APP_PAD_HOLD_MS)
    {
        PAD_SetPumpSpeed(0U);
        PAD_SetValve(PAD_GetOtherCuff(), true);

        if (PAD_context.state == APP_PAD_STATE_RUNNING)
        {
            PAD_context.step          = APP_PAD_STEP_TRANSFER;
            PAD_context.step_start_ms = now_ms;
            PAD_PrintMessage("CYCLE: transfer, both groups joined\r\n");
        }
    }
    else
    {
        if (!PAD_context.is_topping_up && (p_hold < APP_PAD_TOPUP_START_TENTHS_MMHG))
        {
            PAD_context.is_topping_up = true;
            PAD_PrintMessage("TOP-UP: full group below 49 mmHg, pump on\r\n");
        }
        else if (PAD_context.is_topping_up && (p_hold >= APP_PAD_MAX_TENTHS_MMHG))
        {
            PAD_context.is_topping_up = false;
        }
        else
        {
            /* Keep the present top-up decision. */
        }

        PAD_SetPumpSpeed(PAD_context.is_topping_up ? PAD_ComputeSpeed(p_hold) : 0U);
    }
}


/**
 * @brief TRANSFER: both groups are joined and the pump feeds them until the new one is at 30 %.
 * @param now_ms Current time in milliseconds.
 * @note  When the new group reaches APP_PAD_TRANSFER_TARGET_TENTHS_MMHG the old group's valve
 *        is released (it vents quickly), the roles swap and the pump starts filling the new group.
 */
static void PAD_RunTransfer(uint32_t now_ms)
{
    uint8_t old_group = PAD_GetActiveCuff();
    uint8_t new_group = PAD_GetOtherCuff();
    int32_t p_new     = PAD_context.pressure_tenths[new_group];

    if (p_new >= APP_PAD_TRANSFER_TARGET_TENTHS_MMHG)
    {
        PAD_SetValve(old_group, false);

        if (PAD_context.state == APP_PAD_STATE_RUNNING)
        {
            PAD_context.phase = (PAD_context.phase == APP_PAD_PHASE_INFLATE_1) ?
                                APP_PAD_PHASE_INFLATE_2 : APP_PAD_PHASE_INFLATE_1;
            PAD_context.step          = APP_PAD_STEP_INFLATE;
            PAD_context.step_start_ms = now_ms;
            PAD_context.is_topping_up = false;
            PAD_PrintTransfer(p_new);
        }
    }
    else if ((now_ms - PAD_context.step_start_ms) > APP_PAD_TRANSFER_TIMEOUT_MS)
    {
        PAD_EnterFault(LIB_FEM_EVT_VENT_FAILED, PAD_GetSensorSource(new_group));
    }
    else
    {
        /* Still transferring: soft cuffs soak up the air, so the pump keeps feeding both groups. */
        PAD_SetPumpSpeed(PAD_ComputeSpeed(p_new));
    }
}

/**
 * @brief Stop the pump, then hold the pressure (or vent, for over-pressure and driver failures),
 *        set the LED blink code and report the event.
 * @param event_id Event to report.
 * @param source   Module that detected the problem.
 * @note  Writes to the drivers directly so a failing driver cannot cause recursion.
 */
static void PAD_EnterFault(LIB_FEM_EventId_e event_id, LIB_FEM_Source_e source)
{
    bool     pump_off_ok = DRV_PUMP_SetSpeed(PAD_context.config.pump, 0U);
    bool     must_vent   = (event_id == LIB_FEM_EVT_PRESSURE_OVER_LIMIT) ||
                           (event_id == LIB_FEM_EVT_PUMP_DRIVER_FAILED) ||
                           (event_id == LIB_FEM_EVT_VALVE_DRIVER_FAILED) ||
                           (event_id == LIB_FEM_EVT_INVALID_STATE);
    bool     vent_ok     = true;

    PAD_context.pump_speed_permille = 0U;
    PAD_context.is_topping_up       = false;
    PAD_context.config.gpio_set(PAD_context.config.led_pin_id, false);
    PAD_context.fault_code          = PAD_FaultCode(event_id);
    PAD_context.led_phase_start_ms  = PAD_context.config.get_ms();

    if (must_vent)
    {
        vent_ok = PAD_VentAll();
        /* Over-pressure: vented, then IDLE. A driver failure latches FAULT. */
        PAD_context.state = (event_id == LIB_FEM_EVT_PRESSURE_OVER_LIMIT) ?
                            APP_PAD_STATE_IDLE : APP_PAD_STATE_FAULT;
    }
    else
    {
        /* Every other fault: pressure is kept, nothing can rise with the pump off. */
        PAD_Hold(PAD_context.led_phase_start_ms, true);
    }

    (void)LIB_FEM_Report(event_id, LIB_FEM_SEV_FAULT, source, APP_PAD_NODE_ID, NULL, 0U);

    if (!pump_off_ok)
    {
        (void)LIB_FEM_Report(LIB_FEM_EVT_PUMP_DRIVER_FAILED, LIB_FEM_SEV_CRITICAL,
                             LIB_FEM_SRC_PUMP, APP_PAD_NODE_ID, NULL, 0U);
    }

    if (!vent_ok)
    {
        (void)LIB_FEM_Report(LIB_FEM_EVT_VALVE_DRIVER_FAILED, LIB_FEM_SEV_CRITICAL,
                             LIB_FEM_SRC_APP, APP_PAD_NODE_ID, NULL, 0U);
    }
}


/**
 * @brief Send a text buffer over the UART if it is complete.
 * @param text Buffer to send. Truncated text is dropped, not sent.
 * @note  A failed UART write is not reported over the same failed channel.
 */
static void PAD_Print(const LIB_TXT_Buffer_t *text)
{
    bool write_ok = false;

    if (!LIB_TXT_HasOverflowed(text))
    {
        write_ok = PAD_context.config.uart_write(text->storage, LIB_TXT_GetLength(text));
    }

    if (!write_ok)
    {
        /* Nothing more can be done: reporting would use the same UART. */
    }
}


/**
 * @brief Send a fixed message over the UART.
 * @param message Zero-terminated message.
 */
static void PAD_PrintMessage(const char *message)
{
    uint8_t          storage[APP_PAD_TEXT_BUFFER_SIZE_BYTES];
    LIB_TXT_Buffer_t text;

    if (LIB_TXT_Init(&text, storage, (uint16_t)APP_PAD_TEXT_BUFFER_SIZE_BYTES))
    {
        (void)LIB_TXT_AppendString(&text, message);
        PAD_Print(&text);
    }
}


/**
 * @brief Print the two zero points found at start-up.
 */
static void PAD_PrintBaselines(void)
{
    uint8_t          storage[APP_PAD_TEXT_BUFFER_SIZE_BYTES];
    LIB_TXT_Buffer_t text;

    if (LIB_TXT_Init(&text, storage, (uint16_t)APP_PAD_TEXT_BUFFER_SIZE_BYTES))
    {
        (void)LIB_TXT_AppendString(&text, "baseline1=");
        (void)LIB_TXT_AppendInt32(&text, PAD_context.baseline_counts[APP_PAD_CUFF_1]);
        (void)LIB_TXT_AppendString(&text, " baseline2=");
        (void)LIB_TXT_AppendInt32(&text, PAD_context.baseline_counts[APP_PAD_CUFF_2]);
        (void)LIB_TXT_AppendString(&text, "  (press button to START)\r\n");
        PAD_Print(&text);
    }
}


/**
 * @brief Print the transfer result.
 * @param new_tenths Pressure of the new group when the old group was released.
 */
static void PAD_PrintTransfer(int32_t new_tenths)
{
    uint8_t          storage[APP_PAD_TEXT_BUFFER_SIZE_BYTES];
    LIB_TXT_Buffer_t text;

    if (LIB_TXT_Init(&text, storage, (uint16_t)APP_PAD_TEXT_BUFFER_SIZE_BYTES))
    {
        (void)LIB_TXT_AppendString(&text, "SWAP new group at ");
        (void)LIB_TXT_AppendTenths(&text, new_tenths);
        (void)LIB_TXT_AppendString(&text, " mmHg, old group vents, pump fills new group\r\n");
        PAD_Print(&text);
    }
}


/**
 * @brief  One-letter code of the current step for the status line.
 * @return "P" fill A, "R" refill, "H" hold, "T" transfer, "-" when not running.
 */
static const char *PAD_StepLetter(void)
{
    const char *letter = "-";

    if (PAD_context.state == APP_PAD_STATE_RUNNING)
    {
        if (PAD_context.step == APP_PAD_STEP_PREFILL)
        {
            letter = "P";
        }
        else if (PAD_context.step == APP_PAD_STEP_INFLATE)
        {
            letter = "R";
        }
        else if (PAD_context.step == APP_PAD_STEP_TRANSFER)
        {
            letter = "T";
        }
        else
        {
            letter = "H";
        }
    }

    return letter;
}


/**
 * @brief  One-letter code of a valve position for the status line.
 * @param  cuff APP_PAD_CUFF_1 or APP_PAD_CUFF_2.
 * @return "P" on the pump line, "V" venting.
 */
static const char *PAD_ValveLetter(uint8_t cuff)
{
    return PAD_context.valve_on_pump[cuff] ? "P" : "V";
}


/**
 * @brief  Build one status line if its interval has passed.
 * @param  now_ms  Current time in milliseconds.
 * @param  text    Receives the text. Initialised here on top of storage.
 * @param  storage Caller-owned byte array of APP_PAD_TEXT_BUFFER_SIZE_BYTES.
 * @return true if a line was built and must be printed.
 */
static bool PAD_BuildStatus(uint32_t now_ms, LIB_TXT_Buffer_t *text, uint8_t *storage)
{
    bool is_ready = false;

    if (((now_ms - PAD_context.last_status_ms) >= APP_PAD_STATUS_PERIOD_MS) &&
        LIB_TXT_Init(text, storage, (uint16_t)APP_PAD_TEXT_BUFFER_SIZE_BYTES))
    {
        PAD_context.last_status_ms = now_ms;
        is_ready                   = true;

        (void)LIB_TXT_AppendString(text, "state=");
        (void)LIB_TXT_AppendInt32(text, (int32_t)PAD_context.state);
        (void)LIB_TXT_AppendString(text, " step=");
        (void)LIB_TXT_AppendString(text, PAD_StepLetter());
        (void)LIB_TXT_AppendString(text, " full=g");
        (void)LIB_TXT_AppendInt32(text, (int32_t)PAD_GetActiveCuff() + 1);
        (void)LIB_TXT_AppendString(text, " s1=");
        (void)LIB_TXT_AppendTenths(text, PAD_context.pressure_tenths[APP_PAD_CUFF_1]);
        (void)LIB_TXT_AppendString(text, " s2=");
        (void)LIB_TXT_AppendTenths(text, PAD_context.pressure_tenths[APP_PAD_CUFF_2]);
        (void)LIB_TXT_AppendString(text, " mmHg pump=");
        (void)LIB_TXT_AppendInt32(
            text, (int32_t)(PAD_context.pump_speed_permille / APP_PAD_PERMILLE_PER_PERCENT));
        (void)LIB_TXT_AppendString(text, "% knob=");
        (void)LIB_TXT_AppendInt32(
            text, (int32_t)(PAD_context.knob_speed_permille / APP_PAD_PERMILLE_PER_PERCENT));
        (void)LIB_TXT_AppendString(text, "% v=");
        (void)LIB_TXT_AppendString(text, PAD_ValveLetter(APP_PAD_CUFF_1));
        (void)LIB_TXT_AppendString(text, PAD_ValveLetter(APP_PAD_CUFF_2));
        (void)LIB_TXT_AppendString(text, "\r\n");
    }

    return is_ready;
}
