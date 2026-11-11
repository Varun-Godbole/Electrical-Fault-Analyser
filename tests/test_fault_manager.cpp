/**
 * @file test_fault_manager.cpp
 * @brief Unit tests for in-memory fault history repository, search, and filtering.
 */

#include "gtest/gtest.h"
#include "fault_manager.h"
#include <cstring>

TEST(FaultManagerTest, AddAndFindById) {
    fault_manager_init();
    EXPECT_EQ(fault_manager_get_count(), 0);

    FaultAssessment_t fa1;
    memset(&fa1, 0, sizeof(fa1));
    strncpy(fa1.telemetry.record_id, "REC-001", sizeof(fa1.telemetry.record_id));
    fa1.identified_fault = FAULT_TYPE_LINE_TO_GROUND;
    fa1.max_phase_current = 1500.0;

    EXPECT_EQ(fault_manager_add(&fa1), STATUS_SUCCESS);
    EXPECT_EQ(fault_manager_get_count(), 1);

    FaultAssessment_t found;
    EXPECT_EQ(fault_manager_find_by_id("REC-001", &found), STATUS_SUCCESS);
    EXPECT_EQ(found.identified_fault, FAULT_TYPE_LINE_TO_GROUND);
    EXPECT_NEAR(found.max_phase_current, 1500.0, 1e-3);

    /* Negative search */
    EXPECT_EQ(fault_manager_find_by_id("NON_EXISTENT", &found), STATUS_ERR_RECORD_NOT_FOUND);
}

TEST(FaultManagerTest, FilterByTypeAndStatistics) {
    fault_manager_init();

    FaultAssessment_t r1, r2, r3;
    memset(&r1, 0, sizeof(r1));
    memset(&r2, 0, sizeof(r2));
    memset(&r3, 0, sizeof(r3));

    strncpy(r1.telemetry.record_id, "R1", sizeof(r1.telemetry.record_id));
    r1.identified_fault = FAULT_TYPE_LINE_TO_LINE;
    r1.recommended_action = ACTION_TRIP_FEEDER_BREAKER;
    r1.max_phase_current = 2200.0;

    strncpy(r2.telemetry.record_id, "R2", sizeof(r2.telemetry.record_id));
    r2.identified_fault = FAULT_TYPE_LINE_TO_LINE;
    r2.recommended_action = ACTION_TRIP_FEEDER_BREAKER;
    r2.max_phase_current = 2800.0;

    strncpy(r3.telemetry.record_id, "R3", sizeof(r3.telemetry.record_id));
    r3.identified_fault = FAULT_TYPE_NORMAL;
    r3.recommended_action = ACTION_NO_ACTION;
    r3.max_phase_current = 400.0;

    fault_manager_add(&r1);
    fault_manager_add(&r2);
    fault_manager_add(&r3);

    /* Filter for Line-to-Line faults */
    FaultAssessment_t ll_list[5];
    size_t ll_count = 0;
    EXPECT_EQ(fault_manager_filter_by_type(FAULT_TYPE_LINE_TO_LINE, ll_list, 5, &ll_count), STATUS_SUCCESS);
    EXPECT_EQ(ll_count, 2);

    /* Verify Statistics Calculation */
    SystemStatistics_t stats;
    EXPECT_EQ(fault_manager_calculate_statistics(&stats), STATUS_SUCCESS);
    EXPECT_EQ(stats.total_records_evaluated, 3);
    EXPECT_EQ(stats.ll_fault_count, 2);
    EXPECT_EQ(stats.normal_count, 1);
    EXPECT_EQ(stats.breaker_trip_count, 2);
    EXPECT_NEAR(stats.max_recorded_fault_current, 2800.0, 1e-3);
}
