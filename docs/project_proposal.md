# Project Proposal: Electrical Fault Diagnosis & Protection Analysis System

## 1. Project Information
- **Project Title:** Electrical Fault Diagnosis & Protection Analysis System
- **Engineering Domain:** Energy / Electrical Power Systems / Industrial Automation
- **Team Details:**
  - **Student Name:** [Individual Student Submission]
  - **Roll Number:** [Student Roll Number]
  - **Division / Batch:** [Division / Batch]
  - **Repository Name:** electrical-fault-diagnosis-system
  - **GitHub Repository Link:** https://github.com/[username]/electrical-fault-diagnosis-system

### 1.1 Individual Project Declaration
I confirm that this project is my individual work. I understand the design, source code, unit tests, Makefile and documentation included in this repository, and I am prepared to explain or modify any part during evaluation.

---

## 2. Engineering Scenario
In electrical distribution networks, industrial manufacturing plants, and substation switchyards, transmission and feeder lines are subject to unpredictable insulation breakdown, physical contact, environmental surges, and lightning strikes. 

Currently, maintenance engineers and switchgear operators often rely on manual oscillogram reviews or delayed inspection logs after a feeder trips. Without real-time automated fault type classification and short-circuit prospective calculations, operators face difficulties determining whether a trip was caused by a transient single-line-to-ground (L-G) arc, a severe two-phase line-to-line (L-L) contact, or a catastrophic symmetrical three-phase short circuit.

This delay results in:
1. Prolonged power outages and extended downtime for industrial production.
2. Inappropriate reclosing into persistent dead short-circuits, damaging multi-million-dollar power transformers.
3. Lack of structured digital logs and difficulty in verifying whether protection relays operated within their designated ANSI 50/51/27 time curves.

---

## 3. Problem Statement
In electrical power distribution networks, substation maintenance engineers currently face delayed and error-prone identification of fault types and prospective short-circuit currents during feeder tripping events. The current method results in prolonged power outages, inaccurate relay coordination assessments, and risks of equipment damage from reclosing into uncleared faults. The proposed software will provide automated rule-based fault identification, symmetrical prospective fault current calculation, ANSI 50/51 overcurrent and ANSI 27 undervoltage evaluation, and recommended protection actions so that users can rapidly diagnose network faults, maintain verifiable fault histories, and optimize switchgear protection response.

---

## 4. Proposed Solution
The proposed software is a modular, high-reliability C engineering application that:
- **Parses Recorded Electrical Telemetry:** Ingests 3-phase currents ($I_A, I_B, I_C$), neutral/ground current ($I_N$), and voltages ($V_A, V_B, V_C$) from recorded CSV switchgear event logs.
- **Rule-Based Fault Classification:** Employs pre-defined power engineering rules comparing phase anomalies, ground return currents, and voltage collapse to classify:
  - Normal Balanced Operation
  - Single Line-to-Ground (L-G) Fault
  - Line-to-Line (L-L) Fault
  - Double Line-to-Ground (L-L-G) Fault
  - Three-Phase Symmetrical (3P) Fault
  - Sustained Overload / Phase Current Unbalance
- **Prospective Short-Circuit Current Calculation:** Solves prospective theoretical short-circuit current from system nominal voltage and complex loop impedances ($Z_{source} + Z_{feeder}$).
- **Protective Relay Decision Engine:** Evaluates instantaneous overcurrent (ANSI 50), IEC standard inverse-time overcurrent curves (ANSI 51), and undervoltage sag (ANSI 27) to recommend actionable switchgear responses (e.g., Instantaneous Trip, Feeder Breaker Trip, Bus Lockout, Alarm).
- **History Management & Diagnostics:** Stores records in an in-memory repository with ID search, fault-type filtering, statistical aggregation, and formatted text report generation.

---

## 5. Target Users
| User Type | Responsibility | How the User Will Use the Application |
| :--- | :--- | :--- |
| **Substation Protection Engineer** | Relay coordination & fault clearance verification | Loads event telemetry, validates tripping times against ANSI/IEC curves, verifies breaker action. |
| **Industrial Plant Electrician** | Feeder monitoring & preventative maintenance | Checks phase unbalance ratios, monitors recurring overload alarms, and reviews historical fault frequencies. |
| **Power Systems Researcher / Student** | Short-circuit analysis & algorithm validation | Evaluates prospective fault calculations against recorded waveforms and tests custom impedance models. |

