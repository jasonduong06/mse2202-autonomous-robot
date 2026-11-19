# Autonomous Scavenger & Sorting Robot

Team project for MSE2202 (Mechatronic Systems Engineering), Western University.

An autonomous robot system that collects erasers from an arena and sorts out the green ones using colour sensing.

## How it works

- **Scavenger robot:** rectangular chassis with front-wheel drive and a rear caster. It follows a predetermined path using encoder feedback and collects objects with an angled front plow.
- **Stationary sorting station:** an elevator lifts the collected objects into a curved funnel. A motor-driven flipper prevents jamming. A stepper-driven sorting wheel indexes each eraser under a colour sensor, and a servo diverts the green ones into a bucket.

**Hardware and tools:** ESP32, DC gear motors, stepper motor, servo, colour sensor, encoders, SolidWorks/CAD, 3D printing, Arduino/C++

## My contribution

- [Designed the chassis in CAD, wrote the sorting firmware, built the elevator]

## Project documentation

- **[Final design, iterations and decisions](Phase%203%20Product%20Design/Product%20Generation/product_generation.md)**
- [CAD](Phase%203%20Product%20Design/Product%20Generation/CAD)
- [Circuit diagrams](Phase%203%20Product%20Design/Product%20Generation/Circuit%20Diagrams%20for%20Final%20Parts)
- [Software](Phase%203%20Product%20Design/Product%20Generation/Software)
- [Bill of materials](Phase%203%20Product%20Design/Product%20Generation/Bill%20of%20Materials.xlsx)

## Design highlights

- Dropped a complex harvester intake for a simpler, more reliable plow after two CAD iterations.
- Worked around the 3D printer's 25 cm size limit by redesigning the elevator in cardboard with wooden supports.
- Fixed servo jamming in the sorting wheel by redesigning the stand across three iterations.

*Developed with teammates in an academic course project.*
