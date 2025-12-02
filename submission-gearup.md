## Project 5: Realtime

The project handout can be found [here](https://cs1230.graphics/projects/realtime/1).

# Project 6: Gear Up — Testing

## Gameplay + Physics + Integration Tests

| Feature | How to produce | Expected Output | Your Output |
|--------|--------------|----------------|------------|
| 1. Physics – Movement (Acceleration and Velocity) | Press **W** for ~5 seconds | Snake moves smoothly with increasing speed due to acceleration | Video below |
| 2. Physics – Collision / Death Animation | Steer snake into a wall or a tree | Snake squashes horizontally and respawns | Video below |
| 3. Growth from Food | Eat 3 food cubes in a row | New body segments spawn one-by-one and follow smoothly | Video below |
| 4. Camera Follow | Move the snake | Camera locks behind snake head with smooth motion | Video below |
| 5. L-System Trees – Randomness | Start scene → observe 3 trees | Same overall structure but slightly different branches | <img width="148" height="218" alt="lsystem_random" src="https://github.com/user-attachments/assets/90770b37-f4bf-4dfc-9ec3-d374a06b9a67" /> |
| 6. L-System Path Strip | Walk halfway down the path | Even spacing of trees along edges | <img width="1094" height="795" alt="lsystem_path" src="https://github.com/user-attachments/assets/3b423ebb-0838-43c3-9679-71a1a05324e6" /> |
| 7. Physics - Full Gameplay Integration Test | Move around, eat food, die, respawn | All mechanics function together with stable framerate | Video below |

---

### Physics Videos

#### 1. Physics - Movement 
https://github.com/user-attachments/assets/360aca19-1c20-4719-a4ce-d1eb2e853634


#### 2. Physics - Collision 
https://github.com/user-attachments/assets/22213884-ddb0-42cb-9b4a-1b3f49e64848

#### 3. Growth from Food
https://github.com/user-attachments/assets/0b946ba0-0914-44c7-b65c-e011f19a5220

#### 4. Camera Follow
https://github.com/user-attachments/assets/71b0febc-b343-4791-9228-e621d3828025

#### 7. Physics - Full Gameplay
https://github.com/user-attachments/assets/cfc5ec84-2907-487b-9be0-92a68ab7454e



---

## Visual Feature Tests 

> Load the provided JSON test scenes and save rendered images.  
> Drag and drop the saved images into the “Your Output” column.

| Input Scene / Setting | How to Produce Output | Expected Output | Your Output |
|---|---|---|---|
| Normal Mapping ON – Brick Cube | Load `brick_normalmap_test.json` → Save `brick_nm_on.png` | Bricks show visible micro-surface detail under lighting | |
| Normal Mapping in Game – Brick Path | N/A | N/A | |
| Grass Bump Mapping | Load `grass_bump_test.json` → Save `grass_bump.png` | Grass shows subtle height changes (not flat) | ![Place grass_bump.png in student_outputs](student_outputs/grass_bump.png) |
| L-system Tall tree | N/A | Tall Blocky L-system tree | |
| L-system Wide and Tall tree | N/A | Tall and wide blocky L-system tree | |

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
