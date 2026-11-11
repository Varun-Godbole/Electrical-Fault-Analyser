/**
 * @file protection_relay.c
 * @brief Implementation of overcurrent/undervoltage detection and relay trip decision logic.
 */

#include "protection_relay.h"
#include "fault_analyzer.h"
#include "validation.h"
#include <math.h>

void get_default_relay_config(RelayConfig_t *cfg)
{
    if (cfg != NULL) {
        cfg->nominal_current     = DEFAULT_CT_PRIMARY_RATIO;   /* 600 A */
        cfg->nominal_voltage     = DEFAULT_NOMINAL_VOLTAGE_KV / sqrt(3.0); /* ~6.35 kV L-N */
        cfg->time_multiplier_tms = 0.15;
        cfg->instant_pickup_mult = INSTANTANEOUS_PICKUP_FACTOR; /* 4.0x -> 2400 A */
        cfg->undervoltage_pu     = UNDERVOLTAGE_THRESHOLD_PU;   /* 0.80 pu */
    }
}

double calculate_iec_trip_time(double current, double pickup_current, double tms)
{
    if (pickup_current <= 0.0 || tms <= 0.0) {
        return 0.0;
    }

    double ratio = current / pickup_current;
    if (ratio <= 1.0) {
        return 999.0; /* No trip below pickup */
    }

    /* IEC Standard Inverse: t = (0.14 * TMS) / (ratio^0.02 - 1) */
    double denominator = pow(ratio, 0.02) - 1.0;
    if (denominator <= 1e-6) {
        return 999.0;
    }

    double t = (0.14 * tms) / denominator;
    if (t < 0.05) {
        t = 0.05; /* Minimum mechanical breaker operating time (50 ms) */
    }
    return t;
}

Status_t evaluate_protection_response(const TelemetryRecord_t *record,
                                      const RelayConfig_t *cfg,
                                      ComplexImpedance_t z_source,
                                      ComplexImpedance_t z_feeder,
                                      FaultAssessment_t *assessment_out)
{
    if (record == NULL || cfg == NULL || assessment_out == NULL) {
        return STATUS_ERR_NULL_PTR;
    }

    Status_t st = validate_telemetry_record(record);
    if (st != STATUS_SUCCESS) return st;

    st = validate_relay_settings(cfg->nominal_current, cfg->nominal_voltage);
    if (st != STATUS_SUCCESS) return st;

    assessment_out->telemetry = *record;

    /* 1. Identify maximum phase current and minimum phase voltage */
    double max_i = record->current_a;
    if (record->current_b > max_i) max_i = record->current_b;
    if (record->current_c > max_i) max_i = record->current_c;
    assessment_out->max_phase_current = max_i;

    double min_v = record->voltage_a;
    if (record->voltage_b < min_v) min_v = record->voltage_b;
    if (record->voltage_c < min_v) min_v = record->voltage_c;
    assessment_out->min_phase_voltage = min_v;

    /* 2. Check Overcurrent & Undervoltage Conditions */
    double pickup_current = cfg->nominal_current * OVERCURRENT_PICKUP_FACTOR;
    double instant_limit  = cfg->nominal_current * cfg->instant_pickup_mult;
    double uv_voltage     = cfg->nominal_voltage * cfg->undervoltage_pu;

    assessment_out->is_overcurrent  = (max_i > pickup_current) || (record->current_n > (cfg->nominal_current * 0.25));
    assessment_out->is_undervoltage = (min_v < uv_voltage);

    /* 3. Rule-based fault classification */
    st = classify_fault(record, cfg->nominal_current, cfg->nominal_voltage,
                        &assessment_out->identified_fault,
                        assessment_out->faulted_phases,
                        &assessment_out->current_unbalance_ratio);
    if (st != STATUS_SUCCESS) return st;

    /* 4. Calculate theoretical prospective fault current */
    st = calculate_prospective_fault_current(cfg->nominal_voltage, z_source, z_feeder,
                                            &assessment_out->calculated_fault_current);
    if (st != STATUS_SUCCESS) {
        assessment_out->calculated_fault_current = 0.0;
    }

    /* 5. Recommended Protection Action Decision Logic */
    if (assessment_out->identified_fault == FAULT_TYPE_NORMAL) {
        assessment_out->recommended_action = ACTION_NO_ACTION;
        assessment_out->trip_time_sec = 0.0;
    } else if (assessment_out->identified_fault == FAULT_TYPE_OVERLOAD_UNBALANCED) {
        assessment_out->recommended_action = ACTION_MONITOR_ALARM;
        assessment_out->trip_time_sec = 0.0;
    } else if (max_i >= instant_limit) {
        /* High fault current demands ANSI 50 Instantaneous Trip */
        if (assessment_out->identified_fault == FAULT_TYPE_THREE_PHASE) {
            assessment_out->recommended_action = ACTION_EMERGENCY_LOCKOUT;
        } else {
            assessment_out->recommended_action = ACTION_INSTANTANEOUS_TRIP;
        }
        assessment_out->trip_time_sec = 0.050; /* 50 ms instantaneous */
    } else {
        /* Moderate fault demands ANSI 51 Time-Overcurrent Breaker Trip */
        assessment_out->recommended_action = ACTION_TRIP_FEEDER_BREAKER;
        assessment_out->trip_time_sec = calculate_iec_trip_time(max_i, pickup_current, cfg->time_multiplier_tms);
    }

    return STATUS_SUCCESS;
}
