# Detailed Software Design: Electrical Fault Diagnosis & Protection System

## 1. System Architecture
The software is architected into clear, decoupled layers:
1. **User Interface Layer (`main.c`):** Interactive terminal command-line interface controlling overall application workflow.
2. **Validation Layer (`validation.c`):** Enforces defensive guard conditions on data formats, physical current/voltage limits, and impedance values.
3. **Fault Analysis Layer (`fault_analyzer.c`):** Implements rule-based heuristics to classify fault types and computes prospective short-circuit currents.
4. **Protection Relay Layer (`protection_relay.c`):** Evaluates ANSI 50 (Instantaneous), ANSI 51 (Time-Overcurrent IEC curve), and ANSI 27 (Undervoltage) conditions, recommending switchgear actions.
5. **Data Management Layer (`fault_manager.c`):** In-memory storage repository providing fast lookup by Record ID, filtering by fault classification, and statistics aggregation.
6. **File I/O Layer (`file_handler.c`):** Handles safe parsing of telemetry CSV files, export of evaluated histories, and formatted diagnostic reporting.

---

## 2. Module Specifications

### Module 1: Validation (`validation.h`, `validation.c`)
- **Responsibilities:** Validates pointers, string lengths, numerical boundaries, and physical validity.
- **Key Functions:**
  - `Status_t validate_telemetry_record(const TelemetryRecord_t *record)`
  - `Status_t validate_impedance(const ComplexImpedance_t *z)`
  - `Status_t validate_relay_settings(double nominal_current, double nominal_voltage)`

### Module 2: Fault Analyzer (`fault_analyzer.h`, `fault_analyzer.c`)
- **Responsibilities:** Identifies fault types, detects faulted phases, computes phase unbalance, and calculates short-circuit current.
- **Key Functions:**
  - `Status_t classify_fault(...)`
  - `Status_t calculate_prospective_fault_current(double v_ln_kv, ComplexImpedance_t zs, ComplexImpedance_t zf, double *fault_current_out)`
  - `double impedance_magnitude(ComplexImpedance_t z)`
  - `const char* fault_type_to_string(FaultType_t type)`
  - `const char* protection_action_to_string(ProtectionAction_t action)`

### Module 3: Protection Relay (`protection_relay.h`, `protection_relay.c`)
- **Responsibilities:** Determines overcurrent/undervoltage conditions, computes IEC inverse-time trip delays, and selects recommended protection actions.
- **Key Functions:**
  - `void get_default_relay_config(RelayConfig_t *cfg)`
  - `double calculate_iec_trip_time(double current, double pickup_current, double tms)`
  - `Status_t evaluate_protection_response(const TelemetryRecord_t *record, const RelayConfig_t *cfg, ComplexImpedance_t zs, ComplexImpedance_t zf, FaultAssessment_t *assessment_out)`

### Module 4: Fault Manager (`fault_manager.h`, `fault_manager.c`)
- **Responsibilities:** Manages the in-memory fault history table, record lookup, filtering, and aggregate statistics calculation.
- **Key Functions:**
  - `void fault_manager_init(void)`
  - `Status_t fault_manager_add(const FaultAssessment_t *assessment)`
  - `Status_t fault_manager_find_by_id(const char *record_id, FaultAssessment_t *assessment_out)`
  - `Status_t fault_manager_filter_by_type(FaultType_t type, FaultAssessment_t *results_out, size_t max_results, size_t *count_out)`
  - `Status_t fault_manager_calculate_statistics(SystemStatistics_t *stats_out)`

### Module 5: File Handler (`file_handler.h`, `file_handler.c`)
- **Responsibilities:** Handles structured CSV reading, fault assessment exporting, and generating comprehensive text diagnostic reports.
- **Key Functions:**
  - `Status_t load_telemetry_csv(const char *filepath, TelemetryRecord_t *records_out, size_t max_records, size_t *count_out)`
  - `Status_t save_fault_history_csv(const char *filepath)`
  - `Status_t generate_diagnostic_report(const char *filepath, ComplexImpedance_t zs, ComplexImpedance_t zf)`

---

## 3. Data Design

### 3.1 Structure Definitions
```c
typedef struct {
    double resistance; /* Ohms (R) */
    double reactance;  /* Ohms (X) */
} ComplexImpedance_t;

typedef struct {
    char record_id[MAX_ID_LEN];
    char feeder_id[MAX_ID_LEN];
    char timestamp[MAX_DATE_LEN];
    double current_a, current_b, current_c, current_n;
    double voltage_a, voltage_b, voltage_c;
} TelemetryRecord_t;

typedef struct {
    TelemetryRecord_t telemetry;
    FaultType_t identified_fault;
    char faulted_phases[16];
    double max_phase_current;
    double min_phase_voltage;
    double calculated_fault_current;
    double current_unbalance_ratio;
    bool is_overcurrent;
    bool is_undervoltage;
    ProtectionAction_t recommended_action;
    double trip_time_sec;
} FaultAssessment_t;
```

