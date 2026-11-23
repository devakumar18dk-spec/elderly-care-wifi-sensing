# ARDUINO UNO Q – TECHNICAL ROLE, WORKFLOW AND IMPLEMENTATION

## 1. Overview

In the proposed elderly-care Wi-Fi sensing system, the Arduino UNO Q acts as the central monitoring, decision-making and room-recalibration controller.

The system uses three ESP32-S3 nodes (Red, Green and Blue) to collect Wi-Fi Channel State Information (CSI) from the surrounding environment. The existing Python-based Wi-Fi Sensing Kit processes the CSI data, extracts features and uses a K-Nearest Neighbours (KNN) model to recognise patient activities.

The Arduino UNO Q operates as a supervisory unit above this existing activity-recognition pipeline. Its responsibility is to monitor whether the room environment remains consistent with the conditions under which the system was initially calibrated.

If the UNO Q detects a persistent change in the CSI environment, it investigates the possible cause, requests confirmation from the caretaker through the dashboard and manages the room-recalibration process.

The primary objective is to maintain reliable activity recognition even when the physical environment changes over time.

## 2. Hardware and Software Responsibilities

The Arduino UNO Q contains two processing units:

### A. Qualcomm QRB2210 – Linux Processing Unit

This is the main processing unit responsible for the UNO Q's supervisory functions.

Responsibilities:

- Runs Debian Linux and Python applications.
- Receives processed CSI information from the existing Wi-Fi Sensing Kit server.
- Performs environmental-change analysis.
- Maintains room-environment baselines.
- Stores and manages different baseline versions.
- Monitors sensing-link health and KNN prediction reliability.
- Manages caretaker alerts and verification requests.
- Controls the room-recalibration workflow.
- Communicates with the existing dashboard.

### B. STM32U585 – Microcontroller Unit

The STM32U585 handles optional hardware-level and real-time control functions.

Responsibilities:

- Controls status indicators and buzzers.
- Implements hardware watchdog functions.
- Monitors simple peripheral devices.
- Handles basic real-time control tasks.
- Communicates with the Linux processor through the Arduino Bridge.

For the initial implementation, the QRB2210 Linux processor will handle most of the work. The STM32U585 can be integrated later when additional hardware-control requirements arise.

## 3. Overall System Architecture

The system consists of four main layers.

### Layer 1: Wi-Fi CSI Sensing

Three ESP32-S3 nodes are distributed around the room:

- Red ESP32-S3
- Green ESP32-S3
- Blue ESP32-S3

These nodes collect CSI measurements from router-to-node and node-to-node wireless links.

The collected measurements are transmitted to the existing Python server over the local network.

### Layer 2: Existing Wi-Fi Sensing Kit and KNN Pipeline

The existing Python server performs:

1. UDP packet reception.
2. CSI packet parsing.
3. Link identification and management.
4. CSI feature extraction.
5. Feature-window generation.
6. KNN-based activity classification.
7. Activity prediction and dashboard data generation.

The existing KNN model remains responsible for recognising activities such as:

- Nobody.
- Standing.
- Sitting.
- Walking.
- Lying down.
- Getting into bed.
- Getting out of bed.

These are proposed elderly-care activity classes; the final classes depend on the available labelled dataset and actual model implementation.

### Layer 3: Arduino UNO Q – Environmental Monitoring and Supervision

The UNO Q receives relevant information from the existing Python server.

It performs four major functions:

1. Room-environment baseline management.
2. Environmental-change detection.
3. KNN reliability monitoring.
4. Caretaker-supervised recalibration.

### Layer 4: Caretaker Dashboard

The dashboard provides:

- Patient activity information.
- Environmental-change alerts.
- Sensor and link health information.
- Caretaker verification requests.
- Room-baseline version history.
- Recalibration status.
- Activity-model validation status.

The caretaker remains involved in confirming physical room changes and approving supervised updates.

## 4. Data Communication Between the Existing Server and UNO Q

The UNO Q does not need to independently process every raw CSI packet.

Instead, the existing Python server can send compact feature-window information and system-status data to the UNO Q.

For the initial implementation, HTTP REST communication over the local network is recommended.

The information sent to the UNO Q should include:

- Timestamp.
- Room identification.
- Active CSI link identifiers.
- CSI feature vectors.
- Link packet rates.
- Link availability.
- Wi-Fi channel information.
- Current KNN prediction.
- KNN confidence or prediction-stability information.
- Activity-model version.

