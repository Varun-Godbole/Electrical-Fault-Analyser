/**
 * @file test_validation.cpp
 * @brief Unit tests for electrical record validation and impedance parameter bounds.
 */

#include "gtest/gtest.h"
#include "validation.h"
#include <cstring>

TEST(ValidationTest, PositiveTelemetryRecord) {
    TelemetryRecord_t rec;
    strncpy(rec.record_id, "FLT-100", sizeof(rec.record_id));
    strncpy(rec.feeder_id, "FEEDER-01", sizeof(rec.feeder_id));
    strncpy(rec.timestamp, "2026-11-11 12:00:00", sizeof(rec.timestamp));
    rec.current_a = 450.0;
    rec.current_b = 455.0;
    rec.current_c = 448.0;
    rec.current_n = 5.0;
    rec.voltage_a = 6.35;
    rec.voltage_b = 6.34;
    rec.voltage_c = 6.36;

    EXPECT_EQ(validate_telemetry_record(&rec), STATUS_SUCCESS);
}

TEST(ValidationTest, NegativeNullArguments) {
    EXPECT_EQ(validate_telemetry_record(nullptr), STATUS_ERR_NULL_PTR);
    EXPECT_EQ(validate_impedance(nullptr), STATUS_ERR_NULL_PTR);
}

TEST(ValidationTest, BoundaryElectricalCurrentRanges) {
    TelemetryRecord_t rec;
    strncpy(rec.record_id, "FLT-001", sizeof(rec.record_id));
    strncpy(rec.feeder_id, "FDR-01", sizeof(rec.feeder_id));
    rec.current_b = 100.0;
    rec.current_c = 100.0;
    rec.current_n = 0.0;
    rec.voltage_a = 6.3;
    rec.voltage_b = 6.3;
    rec.voltage_c = 6.3;

    /* Lower boundary (0.0 A is valid) */
    rec.current_a = 0.0;
    EXPECT_EQ(validate_telemetry_record(&rec), STATUS_SUCCESS);

    /* Negative current is invalid */
    rec.current_a = -5.0;
    EXPECT_EQ(validate_telemetry_record(&rec), STATUS_ERR_OUT_OF_RANGE);

    /* Beyond maximum realistic short circuit limit (> 100 kA) */
    rec.current_a = 150000.0;
    EXPECT_EQ(validate_telemetry_record(&rec), STATUS_ERR_OUT_OF_RANGE);
}

TEST(ValidationTest, BoundaryImpedanceSingularity) {
    ComplexImpedance_t z;
    /* Exactly zero impedance causes short-circuit division by zero */
    z.resistance = 0.0;
    z.reactance  = 0.0;
    EXPECT_EQ(validate_impedance(&z), STATUS_ERR_INVALID_PARAM);

    /* Negative resistance is physically passive invalid */
    z.resistance = -0.5;
    z.reactance  = 1.0;
    EXPECT_EQ(validate_impedance(&z), STATUS_ERR_OUT_OF_RANGE);

    /* Valid impedance */
    z.resistance = 0.5;
    z.reactance  = 1.5;
    EXPECT_EQ(validate_impedance(&z), STATUS_SUCCESS);
}
