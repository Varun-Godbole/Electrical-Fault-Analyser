/**
 * @file validation.c
 * @brief Implementation of defensive validation for electrical telemetry and settings.
 */

#include "validation.h"
#include <string.h>
#include <math.h>

Status_t validate_telemetry_record(const TelemetryRecord_t *record)
{
    if (record == NULL) {
        return STATUS_ERR_NULL_PTR;
    }

    if (strlen(record->record_id) == 0 || strlen(record->record_id) >= MAX_ID_LEN) {
        return STATUS_ERR_INVALID_PARAM;
    }

    if (strlen(record->feeder_id) == 0 || strlen(record->feeder_id) >= MAX_ID_LEN) {
        return STATUS_ERR_INVALID_PARAM;
    }

    /* Check for NaN or Inf in electrical values */
    if (isnan(record->current_a) || isnan(record->current_b) ||
        isnan(record->current_c) || isnan(record->current_n) ||
        isnan(record->voltage_a) || isnan(record->voltage_b) || isnan(record->voltage_c)) {
        return STATUS_ERR_INVALID_PARAM;
    }

    /* Electrical current sanity: 0.0 A to 100,000.0 A (100 kA maximum fault) */
    if (record->current_a < 0.0 || record->current_a > 100000.0 ||
        record->current_b < 0.0 || record->current_b > 100000.0 ||
        record->current_c < 0.0 || record->current_c > 100000.0 ||
        record->current_n < 0.0 || record->current_n > 100000.0) {
        return STATUS_ERR_OUT_OF_RANGE;
    }

    /* Voltage sanity: 0.0 kV to 1,000.0 kV */
    if (record->voltage_a < 0.0 || record->voltage_a > 1000.0 ||
        record->voltage_b < 0.0 || record->voltage_b > 1000.0 ||
        record->voltage_c < 0.0 || record->voltage_c > 1000.0) {
        return STATUS_ERR_OUT_OF_RANGE;
    }

    return STATUS_SUCCESS;
}

Status_t validate_impedance(const ComplexImpedance_t *z)
{
    if (z == NULL) {
        return STATUS_ERR_NULL_PTR;
    }

    if (isnan(z->resistance) || isnan(z->reactance)) {
        return STATUS_ERR_INVALID_PARAM;
    }

    /* Resistance must be non-negative, total magnitude must be positive */
    if (z->resistance < 0.0) {
        return STATUS_ERR_OUT_OF_RANGE;
    }

    double mag_sq = (z->resistance * z->resistance) + (z->reactance * z->reactance);
    if (mag_sq < 1e-6) {
        return STATUS_ERR_INVALID_PARAM; /* Zero impedance short-circuit singularity */
    }

    return STATUS_SUCCESS;
}

Status_t validate_relay_settings(double nominal_current, double nominal_voltage)
{
    if (nominal_current <= 0.0 || nominal_current > 10000.0) {
        return STATUS_ERR_OUT_OF_RANGE;
    }

    if (nominal_voltage <= 0.0 || nominal_voltage > 1000.0) {
        return STATUS_ERR_OUT_OF_RANGE;
    }

    return STATUS_SUCCESS;
}