For example, the existing server can send the following type of information:

{
  "timestamp": "2026-11-23T13:05:21",
  "room_id": "eldercare_room_01",
  "links": {
    "1>2": {
      "packet_rate": 22.4,
      "active": true,
      "features": ["..."]
    },
    "2>3": {
      "packet_rate": 21.8,
      "active": true,
      "features": ["..."]
    }
  },
  "knn": {
    "prediction": "walking",
    "confidence": 0.82,
    "model_version": "knn_v1"
  }
}

This is an illustrative communication format. The actual interface will be defined after inspecting the existing Python server and its feature-extraction functions.

The UNO Q should not become an additional CSI sensing node. The three ESP32-S3 boards remain responsible for wireless CSI sensing.

## 5. Room-Environment Baseline Management

The first major responsibility of the UNO Q is to understand the normal RF environment of the room.

During initial installation, the system records CSI measurements under a known and stable room arrangement.

The initial arrangement includes:

- Known furniture positions.
- Known bed position.
- Fixed router location.
- Fixed ESP32-S3 node positions.
- Stable Wi-Fi configuration.
- Healthy CSI link communication.

The UNO Q stores this information as the initial room baseline, called room_v1.

For every available CSI link, the baseline can contain:

- Mean feature values.
- Feature standard deviations.
- Feature distributions.
- Normal packet-rate ranges.
- Normal feature variation over time.
- Link availability information.

The baseline acts as the reference against which future measurements are compared.

## 6. Environmental-Change Detection

During normal operation, the existing Python server continuously sends new CSI feature windows to the UNO Q.

The UNO Q compares the current measurements with the stored room baseline.

A simple standardised deviation calculation can be used:

z_j = |x_j - μ_j| / (σ_j + ε)

Where:

- x_j represents the current value of a feature.
- μ_j represents the baseline mean.
- σ_j represents the baseline standard deviation.
- ε is a small value used to avoid division by zero.

A large deviation indicates that the current CSI measurement differs from the stored room condition.

However, a single deviation should not immediately generate an environmental-change alert because normal human movement can also affect CSI measurements.

Therefore, the UNO Q should examine:

1. Whether the deviation persists across multiple feature windows.
2. Whether multiple independent CSI links show related changes.
3. Whether the room is currently occupied or experiencing movement.
4. Whether packet reception is stable.
5. Whether the router channel or sensor configuration has changed.

The initial implementation can use rolling feature statistics and a change-detection algorithm such as CUSUM.

The system should distinguish between:

- Temporary CSI fluctuations.
- Normal human movement.
- Persistent environmental changes.
- ESP32 sensor displacement.
- Router or Wi-Fi configuration changes.
- Network or packet-reception problems.

The UNO Q should identify a possible environmental change rather than automatically claiming that a particular piece of furniture has moved.

## 7. Environmental-Change Decision Logic

The UNO Q follows a decision process before generating an alert.

Observation: One temporary CSI deviation.

Decision: Continue monitoring.

Observation: Repeated deviations during human movement.

Decision: Wait until the room becomes stable.

Observation: Persistent deviations across multiple healthy CSI links.

Decision: Flag a possible environmental change.

Observation: Packet loss or missing ESP32 nodes.

Decision: Generate a sensing-network warning.

Observation: Router channel changes.

Decision: Generate a Wi-Fi configuration warning.

Observation: Persistent CSI shift after the room becomes stable.

Decision: Request caretaker verification through the dashboard.

The detection thresholds should be determined using actual CSI measurements from the installed system.

## 8. Caretaker Verification Workflow

When the UNO Q identifies a persistent environmental change, it generates an alert on the caretaker dashboard.

Example alert:

"Possible room-environment change detected. The CSI measurements differ from the existing room baseline. Please verify whether furniture, bed position, router position or sensing-device placement has changed."

The caretaker can select one of the following options:

- Furniture was moved.
- Bed position changed.
- ESP32 sensor position changed.
- Router or Wi-Fi settings changed.
- No physical change observed.
- Unknown or not sure.

The caretaker can also confirm whether the new arrangement is stable.

The UNO Q uses this response to determine the next system action.

Caretaker confirmation is important because CSI changes alone cannot reliably identify the exact physical cause.

