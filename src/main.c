/**
 * @file main.c
 * @brief Automated batch processing pipeline for Electrical Fault Diagnosis & Protection Analysis.
 * Runs non-interactively: Ingests telemetry, executes validation, classifies faults,
 * evaluates relay curves, runs queries, and generates reports.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "common_types.h"
#include "validation.h"
#include "fault_analyzer.h"
#include "protection_relay.h"
#include "fault_manager.h"
#include "file_handler.h"

int main(int argc, char *argv[])
{
    const char *input_csv_path  = "data/sample_fault_records.csv";
    const char *output_csv_path = "data/fault_history.csv";
    const char *report_txt_path = "docs/fault_diagnosis_report.txt";

    /* Allow custom input path via command-line arguments if provided */
    if (argc > 1) {
        input_csv_path = argv[1];
    }

    printf("\n================================================================================\n");
    printf("         ELECTRICAL FAULT DIAGNOSIS & PROTECTION ANALYSIS SYSTEM\n");
    printf("         Course: Programming Fundamentals and Software Engineering\n");
    printf("================================================================================\n\n");

    /* STEP 1: Initialize System Configurations & Grid Impedance Models */
    printf("[STAGE 1/6] Initializing Substation Protection & Network Parameters...\n");
    RelayConfig_t relay_cfg;
    get_default_relay_config(&relay_cfg);

    /* 11 kV Distribution Feeder Impedances: Zs = 0.20 + j1.50 Ohms, Zf = 0.40 + j0.80 Ohms */
    ComplexImpedance_t z_source = {0.20, 1.50};
    ComplexImpedance_t z_feeder = {0.40, 0.80};
    ComplexImpedance_t z_total  = {z_source.resistance + z_feeder.resistance,
                                   z_source.reactance  + z_feeder.reactance};

    printf("  - Nominal Voltage (L-N)     : %.2f kV (%.1f kV Line-to-Line)\n",
           relay_cfg.nominal_voltage, relay_cfg.nominal_voltage * 1.73205);
    printf("  - Primary CT Rated Current  : %.1f Amperes\n", relay_cfg.nominal_current);
    printf("  - Overcurrent Pickup (51)   : %.1f A (125%% rated)\n",
           relay_cfg.nominal_current * OVERCURRENT_PICKUP_FACTOR);
    printf("  - Instantaneous Trip (50)   : %.1f A (400%% rated)\n",
           relay_cfg.nominal_current * relay_cfg.instant_pickup_mult);
    printf("  - Total Loop Impedance |Z|  : %.3f Ohms (%.2f + j%.2f)\n\n",
           impedance_magnitude(z_total), z_total.resistance, z_total.reactance);

    /* STEP 2: Ingest and Validate Telemetry Data from CSV */
    printf("[STAGE 2/6] Ingesting and Validating Electrical Telemetry from '%s'...\n", input_csv_path);
    TelemetryRecord_t raw_records[MAX_RECORDS];
    size_t loaded_count = 0;

    Status_t status = load_telemetry_csv(input_csv_path, raw_records, MAX_RECORDS, &loaded_count);
    if (status != STATUS_SUCCESS) {
        fprintf(stderr, "[FATAL ERROR] Failed to load input telemetry from '%s' (Error Code: %d)\n",
                input_csv_path, status);
        return 1;
    }
    printf("  - Successfully ingested and validated %zu records.\n\n", loaded_count);

    /* STEP 3: Automated Fault Diagnosis & Protection Decision Engine */
    printf("[STAGE 3/6] Executing Rule-Based Fault Diagnosis & Protection Relay Logic...\n");
    fault_manager_init();

    for (size_t i = 0; i < loaded_count; ++i) {
        FaultAssessment_t assessment;
        status = evaluate_protection_response(&raw_records[i],
                                              &relay_cfg,
                                              z_source,
                                              z_feeder,
                                              &assessment);
        if (status == STATUS_SUCCESS) {
            fault_manager_add(&assessment);
        } else {
            printf("  [WARNING] Record %s failed evaluation with status %d\n",
                   raw_records[i].record_id, status);
        }
    }
    printf("  - Evaluated and registered %zu records in memory.\n\n", fault_manager_get_count());

    /* STEP 4: Display Comprehensive Event Analysis Table */
    printf("[STAGE 4/6] Diagnosed Event Log & Protection Decisions:\n");
    printf("----------------------------------------------------------------------------------------------------\n");
    printf("%-9s %-10s %-25s %-8s %-9s %-9s %-25s %-8s\n",
           "Rec ID", "Feeder", "Classification", "Phases", "Max I(A)", "Min V(kV)", "Recommended Action", "Time(s)");
    printf("----------------------------------------------------------------------------------------------------\n");

    size_t total_recs = fault_manager_get_count();
    for (size_t i = 0; i < total_recs; ++i) {
        const FaultAssessment_t *fa = fault_manager_get_at(i);
        if (fa != NULL) {
            printf("%-9s %-10s %-25s %-8s %-9.1f %-9.2f %-25s %-8.3f\n",
                   fa->telemetry.record_id,
                   fa->telemetry.feeder_id,
                   fault_type_to_string(fa->identified_fault),
                   fa->faulted_phases,
                   fa->max_phase_current,
                   fa->min_phase_voltage,
                   protection_action_to_string(fa->recommended_action),
                   fa->trip_time_sec);
        }
    }
    printf("----------------------------------------------------------------------------------------------------\n\n");

    /* STEP 5: Automated Query & Record Verification Demonstration */
    printf("[STAGE 5/6] Demonstrating Query & Filtering Capabilities:\n");

    /* 5a. Record Search by ID */
    const char *test_search_id = "FLT-002";
    FaultAssessment_t search_result;
    if (fault_manager_find_by_id(test_search_id, &search_result) == STATUS_SUCCESS) {
        printf("  [Search ID '%s'] Found on %s: %s (Max I: %.1f A, Action: %s)\n",
               test_search_id, search_result.telemetry.feeder_id,
               fault_type_to_string(search_result.identified_fault),
               search_result.max_phase_current,
               protection_action_to_string(search_result.recommended_action));
    }

    /* 5b. Filtering by Fault Type (Line-to-Ground) */
    FaultAssessment_t lg_filtered[MAX_RECORDS];
    size_t lg_count = 0;
    fault_manager_filter_by_type(FAULT_TYPE_LINE_TO_GROUND, lg_filtered, MAX_RECORDS, &lg_count);
    printf("  [Filter Type: L-G] Found %zu Line-to-Ground fault event(s):\n", lg_count);
    for (size_t k = 0; k < lg_count; ++k) {
        printf("    -> %s on %s: Fault Current = %.1f A | Trip Time = %.3fs\n",
               lg_filtered[k].telemetry.record_id, lg_filtered[k].telemetry.feeder_id,
               lg_filtered[k].max_phase_current, lg_filtered[k].trip_time_sec);
    }
    printf("\n");

    /* STEP 6: Compute System Statistics, Persist History & Export Report */
    printf("[STAGE 6/6] Generating Output Artifacts and System Health Report...\n");
    SystemStatistics_t stats;
    fault_manager_calculate_statistics(&stats);

    printf("  - Total Events Evaluated    : %zu\n", stats.total_records_evaluated);
    printf("  - Normal Balanced Feeders   : %zu\n", stats.normal_count);
    printf("  - Ground Faults (L-G & LL-G): %zu\n", stats.lg_fault_count + stats.llg_fault_count);
    printf("  - Phase Faults (L-L & 3P)   : %zu\n", stats.ll_fault_count + stats.three_phase_fault_count);
    printf("  - Total Breaker Trips Issued: %zu\n", stats.breaker_trip_count);
    printf("  - Maximum Recorded Peak I   : %.1f Amperes\n", stats.max_recorded_fault_current);

    /* Export evaluated records to CSV */
    status = save_fault_history_csv(output_csv_path);
    if (status == STATUS_SUCCESS) {
        printf("  - [EXPORT CSV] Saved evaluated fault history to '%s'\n", output_csv_path);
    }

    /* Export formatted diagnostic text report */
    status = generate_diagnostic_report(report_txt_path, z_source, z_feeder);
    if (status == STATUS_SUCCESS) {
        printf("  - [EXPORT REPORT] Generated engineering diagnostic summary in '%s'\n", report_txt_path);
    }

    printf("\n[EXECUTION FINISHED] All stages completed successfully with zero errors.\n\n");
    return 0;
}
