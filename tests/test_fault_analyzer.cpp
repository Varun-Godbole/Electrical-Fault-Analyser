/**
 * @file test_fault_analyzer.cpp
 * @brief Unit tests for rule-based fault classification and short-circuit calculations.
 */

#include "gtest/gtest.h"
#include "fault_analyzer.h"
#include <cstring>

TEST(FaultAnalyzerTest, PositiveThreePhaseFault) {
    TelemetryRecord_t rec;
    strncpy(rec.record_id, "FLT-3P", sizeof(rec.record_id));
    strncpy(rec.feeder_id, "FDR-01", sizeof(rec.feeder_id));
    /* High symmetric currents > 125% of 600A (750A), symmetric voltage drop */
    rec.current_a = 3500.0;
    rec.current_b = 3480.0;
    rec.current_c = 3520.0;
    rec.current_n = 15.0;
    rec.voltage_a = 1.20;
    rec.voltage_b = 1.25;
    rec.voltage_c = 1.18;

    FaultType_t type;
    char phases[16];
    double unbalance;

    Status_t st = classify_fault(&rec, 600.0, 6.35, &type, phases, &unbalance);
    EXPECT_EQ(st, STATUS_SUCCESS);
    EXPECT_EQ(type, FAULT_TYPE_THREE_PHASE);
    EXPECT_TRUE(strstr(phases, "A-B-C") != nullptr);
}

TEST(FaultAnalyzerTest, PositiveSingleLineToGroundFault) {
    TelemetryRecord_t rec;
    strncpy(rec.record_id, "FLT-LG", sizeof(rec.record_id));
    strncpy(rec.feeder_id, "FDR-01", sizeof(rec.feeder_id));
    /* Phase A elevated, Phases B and C normal, high neutral earth current */
    rec.current_a = 2100.0;
    rec.current_b = 400.0;
    rec.current_c = 410.0;
    rec.current_n = 1800.0;
    rec.voltage_a = 1.50;  /* Phase A voltage sag */
    rec.voltage_b = 6.30;
    rec.voltage_c = 6.35;

    FaultType_t type;
    char phases[16];
    double unbalance;

    Status_t st = classify_fault(&rec, 600.0, 6.35, &type, phases, &unbalance);
    EXPECT_EQ(st, STATUS_SUCCESS);
    EXPECT_EQ(type, FAULT_TYPE_LINE_TO_GROUND);
    EXPECT_TRUE(strstr(phases, "A-G") != nullptr);
}

TEST(FaultAnalyzerTest, PositiveLineToLineFault) {
    TelemetryRecord_t rec;
    strncpy(rec.record_id, "FLT-LL", sizeof(rec.record_id));
    strncpy(rec.feeder_id, "FDR-01", sizeof(rec.feeder_id));
    /* Phases B and C high, Phase A normal, low neutral */
    rec.current_a = 380.0;
    rec.current_b = 2600.0;
    rec.current_c = 2580.0;
    rec.current_n = 20.0;
    rec.voltage_a = 6.35;
    rec.voltage_b = 2.10;
    rec.voltage_c = 2.15;

    FaultType_t type;
    char phases[16];
    double unbalance;

    Status_t st = classify_fault(&rec, 600.0, 6.35, &type, phases, &unbalance);
    EXPECT_EQ(st, STATUS_SUCCESS);
    EXPECT_EQ(type, FAULT_TYPE_LINE_TO_LINE);
    EXPECT_TRUE(strstr(phases, "B-C") != nullptr);
}

TEST(FaultAnalyzerTest, ProspectiveFaultCurrentCalculation) {
    ComplexImpedance_t zs = {0.1, 0.5};
    ComplexImpedance_t zf = {0.2, 0.5};
    /* Total Z = 0.3 + j1.0 -> |Z| = sqrt(0.09 + 1.0) = sqrt(1.09) = 1.04403 Ohms */
    /* V_LN = 6.35 kV = 6350 V */
    /* I_f = 6350 / 1.04403 = ~6082 A */
    double if_calc = 0.0;
    Status_t st = calculate_prospective_fault_current(6.35, zs, zf, &if_calc);
    EXPECT_EQ(st, STATUS_SUCCESS);
    EXPECT_NEAR(if_calc, 6082.2, 5.0);
}
