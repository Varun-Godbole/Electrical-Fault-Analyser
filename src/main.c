/**
 * @file main.c
 * @brief Main application entry point for Electrical Fault Diagnosis & Protection Analysis System.
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

static void clear_input_buffer(void)
{
    int c;
    while ((c = getchar()) != '\n' && c != EOF) {}
}

static void print_banner(void)
{
    printf("\n================================================================================\n");
    printf("         ELECTRICAL FAULT DIAGNOSIS & PROTECTION ANALYSIS SYSTEM\n");
    printf("         Course: Programming Fundamentals and Software Engineering\n");
    printf("================================================================================\n");
}

static void print_menu(void)
{
    printf("\n--- SYSTEM CONTROL MENU ---\n");
    printf("1. Load Recorded Electrical Telemetry from CSV\n");
    printf("2. Execute Fault Diagnosis & Protection Assessment\n");
    printf("3. Display Evaluated Fault History & Statistics\n");
    printf("4. Search Fault Record by Record ID\n");
    printf("5. Filter Records by Fault Type\n");
    printf("6. Calculate Prospective Short-Circuit Fault Current\n");
    printf("7. Generate Protection Diagnostic Report (.txt)\n");
    printf("8. Save Fault History to CSV\n");
    printf("0. Exit Application\n");
    printf("Enter choice [0-8]: ");
}

int main(void)
{
    /* 1. Initialize system configuration & impedance parameters */
    RelayConfig_t relay_cfg;
    get_default_relay_config(&relay_cfg);

    /* 11 kV Substation default impedances: Zs = 0.2 + j1.5, Zf = 0.4 + j0.8 */
    ComplexImpedance_t z_source = {0.20, 1.50};
    ComplexImpedance_t z_feeder = {0.40, 0.80};

    fault_manager_init();

    TelemetryRecord_t loaded_telemetry[MAX_RECORDS];
    size_t loaded_count = 0;
    bool evaluated = false;

    print_banner();

    int choice = -1;
    while (choice != 0) {
        print_menu();
        if (scanf("%d", &choice) != 1) {
            printf("\n[ERROR] Invalid input. Please enter an integer.\n");
            clear_input_buffer();
            continue;
        }
        clear_input_buffer();

        switch (choice) {
            case 1: {
                char path[MAX_NAME_LEN];
                printf("Enter CSV file path [default: data/sample_fault_records.csv]: ");
                if (fgets(path, sizeof(path), stdin) == NULL || path[0] == '\n') {
                    strncpy(path, "data/sample_fault_records.csv", sizeof(path));
                } else {
                    path[strcspn(path, "\r\n")] = 0;
                }

                Status_t st = load_telemetry_csv(path, loaded_telemetry, MAX_RECORDS, &loaded_count);
                if (st == STATUS_SUCCESS) {
                    printf("[SUCCESS] Loaded %zu valid electrical records from '%s'.\n", loaded_count, path);
                    evaluated = false;
                } else {
                    printf("[ERROR] Failed to load CSV file '%s' (Error Code: %d).\n", path, st);
                }
                break;
            }
            case 2: {
                if (loaded_count == 0) {
                    printf("[NOTICE] No telemetry loaded. Please load CSV data first (Option 1).\n");
                    break;
                }

                fault_manager_init();
                size_t processed = 0;
                for (size_t i = 0; i < loaded_count; ++i) {
                    FaultAssessment_t assessment;
                    Status_t st = evaluate_protection_response(&loaded_telemetry[i],
                                                              &relay_cfg,
                                                              z_source,
                                                              z_feeder,
                                                              &assessment);
                    if (st == STATUS_SUCCESS) {
                        fault_manager_add(&assessment);
                        processed++;
                    }
                }

                evaluated = true;
                printf("[ASSESSMENT COMPLETE] Evaluated %zu fault records successfully.\n", processed);
                break;
            }
            case 3: {
                if (!evaluated || fault_manager_get_count() == 0) {
                    printf("[NOTICE] Run diagnosis first (Option 2) to view history.\n");
                    break;
                }

                SystemStatistics_t stats;
                fault_manager_calculate_statistics(&stats);

                printf("\n--- SYSTEM OVERVIEW STATISTICS ---\n");
                printf("Total Evaluated Records : %zu\n", stats.total_records_evaluated);
                printf("Normal Operation Records: %zu\n", stats.normal_count);
                printf("Line-to-Ground (L-G)    : %zu\n", stats.lg_fault_count);
                printf("Line-to-Line (L-L)      : %zu\n", stats.ll_fault_count);
                printf("Double Line-to-Ground   : %zu\n", stats.llg_fault_count);
                printf("Three-Phase Symmetrical : %zu\n", stats.three_phase_fault_count);
                printf("Overload / Unbalanced   : %zu\n", stats.overload_count);
                printf("Total Breaker Trips     : %zu\n", stats.breaker_trip_count);
                printf("Max Recorded Current    : %.2f Amperes\n\n", stats.max_recorded_fault_current);

                printf("%-10s %-8s %-25s %-10s %-10s %-10s %-22s\n",
                       "Record ID", "Feeder", "Classification", "Phases", "Max I(A)", "Min V(kV)", "Action");
                printf("---------------------------------------------------------------------------------------------\n");
                size_t count = fault_manager_get_count();
                for (size_t i = 0; i < count; ++i) {
                    const FaultAssessment_t *fa = fault_manager_get_at(i);
                    printf("%-10s %-8s %-25s %-10s %-10.1f %-10.2f %-22s\n",
                           fa->telemetry.record_id,
                           fa->telemetry.feeder_id,
                           fault_type_to_string(fa->identified_fault),
                           fa->faulted_phases,
                           fa->max_phase_current,
                           fa->min_phase_voltage,
                           protection_action_to_string(fa->recommended_action));
                }
                break;
            }
            case 4: {
                char target_id[MAX_ID_LEN];
                printf("Enter Record ID to search (e.g. FLT-001): ");
                if (fgets(target_id, sizeof(target_id), stdin) == NULL) break;
                target_id[strcspn(target_id, "\r\n")] = 0;

                FaultAssessment_t found;
                Status_t st = fault_manager_find_by_id(target_id, &found);
                if (st == STATUS_SUCCESS) {
                    printf("\n--- FAULT RECORD DETAILS [%s] ---\n", found.telemetry.record_id);
                    printf("Feeder ID            : %s\n", found.telemetry.feeder_id);
                    printf("Timestamp            : %s\n", found.telemetry.timestamp);
                    printf("Phase Currents (A)   : A=%.1f, B=%.1f, C=%.1f, Neutral=%.1f\n",
                           found.telemetry.current_a, found.telemetry.current_b,
                           found.telemetry.current_c, found.telemetry.current_n);
                    printf("Phase Voltages (kV)  : A=%.2f, B=%.2f, C=%.2f\n",
                           found.telemetry.voltage_a, found.telemetry.voltage_b, found.telemetry.voltage_c);
                    printf("Fault Classification : %s\n", fault_type_to_string(found.identified_fault));
                    printf("Faulted Phases       : %s\n", found.faulted_phases);
                    printf("Overcurrent Condition: %s\n", found.is_overcurrent ? "YES (Exceeded Pickup)" : "NO");
                    printf("Undervoltage (UV 27) : %s\n", found.is_undervoltage ? "YES (Voltage Sag Detected)" : "NO");
                    printf("Calculated Fault I   : %.2f A\n", found.calculated_fault_current);
                    printf("Recommended Action   : %s\n", protection_action_to_string(found.recommended_action));
                    printf("Relay Operating Time : %.3f seconds\n", found.trip_time_sec);
                } else {
                    printf("[ERROR] Record '%s' not found in history.\n", target_id);
                }
                break;
            }
            case 5: {
                printf("\nSelect Fault Type to filter:\n");
                printf("0. Normal Operation\n");
                printf("1. Line-to-Ground (L-G)\n");
                printf("2. Line-to-Line (L-L)\n");
                printf("3. Double Line-to-Ground (L-L-G)\n");
                printf("4. Three-Phase Symmetrical (3P)\n");
                printf("5. Overload / Unbalanced\n");
                printf("Enter type [0-5]: ");
                int t = 0;
                if (scanf("%d", &t) != 1 || t < 0 || t > 5) {
                    printf("[ERROR] Invalid fault type selection.\n");
                    clear_input_buffer();
                    break;
                }
                clear_input_buffer();

                FaultAssessment_t filtered[MAX_RECORDS];
                size_t filter_count = 0;
                fault_manager_filter_by_type((FaultType_t)t, filtered, MAX_RECORDS, &filter_count);

                printf("\nFound %zu matching record(s) for '%s':\n", filter_count, fault_type_to_string((FaultType_t)t));
                for (size_t i = 0; i < filter_count; ++i) {
                    printf(" -> ID: %-10s | Feeder: %-8s | Max Current: %8.1f A | Action: %s\n",
                           filtered[i].telemetry.record_id,
                           filtered[i].telemetry.feeder_id,
                           filtered[i].max_phase_current,
                           protection_action_to_string(filtered[i].recommended_action));
                }
                break;
            }
            case 6: {
                double v_kv = 0.0, rs = 0.0, xs = 0.0, rf = 0.0, xf = 0.0;
                printf("\nEnter Nominal Line-to-Neutral Voltage (kV) [%.2f]: ", relay_cfg.nominal_voltage);
                if (scanf("%lf", &v_kv) != 1 || v_kv <= 0.0) v_kv = relay_cfg.nominal_voltage;
                printf("Enter Source Resistance Rs (Ohms): ");
                if (scanf("%lf", &rs) != 1) rs = z_source.resistance;
                printf("Enter Source Reactance Xs (Ohms): ");
                if (scanf("%lf", &xs) != 1) xs = z_source.reactance;
                printf("Enter Feeder Resistance Rf (Ohms): ");
                if (scanf("%lf", &rf) != 1) rf = z_feeder.resistance;
                printf("Enter Feeder Reactance Xf (Ohms): ");
                if (scanf("%lf", &xf) != 1) xf = z_feeder.reactance;
                clear_input_buffer();

                ComplexImpedance_t zs = {rs, xs};
                ComplexImpedance_t zf = {rf, xf};
                double prospective_i = 0.0;
                Status_t st = calculate_prospective_fault_current(v_kv, zs, zf, &prospective_i);
                if (st == STATUS_SUCCESS) {
                    printf("\n[CALCULATION RESULT]\n");
                    printf("Total Short-Circuit Loop Impedance : %.3f + j%.3f Ohms (|Z| = %.3f Ohms)\n",
                           zs.resistance + zf.resistance, zs.reactance + zf.reactance,
                           impedance_magnitude((ComplexImpedance_t){zs.resistance + zf.resistance, zs.reactance + zf.reactance}));
                    printf("Theoretical Symmetrical Fault Current: %.2f Amperes (%.3f kA)\n",
                           prospective_i, prospective_i / 1000.0);
                } else {
                    printf("[ERROR] Calculation failed with error code %d\n", st);
                }
                break;
            }
            case 7: {
                const char *rpt_path = "docs/fault_diagnosis_report.txt";
                Status_t st = generate_diagnostic_report(rpt_path, z_source, z_feeder);
                if (st == STATUS_SUCCESS) {
                    printf("[SUCCESS] Diagnostic report successfully generated at '%s'.\n", rpt_path);
                } else {
                    printf("[ERROR] Failed to generate report (Code %d).\n", st);
                }
                break;
            }
            case 8: {
                const char *out_csv = "data/fault_history.csv";
                Status_t st = save_fault_history_csv(out_csv);
                if (st == STATUS_SUCCESS) {
                    printf("[SUCCESS] Fault history saved to '%s'.\n", out_csv);
                } else {
                    printf("[ERROR] Failed to save CSV history (Code %d).\n", st);
                }
                break;
            }
            case 0: {
                printf("\nExiting Electrical Fault Diagnosis & Protection System. Goodbye!\n");
                break;
            }
            default: {
                printf("[WARNING] Invalid option selected. Please choose between 0 and 8.\n");
                break;
            }
        }
    }

    return 0;
}