### 3.2 Enumerations
```c
typedef enum {
    FAULT_TYPE_NORMAL              = 0,
    FAULT_TYPE_LINE_TO_GROUND      = 1,
    FAULT_TYPE_LINE_TO_LINE        = 2,
    FAULT_TYPE_DOUBLE_LINE_GROUND  = 3,
    FAULT_TYPE_THREE_PHASE         = 4,
    FAULT_TYPE_OVERLOAD_UNBALANCED = 5
} FaultType_t;

typedef enum {
    ACTION_NO_ACTION               = 0,
    ACTION_MONITOR_ALARM           = 1,
    ACTION_TRIP_FEEDER_BREAKER     = 2,
    ACTION_INSTANTANEOUS_TRIP      = 3,
    ACTION_EMERGENCY_LOCKOUT       = 4
} ProtectionAction_t;

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
```

---

## 4. Input-Validation Plan
| Parameter | Source | Validation Criteria | Invalid Example | Action |
| :--- | :--- | :--- | :--- | :--- |
| `record_id` | CSV / User | Non-empty string, length < 32 | `""` (empty) | Returns `STATUS_ERR_INVALID_PARAM` |
| `current_*` | CSV | $0.0 \le I \le 100,000.0$ A | `-15.0`, `150000.0` | Returns `STATUS_ERR_OUT_OF_RANGE` |
| `voltage_*` | CSV | $0.0 \le V \le 1,000.0$ kV | `-2.0`, `5000.0` | Returns `STATUS_ERR_OUT_OF_RANGE` |
| `impedance` | Internal / User | $R \ge 0$, $|Z| > 10^{-4}\ \Omega$ | $R = -1.0, |Z|=0$ | Returns `STATUS_ERR_INVALID_PARAM` |
| Pointers | Module calls | `ptr != NULL` | `NULL` passed | Returns `STATUS_ERR_NULL_PTR` |

---

## 5. File-Handling Design
- **`data/sample_fault_records.csv`:** Input dataset containing multi-channel electrical measurements. Parsed safely using bounded `sscanf` with line validation.
- **`data/fault_history.csv`:** Exported assessment table preserving detected fault classes, trip times, and recommended switchgear actions.
- **`docs/fault_diagnosis_report.txt`:** Formatted human-readable report summarizing total statistics, event breakdown, and protection recommendations.

---

## 6. Unit-Test Plan (Google Test)
| Test ID | Module | Category | Description | Expected Outcome |
| :--- | :--- | :--- | :--- | :--- |
| **UT-01** | `validation` | Positive | Valid telemetry record with standard parameters | `STATUS_SUCCESS` |
| **UT-02** | `validation` | Negative | NULL pointer passed to validator | `STATUS_ERR_NULL_PTR` |
| **UT-03** | `validation` | Boundary | Current at boundary (0.0 A vs negative vs extreme) | Range check enforcement |
| **UT-04** | `validation` | Boundary | Zero impedance singularity check ($R=0, X=0$) | `STATUS_ERR_INVALID_PARAM` |
| **UT-05** | `fault_analyzer` | Positive | Symmetrical high current and voltage sag | `FAULT_TYPE_THREE_PHASE` |
| **UT-06** | `fault_analyzer` | Positive | Single phase elevated current with ground return | `FAULT_TYPE_LINE_TO_GROUND` |
| **UT-07** | `fault_analyzer` | Positive | Two phases elevated without ground return | `FAULT_TYPE_LINE_TO_LINE` |
| **UT-08** | `fault_analyzer` | Calculation | Prospective short-circuit calculation $V/|Z|$ | Accurately matches theoretical $I_f$ |
| **UT-09** | `protection_relay`| Positive | Normal rated currents and voltages | `ACTION_NO_ACTION`, no trip |
| **UT-10** | `protection_relay`| Boundary | Current exceeding $4\times I_{nominal}$ | Instantaneous 50 ms trip |
| **UT-11** | `protection_relay`| Boundary | Operating time curve calculation across ratios | Correct inverse-time behavior |
| **UT-12** | `fault_manager` | Positive | Adding record and searching by Record ID | `STATUS_SUCCESS` and data matches |
| **UT-13** | `fault_manager` | Positive | Filtering records by fault type & aggregate stats | Exact count and statistics match |
