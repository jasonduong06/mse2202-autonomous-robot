## Failure Mode and Effects Analysis (FMEA)

| Process Step | Potential Failure Mode | Potential Effect on Customer Because of Defect | SEV | Potential Causes | OCC | Current Process Controls | DET | RPN | Actions Recommended |
|-------------|------------------------|-----------------------------------------------|-----|------------------|-----|--------------------------|-----|-----|---------------------|
| What is the component, function, or feature? | In what ways can the Component, Subsystem, or System potentially fail to meet the Design Intent? | What is the impact on the key output variables if the failure mode is not prevented or corrected? | How severe is the effect? | What causes the step to go wrong? How could the failure mode occur? | How frequent is the cause likely to occur? | What are the existing controls that prevent the failure mode from occurring? | How probable is detection of the cause? | Risk Priority Number used to rank concerns | What actions are recommended to reduce occurrence or improve detection? High RPNs or severity of 9 or 10 should be prioritized. |
| Driving | Bot ceases movement | Bot is unable to complete route | 9 | Wheels disconnect from motor | 2 | Wheels securely fitted to motor shaft | 9 | 162 | Visually inspect wheel connections before use |
| Driving | Robot struggles to turn | Reduced maneuverability and inefficient movement | 7 | Excess friction from wheel setup | 3 | Caster wheel implemented | 8 | 168 | Reduce friction and optimize wheel placement |
| Collection (Plow) | Objects not collected properly | Reduced number of collected objects | 6 | Incorrect plow angle or size | 4 | Angled plow design | 7 | 168 | Adjust plow geometry and size |
| Collection (Plow) | Objects get stuck in front | Robot slows or stops | 7 | Poor plow shape causing blockage | 3 | Iterative plow redesign | 7 | 147 | Smooth edges and refine plow design |
| Elevator | Platform tilts during lifting | Objects fall off platform | 8 | Lack of guidance stability | 3 | Guide wires added to corners | 8 | 192 | Reinforce guides and improve stability |
| Elevator | Objects do not slide off platform | Sorting process delayed | 7 | High friction surface | 4 | Added slope and agitation motion | 7 | 196 | Improve surface material and slope angle |
| Sorting (Funnel) | Funnel becomes jammed | Sorting system stops working | 8 | Erasers enter sideways | 5 | Funnel redesign and flipper added | 6 | 240 | Optimize funnel curvature and entry design |
| Sorting (Funnel) | Multiple objects enter at once | Sorting errors and blockage | 7 | No flow control at entry | 4 | Reduced opening size and cup added | 6 | 168 | Further restrict entry to one object at a time |
| Sorting (Sensor) | Colour sensor misreads object | Object sorted incorrectly | 6 | Poor lighting conditions | 3 | Sensor calibrated before use | 7 | 126 | Recalibrate sensor under test conditions |
| Electronics | Wire disconnection | System malfunctions | 7 | Loose or stressed wiring | 2 | Wires secured and routed safely | 8 | 112 | Inspect wiring before operation |
| Power | Battery failure | Robot stops completely | 9 | Battery not fully charged | 3 | Battery checked before use | 8 | 216 | Ensure battery is fully charged before runs |

## Subsystem Design Analysis and Recommendations

The subsystem design analysis evaluates the performance of each major component of the system, identifies limitations observed during testing, and proposes improvements to enhance overall reliability and functionality.

| System | Current Design Features | Performance | Limitations | Proposed Improvements |
|--------|------------------------|------------|-------------|------------------------|
| **Overall Design** | Modular system with separate scavenger and stationary sorting unit; plow-based collection; funnel + wheel sorting | Reliable and simple system; clear separation of tasks improves testing and debugging | Alignment between subsystems (deposit → funnel → wheel) is sensitive; reliance on manual assembly tolerances | Add fixed mounting points and guides for alignment; use screws/fasteners instead of tape for structural stability |
| **Drive System** | Front-wheel drive with rear caster; encoder-based path following; lightweight chassis | Consistent navigation along predefined path; good traction during forward motion | Turning can be inconsistent; weight distribution affects stability; friction from plow impacts motion | Improve weight distribution; tune motor speeds for smoother turning; add support wheels if needed |
| **Pick-up System (Plow)** | Angled front plow guides objects inward; passive collection into storage area | Simple and effective for collecting multiple objects; low mechanical complexity | Objects can get stuck at edges; friction from plow reduces movement efficiency | Refine plow geometry (rounded/trimmed edges); reduce surface friction; adjust angle for smoother intake |
| **Elevator System** | Lifting platform transfers objects from robot to funnel; driven by motor mechanism | Enables vertical transfer to sorting system; integrates well with base station | Objects may misalign or not fully transfer; platform stability can affect consistency | Add side walls or guides; improve platform alignment; ensure consistent lifting height |
| **Sorting System** | Curved funnel, funnel flipper (DC motor), sorting wheel (stepper motor), colour sensor, servo gate | Controlled one-at-a-time sorting; improved flow with funnel and agitation; accurate classification | Jamming still possible with multiple objects; precise alignment required for sensor and wheel; timing between components critical | Optimize funnel shape and spacing; tune flipper timing; calibrate colour sensor; improve synchronization between stepper and servo |
| **Electronics & Control** | Stepper motor (wheel), DC motor (flipper), servo motor (sorting gate), colour sensor | Enables automated sorting and actuation; flexible control through programming | Wiring complexity; potential interference or loose connections; timing coordination required | Improve wire management; secure connections; refine control logic and timing for smoother operation |


