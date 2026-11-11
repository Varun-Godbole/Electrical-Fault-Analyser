/**
 * @file validation.h
 * @brief Input validation functions for electrical telemetry and protection parameters.
 */

#ifndef VALIDATION_H
#define VALIDATION_H

#include "common_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Validates an incoming telemetry record for physical sanity.
 * @param record Pointer to TelemetryRecord_t.
 * @return STATUS_SUCCESS if valid, error code otherwise.
 */
Status_t validate_telemetry_record(const TelemetryRecord_t *record);

/**
 * @brief Validates electrical impedance parameters.
 * @param z Complex impedance (R + jX).
 * @return STATUS_SUCCESS if valid, error code otherwise.
 */
Status_t validate_impedance(const ComplexImpedance_t *z);

/**
 * @brief Validates protection relay settings.
 * @param nominal_current Rated primary current in Amperes (> 0).
 * @param nominal_voltage Rated line-to-neutral voltage in kV (> 0).
 * @return STATUS_SUCCESS if valid, error code otherwise.
 */
Status_t validate_relay_settings(double nominal_current, double nominal_voltage);

#ifdef __cplusplus
}
#endif

#endif /* VALIDATION_H */
