/**
 * @file fault_manager.h
 * @brief In-memory fault record repository, searching, filtering, and statistical analysis.
 */

#ifndef FAULT_MANAGER_H
#define FAULT_MANAGER_H

#include "common_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initializes the fault repository and clears internal storage.
 */
void fault_manager_init(void);

/**
 * @brief Returns total number of fault records currently stored.
 * @return Count of records (0 to MAX_RECORDS).
 */
size_t fault_manager_get_count(void);

/**
 * @brief Adds a new evaluated fault assessment to the history log.
 * @param assessment Pointer to FaultAssessment_t to append.
 * @return STATUS_SUCCESS on success, STATUS_ERR_CAPACITY_FULL if full.
 */
Status_t fault_manager_add(const FaultAssessment_t *assessment);

/**
 * @brief Retrieves a fault assessment record by its unique Record ID.
 * @param record_id Target ID string.
 * @param assessment_out Output buffer to store found record.
 * @return STATUS_SUCCESS if found, STATUS_ERR_RECORD_NOT_FOUND otherwise.
 */
Status_t fault_manager_find_by_id(const char *record_id, FaultAssessment_t *assessment_out);

/**
 * @brief Filters records by fault type classification.
 * @param type Target FaultType_t.
 * @param results_out Array buffer to store matched records.
 * @param max_results Maximum capacity of results buffer.
 * @param count_out Pointer to store number of matched records.
 * @return STATUS_SUCCESS on success, error code otherwise.
 */
Status_t fault_manager_filter_by_type(FaultType_t type,
                                      FaultAssessment_t *results_out,
                                      size_t max_results,
                                      size_t *count_out);

/**
 * @brief Calculates aggregate statistics across all logged fault records.
 * @param stats_out Output pointer to SystemStatistics_t.
 * @return STATUS_SUCCESS on success, error code otherwise.
 */
Status_t fault_manager_calculate_statistics(SystemStatistics_t *stats_out);

/**
 * @brief Returns direct pointer to read-only record at specified index.
 * @param index Array index (0 to count-1).
 * @return Const pointer to record or NULL if out of bounds.
 */
const FaultAssessment_t* fault_manager_get_at(size_t index);

#ifdef __cplusplus
}
#endif

#endif /* FAULT_MANAGER_H */
