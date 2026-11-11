/**
 * @file fault_manager.c
 * @brief Implementation of in-memory fault history repository and query functions.
 */

#include "fault_manager.h"
#include <string.h>

static FaultAssessment_t s_records[MAX_RECORDS];
static size_t s_record_count = 0;

void fault_manager_init(void)
{
    memset(s_records, 0, sizeof(s_records));
    s_record_count = 0;
}

size_t fault_manager_get_count(void)
{
    return s_record_count;
}

Status_t fault_manager_add(const FaultAssessment_t *assessment)
{
    if (assessment == NULL) {
        return STATUS_ERR_NULL_PTR;
    }

    if (s_record_count >= MAX_RECORDS) {
        return STATUS_ERR_CAPACITY_FULL;
    }

    /* Prevent duplicate record IDs */
    for (size_t i = 0; i < s_record_count; ++i) {
        if (strncmp(s_records[i].telemetry.record_id,
                    assessment->telemetry.record_id,
                    MAX_ID_LEN) == 0) {
            /* Update existing entry */
            s_records[i] = *assessment;
            return STATUS_SUCCESS;
        }
    }

    s_records[s_record_count++] = *assessment;
    return STATUS_SUCCESS;
}

Status_t fault_manager_find_by_id(const char *record_id, FaultAssessment_t *assessment_out)
{
    if (record_id == NULL || assessment_out == NULL) {
        return STATUS_ERR_NULL_PTR;
    }

    for (size_t i = 0; i < s_record_count; ++i) {
        if (strncmp(s_records[i].telemetry.record_id, record_id, MAX_ID_LEN) == 0) {
            *assessment_out = s_records[i];
            return STATUS_SUCCESS;
        }
    }

    return STATUS_ERR_RECORD_NOT_FOUND;
}

Status_t fault_manager_filter_by_type(FaultType_t type,
                                      FaultAssessment_t *results_out,
                                      size_t max_results,
                                      size_t *count_out)
{
    if (results_out == NULL || count_out == NULL) {
        return STATUS_ERR_NULL_PTR;
    }

    size_t matches = 0;
    for (size_t i = 0; i < s_record_count; ++i) {
        if (s_records[i].identified_fault == type) {
            if (matches < max_results) {
                results_out[matches] = s_records[i];
            }
            matches++;
        }
    }

    *count_out = (matches > max_results) ? max_results : matches;
    return STATUS_SUCCESS;
}

Status_t fault_manager_calculate_statistics(SystemStatistics_t *stats_out)
{
    if (stats_out == NULL) {
        return STATUS_ERR_NULL_PTR;
    }

    memset(stats_out, 0, sizeof(SystemStatistics_t));
    stats_out->total_records_evaluated = s_record_count;

    for (size_t i = 0; i < s_record_count; ++i) {
        const FaultAssessment_t *rec = &s_records[i];

        switch (rec->identified_fault) {
            case FAULT_TYPE_NORMAL:              stats_out->normal_count++; break;
            case FAULT_TYPE_LINE_TO_GROUND:      stats_out->lg_fault_count++; break;
            case FAULT_TYPE_LINE_TO_LINE:        stats_out->ll_fault_count++; break;
            case FAULT_TYPE_DOUBLE_LINE_GROUND:  stats_out->llg_fault_count++; break;
            case FAULT_TYPE_THREE_PHASE:         stats_out->three_phase_fault_count++; break;
            case FAULT_TYPE_OVERLOAD_UNBALANCED: stats_out->overload_count++; break;
            default: break;
        }

        if (rec->recommended_action == ACTION_TRIP_FEEDER_BREAKER ||
            rec->recommended_action == ACTION_INSTANTANEOUS_TRIP ||
            rec->recommended_action == ACTION_EMERGENCY_LOCKOUT) {
            stats_out->breaker_trip_count++;
        }

        if (rec->max_phase_current > stats_out->max_recorded_fault_current) {
            stats_out->max_recorded_fault_current = rec->max_phase_current;
        }
    }

    return STATUS_SUCCESS;
}

const FaultAssessment_t* fault_manager_get_at(size_t index)
{
    if (index >= s_record_count) {
        return NULL;
    }
    return &s_records[index];
}
