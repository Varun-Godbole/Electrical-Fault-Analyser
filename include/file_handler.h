/**
 * @file file_handler.h
 * @brief File handling operations for CSV telemetry loading, history export, and report generation.
 */

#ifndef FILE_HANDLER_H
#define FILE_HANDLER_H

#include "common_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Parses and loads raw electrical telemetry records from a CSV file.
 * @param filepath Path to source CSV file.
 * @param records_out Output buffer for TelemetryRecord_t items.
 * @param max_records Capacity of records_out buffer.
 * @param count_out Pointer to store actual count of successfully parsed records.
 * @return STATUS_SUCCESS on success, error code otherwise.
 */
Status_t load_telemetry_csv(const char *filepath,
                            TelemetryRecord_t *records_out,
                            size_t max_records,
                            size_t *count_out);

/**
 * @brief Saves evaluated fault history records from fault_manager into a CSV file.
 * @param filepath Destination CSV path.
 * @return STATUS_SUCCESS on success, error code otherwise.
 */
Status_t save_fault_history_csv(const char *filepath);

/**
 * @brief Generates an engineering diagnostic and protection analysis report in text format.
 * @param filepath Destination text report path.
 * @param z_source Source impedance parameters.
 * @param z_feeder Feeder line impedance parameters.
 * @return STATUS_SUCCESS on success, error code otherwise.
 */
Status_t generate_diagnostic_report(const char *filepath,
                                    ComplexImpedance_t z_source,
                                    ComplexImpedance_t z_feeder);

#ifdef __cplusplus
}
#endif

#endif /* FILE_HANDLER_H */
