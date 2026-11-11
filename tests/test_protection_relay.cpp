/**
 * @file test_protection_relay.cpp
 * @brief Unit tests for protection relay trip decisions, IEC time curves, and emergency lockouts.
 */

#include "gtest/gtest.h"
#include "protection_relay.h"
#include <cstring>

TEST(ProtectionRelayTest, NormalConditionNoTrip) {
    RelayConfig_t cfg;
    get_default_relay_config(&cfg);

    TelemetryRecord_t rec;
    strncpy(rec.record_id, "NORM-01", sizeof(rec.record_id));
    strncpy(rec.feeder_id, "FDR-01", sizeof(rec.feeder_id));
    /* Rated load: 400 A per phase (below 750 A pickup), nominal voltage ~6.35 kV */
    rec.current_a = 400.0;
    rec.current_b = 395.0;
    rec.current_c = 405.0;
    rec.current_n = 5.0;
    rec.voltage_a = 6.35;
    rec.voltage_b = 6.34;
    rec.voltage_c = 6.36;

    ComplexImpedance_t zs = {0.2, 1.0};
    ComplexImpedance_t zf = {0.3, 0.7};

    FaultAssessment_t assessment;
    Status_t st = evaluate_protection_response(&rec, &cfg, zs, zf, &assessment);
    EXPECT_EQ(st, STATUS_SUCCESS);
    EXPECT_EQ(assessment.identified_fault, FAULT_TYPE_NORMAL);
    EXPECT_EQ(assessment.recommended_action, ACTION_NO_ACTION);
    EXPECT_FALSE(assessment.is_overcurrent);
    EXPECT_FALSE(assessment.is_undervoltage);
}

TEST(ProtectionRelayTest, SevereFaultInstantaneousTrip) {
    RelayConfig_t cfg;
    get_default_relay_config(&cfg);

    TelemetryRecord_t rec;
    strncpy(rec.record_id, "INST-01", sizeof(rec.record_id));
    strncpy(rec.feeder_id, "FDR-01", sizeof(rec.feeder_id));
    /* Current = 3200 A (> 4.0 * 600A = 2400 A instantaneous threshold) */
    rec.current_a = 3200.0;
    rec.current_b = 400.0;
    rec.current_c = 400.0;
    rec.current_n = 2800.0;
    rec.voltage_a = 0.8;
    rec.voltage_b = 6.3;
    rec.voltage_c = 6.3;

    ComplexImpedance_t zs = {0.2, 1.0};
    ComplexImpedance_t zf = {0.3, 0.7};

    FaultAssessment_t assessment;
    Status_t st = evaluate_protection_response(&rec, &cfg, zs, zf, &assessment);
    EXPECT_EQ(st, STATUS_SUCCESS);
    EXPECT_TRUE(assessment.is_overcurrent);
    EXPECT_TRUE(assessment.is_undervoltage);
    EXPECT_EQ(assessment.recommended_action, ACTION_INSTANTANEOUS_TRIP);
    EXPECT_NEAR(assessment.trip_time_sec, 0.050, 1e-4); /* 50 ms instantaneous */
}

TEST(ProtectionRelayTest, BoundaryIecTripTimeCurves) {
    /* Below pickup (400 A vs 750 A pickup) -> no trip (999.0s) */
    double t_below = calculate_iec_trip_time(400.0, 750.0, 0.15);
    EXPECT_NEAR(t_below, 999.0, 0.1);

    /* Moderate overcurrent (1500 A vs 750 A pickup -> 2.0x pickup) */
    double t_mod = calculate_iec_trip_time(1500.0, 750.0, 0.15);
    EXPECT_TRUE(t_mod > 0.05 && t_mod < 2.0);

    /* Severe overcurrent (6000 A vs 750 A pickup -> 8.0x pickup) -> Faster trip */
    double t_high = calculate_iec_trip_time(6000.0, 750.0, 0.15);
    EXPECT_TRUE(t_high < t_mod);
}