## Major Adjustments Made Across Prototyping

### Scavenging System

| Points of Failure | Solution |
|------------------|---------|
| The two rear wheels created excessive friction, making it difficult for the robot to turn. | The rear wheels were replaced with a single caster wheel to reduce friction and improve maneuverability. |
| The chassis was too long, making the robot difficult to drive and reducing overall power efficiency. | The chassis length was reduced to decrease load and improve motor performance and maneuverability. |
| The rotating harvester mechanism was too complex to implement reliably within the given time constraints. | The harvester was replaced with a simpler plow-based collection system to improve feasibility and ease of implementation. |

---

### Elevator System

| Points of Failure | Solution |
|------------------|---------|
| The elevator could not travel upwards straight without tilting/rotating. | Four copper wires were added to each corner to guide the platform in a straight trajectory. |
| There was too much friction between the eraser and the platform for the erasers to fall off the ramp. | A cardboard platform was added to create a steep slope, and the elevator was programmed to agitate to shake off the erasers. |

---

### Sorting System

| Points of Failure | Solution |
|------------------|---------|
| Colour sensor having trouble reading due to refraction | Black filament was used to improve sensor accuracy. |

---

## Test Runs

Testing was conducted to evaluate the robot’s performance in navigation, object collection, transfer, and sorting accuracy in relation to the project objectives. Each subsystem was tested over multiple trials to assess consistency and reliability.

### Drive System
The drive system was tested by running the robot along a predetermined path over 5 trials. The robot successfully completed the path in 4 out of 5 trials, with minor deviations caused by wheel slip and surface friction. After adjustments to motor speed and weight distribution, path accuracy improved and deviations were reduced.

### Pick-Up System (Plow)
The pick-up system was evaluated by placing 10 erasers in the test area for each run. On average, the plow successfully collected 2-3 out of 5 objects per trial, demonstrating strong collection capability. Occasional losses occurred when objects became caught near the edges of the plow.

### Elevator and Funnel System
The elevator and funnel system were tested for transfer efficiency. Across 5 trials, almost all collected objects were successfully transferred into the sorting system. In some cases, a single eraser would become stuck along the side of the funnel, preventing full transfer. The addition of the curved funnel and flipper significantly reduced jamming, although minor blockages still occurred when multiple objects entered simultaneously.

### Sorting System
The sorting system was tested using a mix of green and non-green erasers. The colour sensor and servo mechanism were able to sort objects with generally high accuracy under consistent lighting conditions. Misclassifications occurred primarily when lighting conditions varied or when objects were not properly aligned in the sorting wheel.

### Jamming Test
A jamming test was conducted by introducing multiple objects into the funnel at once. Jamming occurred in 2 out of 5 trials, but was resolved quickly due to the flipper mechanism, demonstrating improved robustness compared to earlier iterations.

### Summary
Overall, the system met the primary project objectives, demonstrating reliable navigation, effective object collection, and accurate sorting. Limitations were identified in areas such as object alignment, occasional jamming, and sensitivity of the colour sensor to lighting conditions. These results informed iterative improvements and increased confidence in the final system performance.

---

## Reflective Comments on the Results

The design analysis focused on evaluating the performance of each subsystem and identifying key failure modes through structured methods such as FMEA and subsystem design analysis. Based on the testing results and observed system behavior, a high level of confidence has been achieved in the overall functionality and reliability of the final design.

The drive system improvements, particularly the transition from dual rear wheels to a single caster wheel, significantly enhanced maneuverability and reduced friction. This change addressed one of the highest-impact issues identified during testing and allowed the robot to turn freely in any direction.

The plow-based collection system proved to be an effective and reliable solution. Compared to the previously considered harvesting mechanism, the plow offered a simpler and more feasible approach while maintaining strong collection performance. Minor limitations such as object blockage and friction were observed, but these are expected to be fixed through improvement in CAD designs.

The elevator and funnel systems highlighted critical challenges related to alignment and object flow. Early failures such as platform tilting and object jamming were successfully addressed through the addition of guide wires, slope adjustments, and an agitation mechanism. The implementation of the funnel flipper significantly reduced jamming.

The sorting system demonstrated accurate object classification and reliable one-at-a-time processing using the rotating wheel and colour sensor. However, confidence in the mechanical stability of the system, particularly in component alignment and synchronization between the stepper motor and servo, was initially limited. Frequent mechanical design improvements improved reliability and performance consistency.

From an electronics and control perspective, issues such as wire disconnection and component interference were identified. Improvements in wire management and secure mounting reduced these risks and contributed to more stable operation.

Overall, the iterative design process allowed for the identification and resolution of key failure modes, leading to a system that meets the primary project objectives with a strong degree of confidence.