---

## 6. Project Scope
### 6.1 In-Scope Features
- Ingestion of structured electrical telemetry logs from CSV files.
- Rule-based multi-phase fault detection and classification.
- Calculation of theoretical prospective fault current using complex impedance models.
- ANSI 50 (Instantaneous Overcurrent) and ANSI 51 (IEC Inverse Time) curve evaluation.
- ANSI 27 (Undervoltage) sag detection.
- In-memory fault history repository with ID search and fault-type filtering.
- Comprehensive statistics computation and diagnostic text report generation.
- Automated unit testing using Google Test covering positive, negative, and boundary cases.
- Automated build management using a cross-platform Makefile.

### 6.2 Out-of-Scope Features
- Real-time analog-to-digital converter (ADC) sampling at megahertz frequencies.
- High-voltage arc flash thermal optical spectral analysis.
- Remote SCADA DNP3 / IEC 61850 network protocol stack communications.

### 6.3 Assumptions & Constraints
- Assumes 50/60 Hz three-phase alternating current distribution systems.
- Maximum storage buffer size of 256 records in static in-memory repository for deterministic memory usage.
- Strict ANSI C99 compliance, compiling warning-free under `-Wall -Wextra -Wpedantic`.

---

## 7. Requirements
### 7.1 Functional Requirements
- **FR-01:** The system shall parse and validate recorded 3-phase current, neutral current, and voltage telemetry from CSV files.
- **FR-02:** The system shall validate physical boundaries of electrical current, voltage, and complex impedance parameters.
- **FR-03:** The system shall identify the specific fault type (Normal, L-G, L-L, L-L-G, 3P, Overload) based on predefined rule heuristics.
- **FR-04:** The system shall compute theoretical prospective short-circuit current from nominal voltage and complex loop impedance ($Z_s + Z_L$).
- **FR-05:** The system shall detect overcurrent and undervoltage conditions against configurable relay pickup limits.
- **FR-06:** The system shall calculate relay operating trip times according to the IEC standard inverse time-overcurrent curve.
- **FR-07:** The system shall recommend appropriate protection actions (No Action, Alarm, Feeder Trip, Instantaneous Trip, Bus Lockout).
- **FR-08:** The system shall maintain an in-memory fault history supporting record search by ID and filtering by fault classification.
- **FR-09:** The system shall export evaluated fault records to CSV and generate human-readable engineering diagnostic summary reports.

### 7.2 Non-Functional Requirements
- **NFR-01:** The source code shall compile without warnings using `-Wall -Wextra -Wpedantic`.
- **NFR-02:** The project shall use a modular multi-file architecture with strict separation of headers (`include/`) and source (`src/`).
- **NFR-03:** The application shall handle invalid, corrupted, or out-of-range inputs gracefully without crashing.
- **NFR-04:** Critical calculation, classification, and validation functions shall be verified via Google Test.
- **NFR-05:** The project shall provide automated building and execution via a Makefile supporting `all`, `run`, `test`, and `clean`.
- **NFR-06:** The project shall be maintained in a Git repository with progressive, descriptive commits.

---

## 8. High-Level Architecture & Workflow
```
[Raw Event Telemetry CSV]
          │
          ▼
┌──────────────────────────┐
│ Telemetry Ingestion &    │ <── Guard Check: Voltage, Current, String Bounds
│ Validation Layer         │
└──────────┬───────────────┘
           │ Validated Records
           ▼
┌──────────────────────────┐     System Impedance (Zs, Zf)
│ Fault Analysis Engine    │ <───────────────────────────────┐
│ (Rule-Based Classifier)  │                                 │
└──────────┬───────────────┘                                 │
           │ Fault Type, Inbalance Ratio                     │
           ▼                                                 │
┌──────────────────────────┐                                 │
│ Protection Relay Engine  │ ──> Calculate Fault Current (If)┘
│ (ANSI 50 / 51 / 27)      │ ──> Calculate IEC Trip Time (t)
└──────────┬───────────────┘ ──> Recommend Switchgear Action
           │
           ▼
┌──────────────────────────┐
│ Fault History Repository │ <── Search by ID / Filter by Type
│ & Analytics Manager      │
└──────────┬───────────────┘
           │
           ├────────────────────────────┐
           ▼                            ▼
[Exported CSV History]       [Formatted Diagnostic Report (.txt)]
```
