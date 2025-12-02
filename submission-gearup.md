## Project 5: Realtime

The project handout can be found [here](https://cs1230.graphics/projects/realtime/1).

# Project 6: Gear Up — Testing

## Gameplay + Physics + Integration Tests

| Feature | Instructions | Expected Output | Your Output |
|--------|--------------|----------------|------------|
| Physics – Translation Stability | Press **W** for ~8 seconds | Snake moves smoothly with consistent speed and no jitter | Video below |
| Physics – Collision / Death Animation | Steer snake into a wall or a tree | Snake squashes horizontally and respawns after ~0.6s | Video below |
| Growth from Food | Eat 3 food cubes in a row | New body segments spawn one-by-one and follow smoothly | Video below |
| Door Opening Timing | Wait ~20s from start | Front door opens smoothly revealing the full path | *(Insert video below)* |
| Camera Follow Mode | Press **C** to toggle follow camera | Camera locks behind snake head with smooth motion | Video below |
| L-System Trees – Randomness | Start scene → observe 3 trees | Same overall structure but slightly different branches | *(Insert screenshot in this column)* |
| L-System Path Strip | Walk halfway down the path | Even spacing of trees along edges | *(Insert screenshot in this column)* |
| Full Gameplay Integration Test | Move around, eat food, die, respawn | All mechanics function together with stable framerate | Video below |

---

### Physics Videos

> Place your recorded outputs here like Project 5 did:

https://github.com/user-attachments/assets/INSERT_TRANSLATION_VIDEO  
https://github.com/user-attachments/assets/INSERT_DEATH_VIDEO  
https://github.com/user-attachments/assets/INSERT_GROWTH_VIDEO  
https://github.com/user-attachments/assets/INSERT_DOOR_VIDEO  
https://github.com/user-attachments/assets/INSERT_CAMERA_VIDEO  
https://github.com/user-attachments/assets/INSERT_FULLGAME_VIDEO  

---

## Visual Feature Tests 

> Load the provided JSON test scenes and save rendered images.  
> Drag and drop the saved images into the “Your Output” column.

| Input Scene / Setting | How to Produce Output | Expected Output | Your Output |
|---|---|---|---|
| Normal Mapping ON – Brick Cube | Load `brick_normalmap_test.json` → Save `brick_nm_on.png` | Bricks show visible micro-surface detail under lighting | ![Place brick_nm_on.png in student_outputs](student_outputs/brick_nm_on.png) |
| Normal Mapping OFF – Brick Cube | Toggle **N** → Save `brick_nm_off.png` | Flat shading → grooves disappear | ![Place brick_nm_off.png in student_outputs](student_outputs/brick_nm_off.png) |
| Grass Bump Mapping | Load `grass_bump_test.json` → Save `grass_bump.png` | Grass shows subtle height changes (not flat) | ![Place grass_bump.png in student_outputs](student_outputs/grass_bump.png) |
| Normal Mapping Light Proof | Rotate camera around cube → Save `brick_shading.png` | Highlights shift per-pixel across brick | ![Place brick_shading.png in student_outputs](student_outputs/brick_shading.png) |
| Tree Shading / Geometry Test | Load `lsystem_only.json` → Save `lsystem_test.png` | Procedural branches visible + shadows correct | ![Place lsystem_test.png in student_outputs](student_outputs/lsystem_test.png) |

---


# EVERYTHING BELOW IS PROJECT 5

#### Camera Translation



##### Expected Output

https://github.com/BrownCSCI1230/projects_realtime_template/assets/45575415/710ff8b4-6db4-445b-811d-f6c838741e67

##### Your Output



https://github.com/user-attachments/assets/e4ba8c11-4078-4b26-aacc-592b53f83ee8



<!---
Paste your output on top of this comment!
-->

#### Camera Rotation

_Instructions:_ Load `chess.json`. Take a look around!

##### Expected Output

https://github.com/BrownCSCI1230/projects_realtime_template/assets/45575415/a14f4d32-88ee-4f5f-9843-74dd5c89b9dd

##### Your Output



https://github.com/user-attachments/assets/e8c9d857-3c10-4459-92c2-9cc513667cbf


<!---
Paste your output on top of this comment!
-->

### Design Choices
- Pipeline Structure & Code Reuse: I built on top of the existing architecture from previous labs. The tessellation code for cube, cone, cylinder, and sphere, as well as the scene parsing logic, were reused and integrated into the realtime OpenGL pipeline.
- Shaders & Phong Lighing: Lighting was computed fully in the fragment shader using our the Phong implementation. Ambient, diffuse, and specular terms were handled manually and recomputed per light. 
- Lights: Instead of using a singular specific light type, I implemented a generalized upload system that supports eight lights of mixed types (directional, point, spot). This made the shader flexible and able to match the scene files and test cases.
- Camera implementation: The projection matrix follows class formulas, and camera movement is based on recreated basis vectors. For rotation I used Rodrigues-style rotation and vector math.


## Collaboration/References
- Relied heavily on lectures
- Used code from labs 4, 8, 10 in this project
- I used ChatGPT (OpenAI, GPT-5, 2025) as a debugging/explanation tool for this project

    Specifically, ChatGPT I used it to:
    
    - Help translate shader math and camera rotation formulas from lecture into GLSL/C++ code. 
    - Debug VAO/VBO mistmatches
    - understand and design pseudocode for camera movement and rotations
    - pass in code from other labs to this project correctly
    - Debug shapes position (frag file affected). For some reason my shapes were appearing at a weird angle.
    
    Types of Prompts:
    - Explain shader error why is nothing appearing being colored in 
    - Help me implement camera movement
    - lets test the setup with one shape first 
    - (Once i got stuck moving from hard coded shape to all shaoes) Help me figure out why my other shapes aren't appearing bt my cylinder is.
    
    Code areas influenced:
    - Shader files more specifically diffuse/phong part of frag. 
    - camera movement within Realtime.cpp
    - help with rotation 
    

### Known Bugs
N/A

### Extra Credit
N/A
