/**
 * @file common_types.h
 * @brief Common types, enumerations, and constants for Electrical Fault Diagnosis & Protection System.
 */

#ifndef COMMON_TYPES_H
#define COMMON_TYPES_H

#include <stddef.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* System Constants */
#define MAX_ID_LEN             32
#define MAX_NAME_LEN           64
#define MAX_DATE_LEN           32
#define MAX_RECORDS            256
#define MAX_LINE_BUFFER        512

/* Default Electrical Thresholds */
#define DEFAULT_NOMINAL_VOLTAGE_KV   11.0    /* 11 kV Distribution Feeder */
#define DEFAULT_CT_PRIMARY_RATIO     600.0   /* 600 A Nominal Rating */
#define OVERCURRENT_PICKUP_FACTOR    1.25    /* 125% of rated nominal */
#define INSTANTANEOUS_PICKUP_FACTOR  4.00    /* 400% of rated nominal (ANSI 50) */
#define UNDERVOLTAGE_THRESHOLD_PU    0.80    /* 80% of nominal voltage (ANSI 27) */

/**
 * @brief System Error and Status Codes
 */
typedef enum {
    STATUS_SUCCESS             = 0,
    STATUS_ERR_INVALID_PARAM   = -1,
    STATUS_ERR_OUT_OF_RANGE    = -2,
    STATUS_ERR_NULL_PTR        = -3,
    STATUS_ERR_FILE_IO         = -4,
    STATUS_ERR_FILE_FORMAT     = -5,
    STATUS_ERR_RECORD_NOT_FOUND= -6,
    STATUS_ERR_CAPACITY_FULL   = -7
} Status_t;

/**
 * @brief Electrical Fault Classification Types
 */
typedef enum {
    FAULT_TYPE_NORMAL              = 0,  /* Balanced normal operating state */
    FAULT_TYPE_LINE_TO_GROUND      = 1,  /* Single Line-to-Ground (L-G) */
    FAULT_TYPE_LINE_TO_LINE        = 2,  /* Line-to-Line (L-L) */
    FAULT_TYPE_DOUBLE_LINE_GROUND  = 3,  /* Double Line-to-Ground (L-L-G) */
    FAULT_TYPE_THREE_PHASE         = 4,  /* Symmetrical Three-Phase (L-L-L / L-L-L-G) */
    FAULT_TYPE_OVERLOAD_UNBALANCED = 5   /* Phase unbalance or moderate sustained overload */
} FaultType_t;

/**
 * @brief Recommended Protection System Action
 */
typedef enum {
    ACTION_NO_ACTION               = 0,  /* System healthy, maintain supply */
    ACTION_MONITOR_ALARM           = 1,  /* Warning alert; minor unbalance/overload */
    ACTION_TRIP_FEEDER_BREAKER     = 2,  /* Time-delayed breaker trip (ANSI 51) */
    ACTION_INSTANTANEOUS_TRIP      = 3,  /* High-speed instantaneous trip (ANSI 50) */
    ACTION_EMERGENCY_LOCKOUT       = 4   /* Severe symmetrical fault; trip and lockout bus */
} ProtectionAction_t;

/**
 * @brief Complex Impedance (R + jX)
 */
typedef struct {
    double resistance; /* Ohms (R) */
    double reactance;  /* Ohms (X) */
} ComplexImpedance_t;

/**
 * @brief 3-Phase Measurement Data Snapshot
 */
typedef struct {
    char record_id[MAX_ID_LEN];
    char feeder_id[MAX_ID_LEN];
    char timestamp[MAX_DATE_LEN];
    double current_a;  /* Phase A Current (Amperes RMS) */
    double current_b;  /* Phase B Current (Amperes RMS) */
    double current_c;  /* Phase C Current (Amperes RMS) */
    double current_n;  /* Neutral / Earth Current (Amperes RMS) */
    double voltage_a;  /* Phase A Voltage (kV RMS Line-to-Neutral) */
    double voltage_b;  /* Phase B Voltage (kV RMS Line-to-Neutral) */
    double voltage_c;  /* Phase C Voltage (kV RMS Line-to-Neutral) */
} TelemetryRecord_t;

/**
 * @brief Comprehensive Fault Diagnosis & Protection Assessment Result
 */
typedef struct {
    TelemetryRecord_t telemetry;
    FaultType_t identified_fault;
    char faulted_phases[16];          /* e.g., "A-G", "B-C", "A-B-C" */
    double max_phase_current;         /* Maximum recorded phase current (A) */
    double min_phase_voltage;         /* Lowest recorded phase voltage (kV) */
    double calculated_fault_current;  /* Theoretical prospective fault current (A) */
    double current_unbalance_ratio;   /* Ratio of negative/zero sequence unbalance */
    bool is_overcurrent;              /* True if current exceeds pickup limit */
    bool is_undervoltage;             /* True if voltage drops below ANSI 27 limit */
    ProtectionAction_t recommended_action;
    double trip_time_sec;             /* Operating time of relay (seconds) */
} FaultAssessment_t;

/**
 * @brief System Overall Statistics & Health Summary
 */
typedef struct {
    size_t total_records_evaluated;
    size_t normal_count;
    size_t lg_fault_count;
    size_t ll_fault_count;
    size_t llg_fault_count;
    size_t three_phase_fault_count;
    size_t overload_count;
    size_t breaker_trip_count;
    double max_recorded_fault_current;
} SystemStatistics_t;

#ifdef __cplusplus
}
#endif

#endif /* COMMON_TYPES_H */
