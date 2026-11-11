/**
 * @file fault_analyzer.h
 * @brief Predefined rule-based electrical fault identification and short-circuit calculations.
 */

#ifndef FAULT_ANALYZER_H
#define FAULT_ANALYZER_H

#include "common_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Returns string description of fault type enumeration.
 * @param type FaultType_t value.
 * @return String literal representation.
 */
const char* fault_type_to_string(FaultType_t type);

/**
 * @brief Returns string description of protection action enumeration.
 * @param action ProtectionAction_t value.
 * @return String literal representation.
 */
const char* protection_action_to_string(ProtectionAction_t action);

/**
 * @brief Computes magnitude of complex impedance |R + jX|.
 * @param z Complex impedance.
 * @return Magnitude in Ohms.
 */
double impedance_magnitude(ComplexImpedance_t z);

/**
 * @brief Calculates theoretical prospective fault current for a 3-phase symmetrical fault.
 * Formula: I_f = V_LN / |Z_source + Z_feeder|
 * @param v_ln_kv Line-to-neutral voltage in kV.
 * @param z_source Source Thevenin impedance (Ohms).
 * @param z_feeder Feeder cable/line impedance (Ohms).
 * @param fault_current_out Output in Amperes RMS.
 * @return STATUS_SUCCESS on success, error code otherwise.
 */
Status_t calculate_prospective_fault_current(double v_ln_kv,
                                             ComplexImpedance_t z_source,
                                             ComplexImpedance_t z_feeder,
                                             double *fault_current_out);

/**
 * @brief Analyzes telemetry record using rule-based criteria to classify fault type.
 * @param record Input telemetry measurements.
 * @param nominal_current Primary rated current (Amperes).
 * @param nominal_voltage Primary rated line-to-neutral voltage (kV).
 * @param fault_type_out Output detected fault type.
 * @param faulted_phases_out Buffer of at least 16 bytes for text description (e.g. "A-G").
 * @param unbalance_ratio_out Ratio of max to min phase current.
 * @return STATUS_SUCCESS on success, error code otherwise.
 */
Status_t classify_fault(const TelemetryRecord_t *record,
                        double nominal_current,
                        double nominal_voltage,
                        FaultType_t *fault_type_out,
                        char *faulted_phases_out,
                        double *unbalance_ratio_out);

#ifdef __cplusplus
}
#endif

#endif /* FAULT_ANALYZER_H */
