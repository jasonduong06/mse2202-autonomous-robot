# 1.2 Product Design Requirements and Constraints

## Functional Requirements
The over-arking functions that the robots must have.

1. **Object Sorting:**  
The robot must be capable of identifying, sorting, and collecting small objects based on colour, while only retaining those classified as “valuable.”

2. **Fully Autonomous Operation:**  
The robot must operate without human intervention after starting. All navigation, collection, sorting, and return must rely on onboard sensors.

3. **Base-Station Return:**  
The robot must depart from and return to a defined home base. All valuable objects must be deposited into the collection container at the end of the run.

---

## Design Requirements
Requirements that the system must have integrated into the design.

1. **Autonomous Operation:**  
After being activated, the robotic system must complete all its tasks without human intervention. No external guides such as floor markings or wire can be used to assist the robot.

2. **Microcontroller:**  
An ESP32-based microcontroller must be used to implement the robot’s control.

3. **Object Collection:**  
The robot must find and gather valuable objects from the ground.

4. **Identification of Valuable Objects:**  
Objects deemed valuable by their colour must be delivered to the base destination whereas non-valuable objects are discarded.

5. **Return to Base Functionality:**  
The robot starts at the base and must return to this same location to deposit the valuable objects.

6. **Time:**  
The collection and sorting task must be completed within 120 seconds.

7. **Size:**  
The entire robotic system must fit inside the Mechatronics Lab locker assigned to the group.

8. **Collision Avoidance:**  
The robot should not crash or be deterred by objects in its path.

9. **Operation within Area of Interest:**
The robot must be able to traverse a flat rectangular area of about 15 m2.
 
10 **Power:**
The robotic system must be able to carry its own power supply and remain connected for the duration of the task. The batteries provided in the MSE2202 Lab Kit must be used in an unmodified form.

11. **Budget:** It is encouraged that parts from the MSE2202 Lab Kit are used, and an additional $20 is provided for electronic components

--- 

## Conceptual Design Constraints
Constraints used to guide development of concepts and evaluate them.

1. **Weight Distribution:** A lighter robotic system results in a higher possible score during the demonstration. Also, the weight should be evenly distributed throughout the chassis to prevent tipping and reduce friction. 

2. **Portability:** The robotic system must be easy to transport as the project will be worked on outside of the provided lab hours.

3. **Durability:** The robotic system must not sustain any damage that would affect its performance.

4. **Cost Efficiency:** Aside from the $20 project budget for electronic components, only equipment provided in the MSE2202 Lab Kits are available to use.

5. **Ease of Implementation:** It is imperative that the selected design can be developed, manufactured, assembled, and tested thoroughly so that each iteration can be improved upon as quickly as possible.

6. **Reliability:** The robot must consistently perform navigation, collection, and sorting tasks.

7. **Ease of Repair:** Components should be easily accessible for quick fixes.

8. **Scalability and Modularity:** Subsystems should be easily modified without redesigning the entire robot.
