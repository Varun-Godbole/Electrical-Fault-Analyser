/**
 * @file protection_relay.h
 * @brief Overcurrent (ANSI 50/51) and Undervoltage (ANSI 27) protection decision engine.
 */

#ifndef PROTECTION_RELAY_H
#define PROTECTION_RELAY_H

#include "common_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Configuration settings for protective relay thresholds.
 */
typedef struct {
    double nominal_current;      /* Primary rated current (A) */
    double nominal_voltage;      /* Line-to-neutral nominal voltage (kV) */
    double time_multiplier_tms;  /* IEC curve Time Multiplier Setting (TMS) */
    double instant_pickup_mult;  /* Instantaneous trip multiplier (default: 4.0x) */
    double undervoltage_pu;      /* Undervoltage threshold in per-unit (default: 0.80) */
} RelayConfig_t;

/**
 * @brief Returns standard default relay configuration settings.
 * @param cfg Pointer to RelayConfig_t to populate.
 */
void get_default_relay_config(RelayConfig_t *cfg);

/**
 * @brief Calculates IEC Standard Inverse time-overcurrent operating trip time (ANSI 51).
 * Formula: t = (0.14 * TMS) / ((I / Is)^0.02 - 1)
 * @param current Measured current (A).
 * @param pickup_current Overcurrent pickup threshold Is (A).
 * @param tms Time Multiplier Setting.
 * @return Operating trip time in seconds.
 */
double calculate_iec_trip_time(double current, double pickup_current, double tms);

/**
 * @brief Performs complete protection assessment and determines recommended relay response.
 * @param record Measured telemetry snapshot.
 * @param cfg Relay threshold configuration.
 * @param z_source Source impedance.
 * @param z_feeder Feeder line impedance.
 * @param assessment_out Output populated assessment report.
 * @return STATUS_SUCCESS on success, error code otherwise.
 */
Status_t evaluate_protection_response(const TelemetryRecord_t *record,
                                      const RelayConfig_t *cfg,
                                      ComplexImpedance_t z_source,
                                      ComplexImpedance_t z_feeder,
                                      FaultAssessment_t *assessment_out);

#ifdef __cplusplus
}
#endif

#endif /* PROTECTION_RELAY_H */