## 9. Room-Recalibration Workflow

When the caretaker confirms that furniture has been moved and the new arrangement is stable, the UNO Q initiates controlled room recalibration.

The recalibration process follows these steps:

### Step 1: Preserve the Existing Configuration

Before recalibration, the UNO Q saves:

- Previous room baseline.
- Existing KNN model.
- Existing training dataset.
- Feature-normalisation parameters.
- Active CSI link configuration.
- Previous room version.
- Caretaker confirmation and timestamp.

This ensures that the previous working configuration remains available.

### Step 2: Check Sensing-Network Health

The UNO Q checks whether:

- Red, Green and Blue ESP32-S3 nodes are communicating.
- Expected CSI links are available.
- Packet rates are stable.
- No unexpected router-channel change occurred.
- Sensor positions remain unchanged.

If a sensor has been moved, the system should treat it as a separate sensor-installation recalibration.

### Step 3: Wait for Stable Room Conditions

The system waits until furniture movement and significant human movement have stopped.

This prevents temporary human activity from being included in the new room baseline.

### Step 4: Collect New CSI Measurements

The existing Python server continues extracting CSI features.

The UNO Q records new measurements across the available healthy links.

### Step 5: Validate the New Baseline

The UNO Q checks:

- Whether feature values are sufficiently stable.
- Whether measurements are available across the expected links.
- Whether packet rates remain healthy.
- Whether the new measurements are consistent across multiple windows and recording sessions.

If the baseline is unstable, the system continues collecting measurements rather than accepting an unreliable baseline.

### Step 6: Create a New Room Version

Once validation succeeds, the UNO Q saves the new baseline as room_v2.

The previous room_v1 remains archived.

### Step 7: Validate the Existing KNN Model

The existing KNN model is tested using caretaker-labelled activity samples collected under the new room arrangement.

If the model continues performing adequately, it can remain unchanged.

If its performance degrades, additional labelled samples can be collected and added to the training dataset through a controlled update process.

The KNN model should not automatically retrain using its own unverified predictions.

## 10. KNN Reliability Monitoring

The UNO Q does not replace or duplicate the existing KNN classifier.

The existing Python KNN pipeline remains responsible for activity recognition.

The UNO Q supervises the classifier by monitoring:

- Prediction confidence, if available.
- Prediction stability.
- Changes in the input feature distribution.
- Missing or inactive CSI links.
- Model-version information.
- Validation performance after recalibration.

For example, if the patient is normally classified as walking but the model starts producing inconsistent predictions after furniture movement, the UNO Q can identify that the sensing environment has changed and that the existing model requires validation.

It should not automatically conclude that the patient's behaviour has changed.

## 11. Patient Daily-Routine Monitoring

In addition to environmental monitoring, the UNO Q can maintain a separate patient-routine profile.

The routine profile can be developed from the activity predictions produced by the existing KNN pipeline.

It can monitor:

- Activity frequency.
- Activity duration.
- Activity transitions.
- Usual activity periods.
- Changes in daily activity patterns.

For example, the system may learn that a patient generally gets out of bed around 7 AM and spends more time walking in the morning.

If the usual activity pattern changes significantly, the UNO Q can generate a caretaker notification.

These notifications should be treated as observations requiring attention rather than medical diagnoses.

The historical routine should be preserved so that gradual changes in mobility are not silently learned as normal.

## 12. Room-Baseline Version Management

The UNO Q should maintain different versions of the room environment.

Example:

room_v1 – Initial room installation.

room_v2 – Furniture movement confirmed by caretaker.

room_v3 – Future room-environment change.

Each version should contain:

- Room version identifier.
- Previous version identifier.
- Change reason.
- Caretaker confirmation.
- Creation timestamp.
- Available CSI links.
- Stored feature statistics.
- Baseline validation status.
- Active or archived status.

SQLite is recommended for the initial implementation because it is lightweight and suitable for local storage on the UNO Q.

Previous room versions should never be overwritten when a new baseline is created.

## 13. Proposed UNO Q Software Structure

The following is the proposed software organisation:

uno_q_system/

    main.py

    monitoring/
        link_health.py
        environment_monitor.py
        change_detector.py
        knn_monitor.py

    calibration/
        baseline_collector.py
        baseline_validator.py
        version_manager.py

    caretaker/
        alert_manager.py
        verification_handler.py

    database/
        room_baselines.db
        events.db

    config/
        room_config.json

    logs/
        system.log

