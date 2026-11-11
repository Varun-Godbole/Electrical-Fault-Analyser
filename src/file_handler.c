/**
 * @file file_handler.c
 * @brief Implementation of robust CSV file parser, history export, and report generator.
 */

#include "file_handler.h"
#include "fault_manager.h"
#include "fault_analyzer.h"
#include "validation.h"
#include <stdio.h>
#include <string.h>

Status_t load_telemetry_csv(const char *filepath,
                            TelemetryRecord_t *records_out,
                            size_t max_records,
                            size_t *count_out)
{
    if (filepath == NULL || records_out == NULL || count_out == NULL) {
        return STATUS_ERR_NULL_PTR;
    }

    FILE *fp = fopen(filepath, "r");
    if (fp == NULL) {
        return STATUS_ERR_FILE_IO;
    }

    char line[MAX_LINE_BUFFER];
    /* Read and skip header line */
    if (fgets(line, sizeof(line), fp) == NULL) {
        fclose(fp);
        return STATUS_ERR_FILE_FORMAT;
    }

    size_t count = 0;
    while (fgets(line, sizeof(line), fp) != NULL && count < max_records) {
        /* Ignore blank lines */
        if (line[0] == '\n' || line[0] == '\r') continue;

        TelemetryRecord_t rec;
        memset(&rec, 0, sizeof(rec));

        /* Format: record_id,feeder_id,timestamp,Ia,Ib,Ic,In,Va,Vb,Vc */
        int parsed = sscanf(line, "%31[^,],%31[^,],%31[^,],%lf,%lf,%lf,%lf,%lf,%lf,%lf",
                            rec.record_id,
                            rec.feeder_id,
                            rec.timestamp,
                            &rec.current_a,
                            &rec.current_b,
                            &rec.current_c,
                            &rec.current_n,
                            &rec.voltage_a,
                            &rec.voltage_b,
                            &rec.voltage_c);

        if (parsed == 10) {
            if (validate_telemetry_record(&rec) == STATUS_SUCCESS) {
                records_out[count++] = rec;
            }
        }
    }

    fclose(fp);
    *count_out = count;
    return STATUS_SUCCESS;
}

Status_t save_fault_history_csv(const char *filepath)
{
    if (filepath == NULL) {
        return STATUS_ERR_NULL_PTR;
    }

    FILE *fp = fopen(filepath, "w");
    if (fp == NULL) {
        return STATUS_ERR_FILE_IO;
    }

    /* CSV Header */
    fprintf(fp, "record_id,feeder_id,timestamp,fault_type,faulted_phases,max_current_A,min_voltage_kV,calc_fault_A,is_oc,is_uv,recommended_action,trip_time_s\n");

    size_t total = fault_manager_get_count();
    for (size_t i = 0; i < total; ++i) {
        const FaultAssessment_t *fa = fault_manager_get_at(i);
        if (fa != NULL) {
            fprintf(fp, "%s,%s,%s,%s,%s,%.2f,%.3f,%.2f,%s,%s,%s,%.3f\n",
                    fa->telemetry.record_id,
                    fa->telemetry.feeder_id,
                    fa->telemetry.timestamp,
                    fault_type_to_string(fa->identified_fault),
                    fa->faulted_phases,
                    fa->max_phase_current,
                    fa->min_phase_voltage,
                    fa->calculated_fault_current,
                    fa->is_overcurrent ? "YES" : "NO",
                    fa->is_undervoltage ? "YES" : "NO",
                    protection_action_to_string(fa->recommended_action),
                    fa->trip_time_sec);
        }
    }

    fclose(fp);
    return STATUS_SUCCESS;
}

Status_t generate_diagnostic_report(const char *filepath,
                                    ComplexImpedance_t z_source,
                                    ComplexImpedance_t z_feeder)
{
    if (filepath == NULL) {
        return STATUS_ERR_NULL_PTR;
    }

    FILE *fp = fopen(filepath, "w");
    if (fp == NULL) {
        return STATUS_ERR_FILE_IO;
    }

    SystemStatistics_t stats;
    fault_manager_calculate_statistics(&stats);

    fprintf(fp, "================================================================================\n");
    fprintf(fp, "             ELECTRICAL FAULT DIAGNOSIS & PROTECTION ANALYSIS REPORT            \n");
    fprintf(fp, "================================================================================\n\n");

    fprintf(fp, "1. SYSTEM IMPEDANCE PARAMETERS:\n");
    fprintf(fp, "   - Source Impedance (Zs) : %.3f + j%.3f Ohms (Mag: %.3f Ohms)\n",
            z_source.resistance, z_source.reactance, impedance_magnitude(z_source));
    fprintf(fp, "   - Feeder Impedance (Zf) : %.3f + j%.3f Ohms (Mag: %.3f Ohms)\n",
            z_feeder.resistance, z_feeder.reactance, impedance_magnitude(z_feeder));
    ComplexImpedance_t z_tot = {z_source.resistance + z_feeder.resistance, z_source.reactance + z_feeder.reactance};
    fprintf(fp, "   - Total Loop Impedance  : %.3f + j%.3f Ohms (Mag: %.3f Ohms)\n\n",
            z_tot.resistance, z_tot.reactance, impedance_magnitude(z_tot));

    fprintf(fp, "2. FAULT EVENT SUMMARY STATISTICS:\n");
    fprintf(fp, "   - Total Records Evaluated : %zu\n", stats.total_records_evaluated);
    fprintf(fp, "   - Normal Healthy State    : %zu\n", stats.normal_count);
    fprintf(fp, "   - Line-to-Ground (L-G)    : %zu\n", stats.lg_fault_count);
    fprintf(fp, "   - Line-to-Line (L-L)      : %zu\n", stats.ll_fault_count);
    fprintf(fp, "   - Double Line-to-Ground   : %zu\n", stats.llg_fault_count);
    fprintf(fp, "   - Three-Phase Symmetrical : %zu\n", stats.three_phase_fault_count);
    fprintf(fp, "   - Sustained Overload/Unbal: %zu\n", stats.overload_count);
    fprintf(fp, "   - Total Breaker Trips     : %zu\n", stats.breaker_trip_count);
    fprintf(fp, "   - Max Peak Fault Current  : %.2f Amperes\n\n", stats.max_recorded_fault_current);

    fprintf(fp, "3. EVALUATED FAULT LOG DETAILS:\n");
    fprintf(fp, "%-10s %-10s %-25s %-10s %-10s %-10s %-24s\n",
            "Record ID", "Feeder", "Fault Classification", "Phases", "Max I (A)", "Min V (kV)", "Recommended Action");
    fprintf(fp, "----------------------------------------------------------------------------------------------------\n");

    size_t count = fault_manager_get_count();
    for (size_t i = 0; i < count; ++i) {
        const FaultAssessment_t *fa = fault_manager_get_at(i);
        if (fa != NULL) {
            fprintf(fp, "%-10s %-10s %-25s %-10s %-10.1f %-10.2f %-24s\n",
                    fa->telemetry.record_id,
                    fa->telemetry.feeder_id,
                    fault_type_to_string(fa->identified_fault),
                    fa->faulted_phases,
                    fa->max_phase_current,
                    fa->min_phase_voltage,
                    protection_action_to_string(fa->recommended_action));
        }
    }

    fprintf(fp, "\n================================================================================\n");
    fprintf(fp, "END OF PROTECTION ANALYSIS REPORT\n");

    fclose(fp);
    return STATUS_SUCCESS;
}
