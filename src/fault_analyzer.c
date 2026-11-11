/**
 * @file fault_analyzer.c
 * @brief Implementation of rule-based fault identification, phase unbalance, and fault currents.
 */

#include "fault_analyzer.h"
#include "validation.h"
#include <stdio.h>
#include <string.h>
#include <math.h>

const char* fault_type_to_string(FaultType_t type)
{
    switch (type) {
        case FAULT_TYPE_NORMAL:              return "Normal Operation";
        case FAULT_TYPE_LINE_TO_GROUND:      return "Line-to-Ground (L-G)";
        case FAULT_TYPE_LINE_TO_LINE:        return "Line-to-Line (L-L)";
        case FAULT_TYPE_DOUBLE_LINE_GROUND:  return "Double Line-to-Ground (L-L-G)";
        case FAULT_TYPE_THREE_PHASE:         return "Three-Phase Symmetrical (3P)";
        case FAULT_TYPE_OVERLOAD_UNBALANCED: return "Overload / Phase Unbalance";
        default:                             return "Unknown State";
    }
}

const char* protection_action_to_string(ProtectionAction_t action)
{
    switch (action) {
        case ACTION_NO_ACTION:           return "NO ACTION (Normal)";
        case ACTION_MONITOR_ALARM:       return "MONITOR / ALARM (Warning)";
        case ACTION_TRIP_FEEDER_BREAKER: return "TRIP FEEDER BREAKER (ANSI 51)";
        case ACTION_INSTANTANEOUS_TRIP:  return "INSTANTANEOUS TRIP (ANSI 50)";
        case ACTION_EMERGENCY_LOCKOUT:   return "EMERGENCY BUS LOCKOUT (86)";
        default:                         return "UNKNOWN ACTION";
    }
}

double impedance_magnitude(ComplexImpedance_t z)
{
    return sqrt((z.resistance * z.resistance) + (z.reactance * z.reactance));
}

Status_t calculate_prospective_fault_current(double v_ln_kv,
                                             ComplexImpedance_t z_source,
                                             ComplexImpedance_t z_feeder,
                                             double *fault_current_out)
{
    if (fault_current_out == NULL) {
        return STATUS_ERR_NULL_PTR;
    }

    if (v_ln_kv <= 0.0) {
        return STATUS_ERR_INVALID_PARAM;
    }

    Status_t st = validate_impedance(&z_source);
    if (st != STATUS_SUCCESS) return st;

    st = validate_impedance(&z_feeder);
    if (st != STATUS_SUCCESS) return st;

    /* Total loop impedance Z_total = (Rs + Rf) + j(Xs + Xf) */
    ComplexImpedance_t z_total;
    z_total.resistance = z_source.resistance + z_feeder.resistance;
    z_total.reactance  = z_source.reactance  + z_feeder.reactance;

    double z_mag = impedance_magnitude(z_total);
    if (z_mag < 1e-4) {
        return STATUS_ERR_INVALID_PARAM;
    }

    /* I_f (Amperes) = (V_LN in Volts) / |Z_total| */
    double v_volts = v_ln_kv * 1000.0;
    *fault_current_out = v_volts / z_mag;

    return STATUS_SUCCESS;
}

Status_t classify_fault(const TelemetryRecord_t *record,
                        double nominal_current,
                        double nominal_voltage,
                        FaultType_t *fault_type_out,
                        char *faulted_phases_out,
                        double *unbalance_ratio_out)
{
    if (record == NULL || fault_type_out == NULL ||
        faulted_phases_out == NULL || unbalance_ratio_out == NULL) {
        return STATUS_ERR_NULL_PTR;
    }

    Status_t st = validate_telemetry_record(record);
    if (st != STATUS_SUCCESS) return st;

    st = validate_relay_settings(nominal_current, nominal_voltage);
    if (st != STATUS_SUCCESS) return st;

    double oc_limit = nominal_current * OVERCURRENT_PICKUP_FACTOR;
    double uv_limit = nominal_voltage * UNDERVOLTAGE_THRESHOLD_PU;
    double ground_limit = nominal_current * 0.20; /* 20% residual threshold */

    /* Flag individual phase anomalies */
    bool a_high = (record->current_a > oc_limit) || (record->voltage_a < uv_limit);
    bool b_high = (record->current_b > oc_limit) || (record->voltage_b < uv_limit);
    bool c_high = (record->current_c > oc_limit) || (record->voltage_c < uv_limit);
    bool n_high = (record->current_n > ground_limit);

    int count_affected = (a_high ? 1 : 0) + (b_high ? 1 : 0) + (c_high ? 1 : 0);

    /* Compute phase unbalance ratio = (max - min) / average */
    double min_curr = record->current_a;
    double max_curr = record->current_a;
    if (record->current_b < min_curr) min_curr = record->current_b;
    if (record->current_b > max_curr) max_curr = record->current_b;
    if (record->current_c < min_curr) min_curr = record->current_c;
    if (record->current_c > max_curr) max_curr = record->current_c;

    double avg_curr = (record->current_a + record->current_b + record->current_c) / 3.0;
    if (avg_curr > 1e-3) {
        *unbalance_ratio_out = (max_curr - min_curr) / avg_curr;
    } else {
        *unbalance_ratio_out = 0.0;
    }

    /* Decision Rule Matrix */
    if (count_affected == 3) {
        /* Symmetrical 3-Phase fault */
        *fault_type_out = FAULT_TYPE_THREE_PHASE;
        snprintf(faulted_phases_out, 16, "A-B-C%s", n_high ? "-G" : "");
    } else if (count_affected == 2) {
        if (n_high) {
            *fault_type_out = FAULT_TYPE_DOUBLE_LINE_GROUND;
            if (a_high && b_high) snprintf(faulted_phases_out, 16, "A-B-G");
            else if (b_high && c_high) snprintf(faulted_phases_out, 16, "B-C-G");
            else snprintf(faulted_phases_out, 16, "A-C-G");
        } else {
            *fault_type_out = FAULT_TYPE_LINE_TO_LINE;
            if (a_high && b_high) snprintf(faulted_phases_out, 16, "A-B");
            else if (b_high && c_high) snprintf(faulted_phases_out, 16, "B-C");
            else snprintf(faulted_phases_out, 16, "A-C");
        }
    } else if (count_affected == 1) {
        if (n_high) {
            *fault_type_out = FAULT_TYPE_LINE_TO_GROUND;
            if (a_high) snprintf(faulted_phases_out, 16, "A-G");
            else if (b_high) snprintf(faulted_phases_out, 16, "B-G");
            else snprintf(faulted_phases_out, 16, "C-G");
        } else {
            *fault_type_out = FAULT_TYPE_OVERLOAD_UNBALANCED;
            if (a_high) snprintf(faulted_phases_out, 16, "Phase A");
            else if (b_high) snprintf(faulted_phases_out, 16, "Phase B");
            else snprintf(faulted_phases_out, 16, "Phase C");
        }
    } else {
        /* Check if unbalance or neutral ground current warrants warning */
        if (n_high || *unbalance_ratio_out > 0.35) {
            *fault_type_out = FAULT_TYPE_OVERLOAD_UNBALANCED;
            snprintf(faulted_phases_out, 16, "Unbalanced");
        } else {
            *fault_type_out = FAULT_TYPE_NORMAL;
            snprintf(faulted_phases_out, 16, "None");
        }
    }

    return STATUS_SUCCESS;
}