### Module Responsibilities

main.py:
Coordinates the UNO Q monitoring services.

link_health.py:
Monitors CSI link availability, packet rates and sensing-network health.

environment_monitor.py:
Compares current CSI feature windows against the active room baseline.

change_detector.py:
Identifies persistent deviations in the room's CSI environment.

knn_monitor.py:
Monitors activity predictions and classifier reliability.

baseline_collector.py:
Collects CSI features for a new room baseline.

baseline_validator.py:
Checks whether the newly collected baseline is stable and usable.

version_manager.py:
Creates, stores, activates and archives room-baseline versions.

alert_manager.py:
Generates environmental-change alerts for the caretaker dashboard.

verification_handler.py:
Receives caretaker responses and initiates the appropriate workflow.

room_baselines.db:
Stores the room-environment baseline versions.

events.db:
Stores system events, alerts, caretaker confirmations and recalibration history.

## 14. Dashboard Integration

The existing Wi-Fi Sensing Kit dashboard should be extended instead of creating an entirely separate application.

A new section called "Environment and System Health" can be introduced.

It should display:

- Current room-baseline version.
- Active CSI link health.
- Environmental stability status.
- KNN model version.
- Latest environmental-change alerts.
- Caretaker verification requests.
- Recalibration progress.
- Previous room versions.
- Activity-model validation status.

Example dashboard information:

Room Baseline: room_v2

Activity Model: knn_v1

CSI Link Health: Healthy

Environment Status: Stable

Latest Event: Furniture movement confirmed

Recalibration Status: Completed

KNN Validation: Pending

This allows the caretaker to understand the system's condition without needing to inspect technical logs.

## 15. Final Deployment Architecture

The recommended final deployment is:

Three ESP32-S3 Nodes:
Responsible for distributed Wi-Fi CSI sensing.

Arduino UNO Q – Qualcomm QRB2210:
Runs Debian Linux, Python applications, the existing Wi-Fi Sensing Kit server, KNN pipeline, environmental monitoring and recalibration manager.

Arduino UNO Q – STM32U585:
Handles optional hardware-level control functions such as status indicators, buzzers and watchdog operations.

UNO Q Local Database:
Stores room-baseline versions, event history and recalibration information.

Caretaker Dashboard:
Hosted by the UNO Q and accessed through a laptop, tablet or mobile browser.

During initial development, the existing Python server can remain on the laptop while the UNO Q receives information over the local network. After the communication interface and monitoring functions are validated, the Python server and KNN pipeline can be migrated to the UNO Q.

## 16. Implementation Order

The implementation should proceed in the following order:

Stage 1 – Data Interface:
Understand the existing Python server and establish communication between the server and UNO Q.

Stage 2 – Baseline Manager:
Implement CSI baseline collection, storage, retrieval and version management.

Stage 3 – Environmental-Change Detection:
Implement persistent CSI deviation detection and sensing-network health checks.

Stage 4 – Caretaker Workflow:
Integrate environmental-change alerts, caretaker confirmation and recalibration requests into the dashboard.

Stage 5 – Recalibration:
Implement new room-baseline collection, validation and version activation.

Stage 6 – KNN Supervision:
Monitor classifier reliability and validate the existing model after environmental changes.

Stage 7 – Routine Monitoring:
Develop patient-specific activity-pattern monitoring using the existing KNN predictions.

## 17. Core Operating Principle

The Arduino UNO Q acts as the supervisor of the sensing environment, while the existing Wi-Fi Sensing Kit and KNN pipeline remain responsible for patient activity recognition.

The three ESP32-S3 nodes observe the room through Wi-Fi CSI.

The existing Python pipeline converts CSI measurements into activity predictions.

The UNO Q monitors the environment in which those predictions are generated.

When it detects a persistent environmental change, it asks the caretaker to verify the situation and manages controlled recalibration.

The new room baseline is stored as a separate version, and the existing KNN model is validated before any activity-model update is approved.

This separation allows the system to continue recognising patient activities while independently monitoring environmental changes that could affect sensing reliability.

**In simple terms: The ESP32-S3 nodes sense the room, the KNN model recognises patient activities, the Arduino UNO Q judges the condition of the sensing environment, and the caretaker supervises important changes.**