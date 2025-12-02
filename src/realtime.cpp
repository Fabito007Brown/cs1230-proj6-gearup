#include "realtime.h"

#include <QCoreApplication>
#include <QMouseEvent>
#include <QKeyEvent>
#include <iostream>
#include "settings.h"
#include <algorithm>
#include <glm/gtc/matrix_transform.hpp>
#include "cube.h"
#include <cmath>
#include <cstdlib>   // for std::rand, RAND_MAX


#include <string>
#include <stack>
#include <unordered_map>


// ================== Rendering the Scene!

Realtime::Realtime(QWidget *parent)
    : QOpenGLWidget(parent)
{
    m_prev_mouse_pos = glm::vec2(size().width()/2, size().height()/2);
    setMouseTracking(true);
    setFocusPolicy(Qt::StrongFocus);

    m_keyMap[Qt::Key_W]       = false;
    m_keyMap[Qt::Key_A]       = false;
    m_keyMap[Qt::Key_S]       = false;
    m_keyMap[Qt::Key_D]       = false;
    m_keyMap[Qt::Key_Control] = false;
    m_keyMap[Qt::Key_Space]   = false;

    // If you must use this function, do not edit anything above this

    // FINAL PROJECT GEAR UP
    // === Stage 1: basic terrain + snake setup (no GL calls here) ===
    const int   gridWidth  = 30;
    const int   gridDepth  = 30;
    const float cellSize   = 1.f;


    // If you must use this function, do not edit anything above this

    // Default camera for our snake arena (no scenefile required)
    m_camPos  = glm::vec3(0.f, 5.f, 10.f);
    m_camLook = glm::normalize(glm::vec3(0.f, -0.4f, -1.f));
    m_camUp   = glm::vec3(0.f, 1.f, 0.f);


    // Snake (rigid body cube) initial state
    m_snake.pos = glm::vec3(0.f, 0.5f, 0.f);  // sits on ground
    m_snakeStartY = 0.5f;
    m_snake.vel = glm::vec3(0.f);             // not moving until key press


}

void Realtime::finish() {
    killTimer(m_timer);
    this->makeCurrent();

    // Students: anything requiring OpenGL calls when the program exits should be done here

    cleanupVAOs();
    cleanupTerrain();
    cleanupCubeMesh();
    if (m_shader) glDeleteProgram(m_shader);

    this->doneCurrent();
}



void Realtime::loadScene() {
    cleanupVAOs();

    std::string scenePath = settings.sceneFilePath;
    if (scenePath.empty()) {
        std::cout << "[Realtime] No scene file selected" << std::endl;
        return;
    }

    bool success = SceneParser::parse(scenePath, m_renderData);
    if (!success) {
        std::cerr << "[Realtime] Failed to parse scene" << std::endl;
        return;
    }

    // Set up camera from scene data
    m_camera.setViewMatrix(
        glm::vec3(m_renderData.cameraData.pos),
        glm::vec3(m_renderData.cameraData.look),
        glm::vec3(m_renderData.cameraData.up)
        );

    float aspect = (size().height() > 0) ? float(size().width()) / float(size().height()) : 1.f;
    m_camera.setProjectionMatrix(aspect, settings.nearPlane, settings.farPlane,
                                 m_renderData.cameraData.heightAngle);

    glm::vec3 pos  = glm::vec3(m_renderData.cameraData.pos);
    glm::vec3 look = glm::vec3(m_renderData.cameraData.look);
    glm::vec3 up   = glm::vec3(m_renderData.cameraData.up);

    // Save camera state
    m_camPos  = pos;
    m_camLook = look;
    m_camUp   = up;

    m_camera.setViewMatrix(m_camPos, m_camLook, m_camUp);



    generateShapeVAOs();

    std::cout << "[Realtime] Scene loaded: " << m_renderData.shapes.size()
              << " shapes, " << m_renderData.lights.size() << " lights" << std::endl;
}

void Realtime::generateShapeVAOs() {
    m_shapeVAOs.clear();

    for (const RenderShapeData& shape : m_renderData.shapes) {
        std::vector<float> vertexData = generateShapeData(
            shape.primitive.type,
            settings.shapeParameter1,
            settings.shapeParameter2
            );

        ShapeVAO shapeVAO;
        glGenVertexArrays(1, &shapeVAO.vao);
        glGenBuffers(1, &shapeVAO.vbo);

        glBindVertexArray(shapeVAO.vao);
        glBindBuffer(GL_ARRAY_BUFFER, shapeVAO.vbo);

        glBufferData(GL_ARRAY_BUFFER, vertexData.size() * sizeof(float),
                     vertexData.data(), GL_STATIC_DRAW);

        // Position attribute
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);

        // Normal attribute
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float),
                              (void*)(3 * sizeof(float)));

        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindVertexArray(0);

        shapeVAO.vertexCount = vertexData.size() / 6;
        m_shapeVAOs.push_back(shapeVAO);
    }
}

std::vector<float> Realtime::generateShapeData(PrimitiveType type, int param1, int param2) {
    int p1 = std::max(1, param1);
    int p2 = std::max(3, param2);

    switch (type) {
    case PrimitiveType::PRIMITIVE_CUBE: {
        Cube cube;
        cube.updateParams(p1, p2);
        return cube.generateShape();
    }
    case PrimitiveType::PRIMITIVE_CONE: {
        Cone cone;
        cone.updateParams(p1, p2);
        return cone.generateShape();
    }
    case PrimitiveType::PRIMITIVE_CYLINDER: {
        Cylinder cylinder;
        cylinder.updateParams(p1, p2);
        return cylinder.generateShape();
    }
    case PrimitiveType::PRIMITIVE_SPHERE: {
        Sphere sphere;
        sphere.updateParams(p1, p2);
        return sphere.generateShape();
    }
    default:
        return std::vector<float>();
    }
}

void Realtime::cleanupVAOs() {
    for (ShapeVAO& shapeVAO : m_shapeVAOs) {
        glDeleteBuffers(1, &shapeVAO.vbo);
        glDeleteVertexArrays(1, &shapeVAO.vao);
    }
    m_shapeVAOs.clear();
}

void Realtime::resetSnake() {
    m_snakeDead      = false;
    m_snakeDeathTime = 0.f;

    m_snakeForceDir = glm::vec3(0.f);

    // Head
    m_snake.pos = glm::vec3(0.f, 0.5f, 0.f);
    m_snake.vel = glm::vec3(0.f);

    // Body + trail
    m_snakeBody.clear();
    m_snakeTrail.clear();
    m_snakeTrail.push_back(m_snake.pos);
    m_lastTrailPos   = m_snake.pos;
    m_trailAccumDist = 0.f;

    // Give the player a new piece of food
    spawnFood();
}


void Realtime::spawnFood() {
    // Path is centered at x = 0, width = m_pathWidth tiles.
    // Keep food safely inside the walkable strip.
    float halfW = 0.5f * float(m_pathWidth) - 0.3f; // small margin from edges
    if (halfW < 0.5f) {
        halfW = 0.5f;
    }

    // Z goes along the strip from just past the door into the distance.
    // Path rows go from m_pathStartZ down to m_pathStartZ - m_pathLengthZ.
    float zNear = float(m_pathStartZ - 1);                 // just beyond the door
    float zFar  = float(m_pathStartZ - m_pathLengthZ + 1); // not at the very end

    // Two random numbers in [0,1]
    float rx = float(std::rand()) / float(RAND_MAX);
    float rz = float(std::rand()) / float(RAND_MAX);

    float x = (rx * 2.f - 1.f) * halfW; // in [-halfW, +halfW]
    float z = zNear + (zFar - zNear) * rz;

    m_foodPos = glm::vec3(x, 0.5f, z);
    m_hasFood = true;
}



void Realtime::initializeGL() {
    m_devicePixelRatio = this->devicePixelRatio();

    m_timer = startTimer(1000/60);
    m_elapsedTimer.start();

    glewExperimental = GL_TRUE;
    GLenum err = glewInit();
    if (err != GLEW_OK) {
        std::cerr << "Error while initializing GL: " << glewGetErrorString(err) << std::endl;
    }
    std::cout << "Initialized GL: Version " << glewGetString(GLEW_VERSION) << std::endl;

    glEnable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);
    glViewport(0, 0, size().width() * m_devicePixelRatio, size().height() * m_devicePixelRatio);

    // Load shader
    m_shader = ShaderLoader::createShaderProgram(
        ":/resources/shaders/default.vert",
        ":/resources/shaders/default.frag"
        );


    // Load brick diffuse + normal textures for the path
    m_pathDiffuseTex = loadTexture2D(":/resources/textures/brick_diffuse.jpg");
    m_pathNormalTex  = loadTexture2D(":/resources/textures/brick_normal.jpg");

    // Load stone wall textures (Rock051)
    m_grassDiffuseTex = loadTexture2D(":/resources/textures/grass_color.jpg");
    m_grassHeightTex  = loadTexture2D(":/resources/textures/grass_height.jpg");

    glClearColor(0.7f, 0.9f, 1.0f, 1.f); // soft sky blue


    // --- Crossy-Road style camera ---
    m_camPos  = glm::vec3(15.f, 20.f, 15.f);           // up and off to the side
    m_camLook = glm::normalize(glm::vec3(0.f) - m_camPos); // look toward origin
    m_camUp   = glm::vec3(0.f, 1.f, 0.f);              // world up

    float aspect = (size().height() > 0)
                       ? float(size().width()) / float(size().height())
                       : 1.f;
    // Slightly narrower FOV for a "telephoto" feel
    float fovY = glm::radians(40.f);

    m_camera.setViewMatrix(m_camPos, m_camLook, m_camUp);
    m_camera.setProjectionMatrix(aspect, settings.nearPlane, settings.farPlane, fovY);


    // --- Generate a flat terrain so we see something ---
    generateTerrain();
    // --- Generate reusable cube mesh + arena walls ---
    generateCubeMesh();
    buildArenaLayout();

    // Snake initial state
    resetSnake();



    //For camera movement stuff
    m_camOffsetFromSnake = m_camPos - m_snake.pos;
    m_followSnake        = true;

    // Door / path state
    m_doorOpened    = false;
    m_doorTimer     = 0.f;
    m_pathMode      = false;

    m_pathWidth     = 3;    // tweak if you want a wider path
    m_pathLengthZ   = 40;
    m_pathStartZ    = -10;  // just beyond front wall at z = -10


}


GLuint Realtime::loadTexture2D(const QString &path)
{
    QImage img(path);
    if (img.isNull()) {
        qWarning() << "Failed to load texture:" << path;
        return 0;
    }

    QImage glImg = img.convertToFormat(QImage::Format_RGBA8888).mirrored();

    GLuint tex = 0;
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);

    glTexImage2D(GL_TEXTURE_2D,
                 0,
                 GL_RGBA,
                 glImg.width(),
                 glImg.height(),
                 0,
                 GL_RGBA,
                 GL_UNSIGNED_BYTE,
                 glImg.bits());

    glGenerateMipmap(GL_TEXTURE_2D);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    glBindTexture(GL_TEXTURE_2D, 0);
    return tex;
}


static std::string expandLSystem(const std::string &axiom,
                                 const std::unordered_map<char, std::string> &rules,
                                 int iterations)
{
    std::string current = axiom;
    for (int i = 0; i < iterations; ++i) {
        std::string next;
        for (char c : current) {
            auto it = rules.find(c);
            if (it != rules.end()) {
                next += it->second;   // rewrite using rule
            } else {
                next.push_back(c);    // keep as-is
            }
        }
        current.swap(next);
    }
    return current;
}


// Generic version used for both the 3-tree scene and the tall-tree scene
void Realtime::addLSystemPlantCustom(float baseX, float baseZ,
                                     int iterations,
                                     float segH,
                                     float horizStep)
{
    using std::string;
    using RuleMap = std::unordered_map<char, string>;

    // Simple bush-like L-system:
    // X -> F[+X]F[-X]FX
    // F -> FF
    RuleMap rules;
    rules['X'] = "F[+X]F[-X]FX";
    rules['F'] = "FF";

    string axiom = "X";
    string str   = expandLSystem(axiom, rules, iterations);

    glm::vec3 trunkColor(0.50f, 0.35f, 0.20f);
    glm::vec3 leafColor (0.35f, 0.65f, 0.30f);

    struct Turtle {
        glm::vec3 pos;
    };

    Turtle t;
    t.pos = glm::vec3(baseX, 0.4f, baseZ);   // base on the ground-ish

    std::stack<Turtle> stack;

    auto addCube = [&](const glm::vec3 &p, float h, const glm::vec3 &col) {
        CubeInstance inst;
        inst.pos   = glm::vec3(p.x, p.y + 0.5f * h, p.z);
        inst.scale = glm::vec3(1.f, h, 1.f);
        inst.color = col;
        // if CubeInstance has a material field, you can default it:
        // inst.material = 0;
        m_cubes.push_back(inst);
    };

    for (char c : str) {
        switch (c) {
        case 'F':
            // trunk / branch going upward
            addCube(t.pos, segH, trunkColor);
            t.pos.y += segH;
            break;
        case 'X':
            // leaf blob at the tip
            addCube(t.pos, segH, leafColor);
            break;
        case '+':
            // small horizontal offset to one side
            t.pos.x += horizStep;
            break;
        case '-':
            // small horizontal offset to the other side
            t.pos.x -= horizStep;
            break;
        case '[':
            stack.push(t);
            break;
        case ']':
            if (!stack.empty()) {
                t = stack.top();
                stack.pop();
            }
            break;
        default:
            break;
        }
    }
}

// Old convenience wrapper used by the arena + 3-tree test
void Realtime::addLSystemPlant(float baseX, float baseZ)
{
    // Your original settings: 2 iters, segH = 0.35, horizStep = 0.6
    addLSystemPlantCustom(baseX, baseZ, 2, 0.35f, 0.6f);
}

void Realtime::buildLSystemTestScene(bool singleTall)
{
    // Clear out gameplay stuff so it doesn't interfere
    m_cubes.clear();
    m_snakeBody.clear();
    m_hasFood   = false;
    m_snakeDead = false;

    // --- Camera: nice angle for screenshot ---
    m_camPos = glm::vec3(0.f, 10.f, 20.f);
    m_camera.setViewMatrix(
        m_camPos,
        glm::vec3(0.f, 4.f, 0.f),   // look toward center
        glm::vec3(0.f, 1.f, 0.f)
        );

    // --- Simple flat platform under the tree(s) ---
    // 11x11 grid of flat tiles centered at origin
    for (int x = -5; x <= 5; ++x) {
        for (int z = -5; z <= 5; ++z) {
            CubeInstance tile;
            tile.pos   = glm::vec3((float)x, 0.f, (float)z);
            tile.scale = glm::vec3(1.f, 0.2f, 1.f);
            tile.color = glm::vec3(0.25f, 0.80f, 0.45f); // green-ish
            // tile.material = 0; // if you have this field
            m_cubes.push_back(tile);
        }
    }

    // --- Trees from the REAL L-system ---
    if (singleTall) {
        // One taller tree in the middle: more iterations + taller segments
        addLSystemPlantCustom(
            /*baseX*/ 0.f,
            /*baseZ*/ 0.f,
            /*iterations*/ 3,     // 2 -> 3 makes it noticeably taller
            /*segH*/      0.45f,  // slightly taller segments
            /*horizStep*/ 0.6f
            );
    } else {
        // Your original 3-tree arrangement, using the default parameters
        addLSystemPlant(-4.f, 0.f);
        addLSystemPlant( 0.f, 0.f);
        addLSystemPlant( 4.f, 0.f);
    }

    update(); // trigger redraw
}

// 3-tree version (what you already had conceptually)
void Realtime::buildLSystemOnlyScene()
{
    buildLSystemTestScene(false);
}

// 1 tall tree version for the test screenshot
void Realtime::buildLSystemTallTreeScene()
{
    buildLSystemTestScene(true);
}

void Realtime::buildLSystemTallWideTreeScene()
{
    // Clear gameplay stuff so it doesn't interfere
    m_cubes.clear();
    m_snakeBody.clear();
    m_hasFood   = false;
    m_snakeDead = false;

    // --- Camera: same nice angle as the other L-system tests ---
    m_camPos = glm::vec3(0.f, 10.f, 20.f);
    m_camera.setViewMatrix(
        m_camPos,
        glm::vec3(0.f, 4.f, 0.f),   // look toward center
        glm::vec3(0.f, 1.f, 0.f)
        );

    // --- Simple flat platform under the tree ---
    for (int x = -5; x <= 5; ++x) {
        for (int z = -5; z <= 5; ++z) {
            CubeInstance tile;
            tile.pos   = glm::vec3((float)x, 0.f, (float)z);
            tile.scale = glm::vec3(1.f, 0.2f, 1.f);
            tile.color = glm::vec3(0.25f, 0.80f, 0.45f);
            // tile.material = 0; // if you have a material field
            m_cubes.push_back(tile);
        }
    }

    // --- Single tall tree with LONGER branches ---
    //    (same L-system rules, just different parameters)
    addLSystemPlantCustom(
        /*baseX*/    0.f,
        /*baseZ*/    0.f,
        /*iterations*/ 3,      // same as tall tree
        /*segH*/      0.45f,   // tall-ish segments
        /*horizStep*/ 1.0f     // BIGGER sideways step = longer branches
        );

    update();
}



void Realtime::buildGrassBumpTestScene() {
    // Clear any existing cubes
    m_cubes.clear();


    // Turn off snake follow so camera doesn't get overridden
    m_followSnake = false;

    // Place camera looking at the cube
    // In switchToBrickTestScene()
    m_camPos  = glm::vec3(0.f, 10.f, 0.2f);

    // Instead of looking STRAIGHT DOWN, tilt forward toward +Z
    m_camLook = glm::vec3(0.f, -0.8f, 0.4f);

    // Up stays world-up-ish
    m_camUp   = glm::vec3(0.f, 1.f, 0.f);

    m_camera.setViewMatrix(m_camPos, m_camLook, m_camUp);
}


void Realtime::generateLSystemFoliageStrip(int zStart, int zEnd, bool leftSide) {
    // Distance from path center to where we plant bushes
    float baseOffsetX = (m_pathWidth * 0.5f) + 3.f; // 1–2 blocks beyond the stone border

    int step = 5; // spacing along the Z direction

    for (int gz = zStart; gz >= zEnd; gz -= step) {
        float x = leftSide ? -baseOffsetX : baseOffsetX;
        addLSystemPlant(x, float(gz));
    }
}



//PROJECT 6 GEAR UP
void Realtime::generateTerrain() {
    cleanupTerrain(); // in case we regenerate

    int resolution = 100;   // 100 x 100 quads
    float size     = 20.f;  // world-space width/depth

    std::vector<float> vertexData = m_terrainGenerator.generateFlatGrid(resolution, size);
    m_terrainVertexCount = static_cast<int>(vertexData.size() / 6); // 3 pos + 3 normal

    glGenVertexArrays(1, &m_terrainVAO);
    glGenBuffers(1, &m_terrainVBO);

    glBindVertexArray(m_terrainVAO);
    glBindBuffer(GL_ARRAY_BUFFER, m_terrainVBO);
    glBufferData(GL_ARRAY_BUFFER,
                 vertexData.size() * sizeof(float),
                 vertexData.data(),
                 GL_STATIC_DRAW);

    // position attribute (location = 0)
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE,
                          6 * sizeof(float),
                          (void*)0);

    // normal attribute (location = 1)
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE,
                          6 * sizeof(float),
                          (void*)(3 * sizeof(float)));

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}

void Realtime::cleanupTerrain() {
    if (m_terrainVBO) {
        glDeleteBuffers(1, &m_terrainVBO);
        m_terrainVBO = 0;
    }
    if (m_terrainVAO) {
        glDeleteVertexArrays(1, &m_terrainVAO);
        m_terrainVAO = 0;
    }
    m_terrainVertexCount = 0;
}

// CUBE MESH + ARENA LAYOUT

void Realtime::generateCubeMesh() {
    cleanupCubeMesh();

    Cube cube;
    cube.updateParams(1, 1); // lowest tessellation; nice blocky cube
    std::vector<float> data = cube.generateShape();
    m_cubeVertexCount = static_cast<int>(data.size() / 6); // 3 pos + 3 normal

    glGenVertexArrays(1, &m_cubeVAO);
    glGenBuffers(1, &m_cubeVBO);

    glBindVertexArray(m_cubeVAO);
    glBindBuffer(GL_ARRAY_BUFFER, m_cubeVBO);

    glBufferData(GL_ARRAY_BUFFER,
                 data.size() * sizeof(float),
                 data.data(),
                 GL_STATIC_DRAW);

    // position (location = 0)
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE,
                          6 * sizeof(float),
                          (void*)0);

    // normal (location = 1)
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE,
                          6 * sizeof(float),
                          (void*)(3 * sizeof(float)));

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}

void Realtime::cleanupCubeMesh() {
    if (m_cubeVBO) {
        glDeleteBuffers(1, &m_cubeVBO);
        m_cubeVBO = 0;
    }
    if (m_cubeVAO) {
        glDeleteVertexArrays(1, &m_cubeVAO);
        m_cubeVAO = 0;
    }
    m_cubeVertexCount = 0;
}


bool Realtime::cellBlocked(int gx, int gz) const {
    // Treat outside playable area as blocked too
    float snakeRadius = 0.4f; // half-size of the snake cube in x/z

    for (const CubeInstance &c : m_cubes) {
        float hx = 0.5f * c.scale.x;
        float hz = 0.5f * c.scale.z;

        float dx = std::abs(float(gx) - c.pos.x);
        float dz = std::abs(float(gz) - c.pos.z);

        if (dx < hx + snakeRadius && dz < hz + snakeRadius) {
            // Only *taller* cubes are solid. Low cubes (path floor) are walkable.
            if (c.scale.y > 0.6f) {
                return true;
            }
        }
    }

    return false;
}



void Realtime::buildArenaLayout() {
    m_cubes.clear();

    // Our terrain is size = 20.f, centered at origin -> half extent = 10
    const float half       = 10.f;
    const float wallHeight = 2.f;   // a bit taller than the snake will be
    const float unit       = 1.f;   // cube size along x/z

    glm::vec3 wallColor(0.9f, 0.9f, 0.95f); // soft light gray/white

    auto addCube = [&](float x, float z, float height,
                       const glm::vec3 &color,
                       int material = MAT_DEFAULT) {
        CubeInstance inst;
        inst.pos      = glm::vec3(x, height * 0.5f, z); // center at half height
        inst.scale    = glm::vec3(unit, height, unit);
        inst.color    = color;
        inst.material = material;
        m_cubes.push_back(inst);
    };

    // ---------- 1) BORDER WALLS (solid ring) ----------
    for (float x = -half; x <= half; x += unit) {
        // front & back walls – **no gap here**, door opens later in openFrontDoor()
        addCube(x, -half, wallHeight, wallColor, MAT_WALL); // front (z = -10)
        addCube(x,  half, wallHeight, wallColor, MAT_WALL); // back  (z = +10)
    }

    for (float z = -half; z <= half; z += unit) {
        addCube(-half, z, wallHeight, wallColor, MAT_WALL); // left  (x = -10)
        addCube( half, z, wallHeight, wallColor, MAT_WALL); // right (x = +10)
    }

    // ---------- 2) PATCHY NOISE-BASED INTERIOR ----------
    glm::vec3 baseGrass(0.55f, 0.85f, 0.55f);
    glm::vec3 dirt     (0.45f, 0.35f, 0.22f);

    auto hash01 = [](int x, int z) {
        float v = std::sin(x * 12.9898f + z * 78.233f) * 43758.5453f;
        return v - std::floor(v);
    };

    const float centerClearRadius = 4.0f;  // always flat zone where snake can live
    const float spawnProbability  = 0.45f; // 45% of tiles get a column

    for (int gz = -9; gz <= 9; ++gz) {
        for (int gx = -9; gx <= 9; ++gx) {
            float distCenter = std::sqrt(float(gx*gx + gz*gz));
            if (distCenter < centerClearRadius)
                continue;

            // keep corridor clear in front of the door (z negative, centered in x)
            if (std::abs(gx) <= 1 && gz <= -2 && gz >= -9)
                continue;

            float r = hash01(gx, gz);
            if (r > spawnProbability)
                continue;

            float nx = gx * 0.35f;
            float nz = gz * 0.35f;
            float n  = 0.5f * std::sin(nx) + 0.5f * std::cos(nz);
            n = 0.5f * (n + 1.f); // [0,1]

            float height = 0.4f + 1.2f * n;

            float t = std::clamp((height - 0.4f) / 1.2f, 0.f, 1.f);
            glm::vec3 color = (1.f - t) * baseGrass + t * dirt;

            addCube(float(gx), float(gz), height, color);
        }
    }
}



void Realtime::buildInitialPathStrip() {
    // NOTE: do NOT clear m_cubes here; we keep the arena + hills.
    const float unit      = 1.f;
    const float halfWidth = m_pathWidth * 0.5f;  // half width of walkable strip

    const float floorH    = 0.1f;   // low so it doesn't block in cellBlocked

    glm::vec3 pathColor   (0.80f, 0.72f, 0.50f);  // dirt-ish
    glm::vec3 edgeStone   (0.70f, 0.70f, 0.78f);  // stone edge
    glm::vec3 foliageGreen(0.45f, 0.70f, 0.40f);  // leafy blocks

    auto addCube = [&](float x, float yHeight, float z,
                       const glm::vec3 &color, int material) {
        CubeInstance inst;
        inst.pos     = glm::vec3(x, yHeight * 0.5f, z);
        inst.scale   = glm::vec3(unit, yHeight, unit);
        inst.color   = color;
        inst.material = material; // 0 = default, 1 = path floor
        m_cubes.push_back(inst);
    };


    // Build a straight strip going in -Z direction starting just outside the door
    int zStart = m_pathStartZ;
    int zEnd   = m_pathStartZ - m_pathLengthZ;

    for (int gz = zStart; gz >= zEnd; --gz) {
        for (int gx = -10; gx <= 10; ++gx) {
            // Inside walkable path (center strip) ->  material 1 (normal-mapped bricks)
            if (std::abs(gx) <= halfWidth) {
                addCube(float(gx), floorH, float(gz), pathColor, /*material=*/1);
            }
            // stone borders (no normal map) -> material 0
            else if (std::abs(gx) == int(halfWidth) + 1) {
                addCube(float(gx), floorH + 0.6f, float(gz), edgeStone, /*material=*/0);
            }
            // foliage / trees (no normal map -> material 0
            else if (std::abs(gx) > int(halfWidth) + 1) {
                float v = std::sin(gx * 12.9898f + gz * 78.233f) * 43758.5453f;
                float r = v - std::floor(v);
                if (r < 0.25f) {
                    float h = 1.4f + 0.8f * r;
                    addCube(float(gx), h, float(gz), foliageGreen, /*material=*/0);
                }
            }

        }
    }

    // Add L-system bushes along both sides of the path
    generateLSystemFoliageStrip(zStart, zEnd, /*leftSide=*/true);
    generateLSystemFoliageStrip(zStart, zEnd, /*leftSide=*/false);
}



void Realtime::buildNormalMapTestScene() {
    // Clear any existing cubes
    m_cubes.clear();

    // One big cube at origin that uses the PATH material (normal-mapped bricks)
    CubeInstance inst;
    inst.pos      = glm::vec3(0.f, 0.f, 0.f);
    inst.scale    = glm::vec3(4.f, 4.f, 4.f); // nice big cube
    inst.color    = glm::vec3(1.f, 1.f, 1.f); // white so brick texture shows clearly
    inst.material = MAT_PATH;                 // uses brick diffuse + normal map
    m_cubes.push_back(inst);

    // Turn off snake follow so camera doesn't get overridden
    m_followSnake = false;

    // Place camera looking at the cube
    // In switchToBrickTestScene()
    m_camPos   = glm::vec3(0.f, 10.f, 0.2f);
    m_camLook  = glm::vec3(0.f, -1.f, 0.f);
    m_camUp    = glm::vec3(0.f, 0.f, 1.f);

    m_camera.setViewMatrix(m_camPos, m_camLook, m_camUp);
}


void Realtime::rebuildMainArenaScene() {
    // Restore original arena + snake + camera so you can keep playing
    m_cubes.clear();
    buildArenaLayout();

    // Reset door/path state
    m_doorOpened  = false;
    m_doorTimer   = 0.f;
    m_pathMode    = false;

    // Reset snake + food
    resetSnake();

    // Back to Crossy-Road camera
    m_camPos  = glm::vec3(15.f, 20.f, 15.f);
    m_camLook = glm::normalize(glm::vec3(0.f) - m_camPos);
    m_camUp   = glm::vec3(0.f, 1.f, 0.f);
    m_camera.setViewMatrix(m_camPos, m_camLook, m_camUp);

    // Follow snake again
    m_camOffsetFromSnake = m_camPos - m_snake.pos;
    m_followSnake        = true;
}



void Realtime::openFrontDoor() {
    if (m_doorOpened) return;
    m_doorOpened = true;

    // Arena extent must match buildArenaLayout
    const float half      = 10.f;
    const float doorZ     = -half;       // front wall (same side as path: m_pathStartZ = -10)
    const float doorXStart = -1.5f;      // centered opening
    const float doorXEnd   =  1.5f;
    const float zEpsilon   = 0.5f;

    for (CubeInstance &c : m_cubes) {
        bool onFrontWall = std::abs(c.pos.z - doorZ) < zEpsilon;
        bool inDoorSpan  = (c.pos.x >= doorXStart && c.pos.x <= doorXEnd);

        if (onFrontWall && inDoorSpan && c.scale.y > 0.f) {
            // “remove” the cube by shrinking its height
            c.scale.y = 0.f;
        }
    }
}


void Realtime::paintGL() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    if (!m_shader) return;

    glUseProgram(m_shader);

    // --- material & texture uniforms ---
    GLint useBlockyLoc       = glGetUniformLocation(m_shader, "useBlocky");
    GLint useNormalMapLoc    = glGetUniformLocation(m_shader, "useNormalMap");
    GLint usePathMaterialLoc = glGetUniformLocation(m_shader, "usePathMaterial");
    GLint pathUVScaleLoc     = glGetUniformLocation(m_shader, "pathUVScale");

    // UV scale for brick tiling
    glUniform1f(pathUVScaleLoc, m_pathUVScale);

    // Bind brick textures to texture units 0 and 1
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, m_pathDiffuseTex);
    glUniform1i(glGetUniformLocation(m_shader, "pathDiffuseMap"), 0);

    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, m_pathNormalTex);
    glUniform1i(glGetUniformLocation(m_shader, "pathNormalMap"), 1);



    //GRASSS
    // NEW: bind grass textures to units 2 and 3
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, m_grassDiffuseTex);
    glUniform1i(glGetUniformLocation(m_shader, "grassDiffuseMap"), 2);

    glActiveTexture(GL_TEXTURE3);
    glBindTexture(GL_TEXTURE_2D, m_grassHeightTex);
    glUniform1i(glGetUniformLocation(m_shader, "grassHeightMap"), 3);

    // UV + bump strength
    glUniform1f(glGetUniformLocation(m_shader, "grassUVScale"),  m_grassUVScale);
    glUniform1f(glGetUniformLocation(m_shader, "grassBumpScale"), m_grassBumpScale);

    GLint useGrassBumpLoc = glGetUniformLocation(m_shader, "useGrassBump");





    // --- camera matrices ---
    glm::mat4 view = m_camera.getViewMatrix();
    glm::mat4 proj = m_camera.getProjMatrix();
    glUniformMatrix4fv(glGetUniformLocation(m_shader, "view"),
                       1, GL_FALSE, &view[0][0]);
    glUniformMatrix4fv(glGetUniformLocation(m_shader, "proj"),
                       1, GL_FALSE, &proj[0][0]);

    // --- camera position ---
    glUniform3fv(glGetUniformLocation(m_shader, "camPos"),
                 1, &m_camPos[0]);

    // --- global lighting coeffs ---
    float ka = 0.2f, kd = 0.8f, ks = 0.3f;
    glUniform1f(glGetUniformLocation(m_shader, "k_a"), ka);
    glUniform1f(glGetUniformLocation(m_shader, "k_d"), kd);
    glUniform1f(glGetUniformLocation(m_shader, "k_s"), ks);

    // --- one directional light ---
    auto loc0 = [&](const std::string &field) {
        std::string name = "lights[0]." + field;
        return glGetUniformLocation(m_shader, name.c_str());
    };
    glUniform1i(glGetUniformLocation(m_shader, "numLights"), 1);

    int typeDir = 1;
    glm::vec3 lightColor(1.f, 1.f, 1.f);
    glm::vec3 lightDir = glm::normalize(glm::vec3(-1.f, -1.f, -1.f));
    glm::vec3 lightPos(0.f);            // unused for directional
    glm::vec3 atten(1.f, 0.f, 0.f);     // no falloff

    glUniform1i(loc0("type"), typeDir);
    glUniform3fv(loc0("color"), 1, &lightColor[0]);
    glUniform3fv(loc0("pos"),   1, &lightPos[0]);
    glUniform3fv(loc0("dir"),   1, &lightDir[0]);
    glUniform3fv(loc0("atten"), 1, &atten[0]);
    glUniform1f(loc0("angle"),    0.f);
    glUniform1f(loc0("penumbra"), 0.f);



    // ---------- TERRAIN (no blocky effect, no textures) ----------
    // ---------- TERRAIN (bump-mapped grass) ----------
    glUniform1i(useBlockyLoc,       0);
    glUniform1i(usePathMaterialLoc, 0);
    // glUniform1i(useNormalMapLoc,    0);

    // NEW: enable bump mapping for terrain
    glUniform1i(useGrassBumpLoc, 1);

    if (m_terrainVAO && m_terrainVertexCount > 0) {
        glm::mat4 model(1.f);
        glUniformMatrix4fv(glGetUniformLocation(m_shader, "model"),
                           1, GL_FALSE, &model[0][0]);

        // These will still be used as "base" values, but diffuse will get overridden
        glm::vec3 cA(0.35f, 0.55f, 0.35f);
        glm::vec3 cD(0.55f, 0.85f, 0.55f);
        glm::vec3 cS(0.04f, 0.04f, 0.04f);
        glUniform3fv(glGetUniformLocation(m_shader, "cAmbient"),  1, &cA[0]);
        glUniform3fv(glGetUniformLocation(m_shader, "cDiffuse"),  1, &cD[0]);
        glUniform3fv(glGetUniformLocation(m_shader, "cSpecular"), 1, &cS[0]);
        glUniform1f(glGetUniformLocation(m_shader, "shininess"),  6.f);

        glBindVertexArray(m_terrainVAO);
        glDrawArrays(GL_TRIANGLES, 0, m_terrainVertexCount);
        glBindVertexArray(0);
    }


    // ---------- ARENA WALL CUBES + PATH (blocky) ----------
    glUniform1i(useGrassBumpLoc,   0);
    glUniform1i(useBlockyLoc, 1);

    if (m_cubeVAO && m_cubeVertexCount > 0) {
        for (const CubeInstance &inst : m_cubes) {
            // Skip "deleted" cubes (e.g., door pieces) so they don't cause seams
            if (inst.scale.y <= 0.f) continue;

            // === material flags: 1 = path bricks, 0 = normal cube ===
            bool isPathFloor = (inst.material == 1);

            glUniform1i(usePathMaterialLoc, isPathFloor ? 1 : 0);
            glUniform1i(useNormalMapLoc,
                        (isPathFloor && m_pathNormalTex != 0) ? 1 : 0);

            glm::mat4 model = glm::translate(glm::mat4(1.f), inst.pos)
                              * glm::scale(glm::mat4(1.f), inst.scale);
            glUniformMatrix4fv(glGetUniformLocation(m_shader, "model"),
                               1, GL_FALSE, &model[0][0]);

            glm::vec3 cD = inst.color;
            glm::vec3 cA = 0.6f * cD;
            glm::vec3 cS(0.08f, 0.08f, 0.08f);
            glUniform3fv(glGetUniformLocation(m_shader, "cAmbient"),  1, &cA[0]);
            glUniform3fv(glGetUniformLocation(m_shader, "cDiffuse"),  1, &cD[0]);
            glUniform3fv(glGetUniformLocation(m_shader, "cSpecular"), 1, &cS[0]);
            glUniform1f(glGetUniformLocation(m_shader, "shininess"), 10.f);

            glBindVertexArray(m_cubeVAO);
            glDrawArrays(GL_TRIANGLES, 0, m_cubeVertexCount);
            glBindVertexArray(0);
        }

        // ---------- SNAKE (single rigid-body cube, blocky, NO normal map) ----------
        // ---------- SNAKE HEAD (single rigid-body cube, blocky, NO normal map) ----------
        glUniform1i(usePathMaterialLoc, 0);
        glUniform1i(useNormalMapLoc,    0);

        float baseScale = 0.8f;
        float scaleY    = baseScale;
        float scaleXZ   = baseScale;

        if (m_snakeDead) {
            float t = glm::clamp(m_snakeDeathTime / 0.5f, 0.f, 1.f);
            scaleY  = baseScale * (1.f - t);
            scaleXZ = baseScale * (1.f + 0.4f * t);
        }

        glm::mat4 snakeModel =
            glm::translate(glm::mat4(1.f), m_snake.pos) *
            glm::scale(glm::mat4(1.f), glm::vec3(scaleXZ, scaleY, scaleXZ));

        glUniformMatrix4fv(glGetUniformLocation(m_shader, "model"),
                           1, GL_FALSE, &snakeModel[0][0]);

        glm::vec3 headD(1.0f, 0.9f, 0.2f);
        glm::vec3 headA = 0.6f * headD;
        glm::vec3 headS(0.12f, 0.12f, 0.12f);

        glUniform3fv(glGetUniformLocation(m_shader, "cAmbient"),  1, &headA[0]);
        glUniform3fv(glGetUniformLocation(m_shader, "cDiffuse"),  1, &headD[0]);
        glUniform3fv(glGetUniformLocation(m_shader, "cSpecular"), 1, &headS[0]);
        glUniform1f(glGetUniformLocation(m_shader, "shininess"),  18.f);

        glBindVertexArray(m_cubeVAO);
        glDrawArrays(GL_TRIANGLES, 0, m_cubeVertexCount);

        // ---------- SNAKE BODY SEGMENTS ----------
        for (const glm::vec3 &segPos : m_snakeBody) {
            glm::mat4 bodyModel =
                glm::translate(glm::mat4(1.f), segPos) *
                glm::scale(glm::mat4(1.f), glm::vec3(0.7f, 0.7f, 0.7f));

            glUniformMatrix4fv(glGetUniformLocation(m_shader, "model"),
                               1, GL_FALSE, &bodyModel[0][0]);

            glm::vec3 bodyD(0.95f, 0.8f, 0.2f); // slightly dimmer yellow
            glm::vec3 bodyA = 0.6f * bodyD;
            glm::vec3 bodyS(0.10f, 0.10f, 0.10f);
            glUniform3fv(glGetUniformLocation(m_shader, "cAmbient"),  1, &bodyA[0]);
            glUniform3fv(glGetUniformLocation(m_shader, "cDiffuse"),  1, &bodyD[0]);
            glUniform3fv(glGetUniformLocation(m_shader, "cSpecular"), 1, &bodyS[0]);
            glUniform1f(glGetUniformLocation(m_shader, "shininess"),  12.f);

            glDrawArrays(GL_TRIANGLES, 0, m_cubeVertexCount);
        }

        // ---------- FOOD ----------
        if (m_hasFood) {
            glm::mat4 foodModel =
                glm::translate(glm::mat4(1.f), m_foodPos) *
                glm::scale(glm::mat4(1.f), glm::vec3(0.6f, 0.6f, 0.6f));

            glUniformMatrix4fv(glGetUniformLocation(m_shader, "model"),
                               1, GL_FALSE, &foodModel[0][0]);

            glm::vec3 fd(0.95f, 0.25f, 0.25f); // reddish fruit
            glm::vec3 fa = 0.6f * fd;
            glm::vec3 fs(0.12f, 0.12f, 0.12f);
            glUniform3fv(glGetUniformLocation(m_shader, "cAmbient"),  1, &fa[0]);
            glUniform3fv(glGetUniformLocation(m_shader, "cDiffuse"),  1, &fd[0]);
            glUniform3fv(glGetUniformLocation(m_shader, "cSpecular"), 1, &fs[0]);
            glUniform1f(glGetUniformLocation(m_shader, "shininess"),  20.f);

            glDrawArrays(GL_TRIANGLES, 0, m_cubeVertexCount);
        }

        glBindVertexArray(0);
    }

    glUseProgram(0);
}





void Realtime::resizeGL(int w, int h) {
    glViewport(0, 0,
               size().width() * m_devicePixelRatio,
               size().height() * m_devicePixelRatio);

    float aspect = (h > 0) ? float(w) / float(h) : 1.f;
    float fovY      = glm::radians(45.f);
    float nearPlane = 0.1f;
    float farPlane  = 100.f;

    m_camera.setProjectionMatrix(aspect, nearPlane, farPlane, fovY);
}


void Realtime::sceneChanged() {
    makeCurrent();
    loadScene();
    doneCurrent();
    update();


}


// NEAR PLANE + FAR PLANE
// NEAR PLANE should chop off front
void Realtime::settingsChanged() {

    makeCurrent();

    if (!m_renderData.shapes.empty()) {
        generateShapeVAOs();
    }

    float aspect = float(size().width()) / float(size().height());
    float fovY = !m_renderData.shapes.empty()
                     ? m_renderData.cameraData.heightAngle
                     : 45.f * 3.1415926535f / 180.f;

    m_camera.setProjectionMatrix(aspect, settings.nearPlane, settings.farPlane, fovY);

    doneCurrent();
    update();

}

// ================== Camera Movement!

// Helper to rotate vector v around (normalized) axis by angle (radians)
//based on rodrigues
static glm::vec3 rotateAroundAxis(const glm::vec3 &v,
                                  const glm::vec3 &axis,
                                  float angle)
{
    glm::vec3 a = glm::normalize(axis);
    float c = std::cos(angle);
    float s = std::sin(angle);

    // Rodrigues' rotation formula
    return v * c
           + glm::cross(a, v) * s
           + a * glm::dot(a, v) * (1.f - c);
}


void Realtime::keyPressEvent(QKeyEvent *event) {
    Qt::Key key = Qt::Key(event->key());

    // Debug / test scenes
    if (key == Qt::Key_B) {
        // B = Brick normal-map test cube
        buildNormalMapTestScene();
        update();
        return;
    }
    if (key == Qt::Key_L) {
        // L = three-tree L-system scene (what you had)
        buildLSystemOnlyScene();
        update();
        return;
    }

    if (key == Qt::Key_T) {
        // T = single tall L-system tree scene for testing
        buildLSystemTallTreeScene();
        update();
        return;
    }

    // NEW: save grass bump-mapping screenshot
    if (key == Qt::Key_G) {
        // NEW: grass bump-mapping test view
        buildGrassBumpTestScene();
        update();
        return;
    }


    if (key == Qt::Key_Y) {
        // Y = tall tree with long branches
        buildLSystemTallWideTreeScene();
        update();
        return;
    }

    if (key == Qt::Key_R) {
        // R = Return to main arena game
        rebuildMainArenaScene();
        update();
        return;
    }

    if (event->key() == Qt::Key_N) {
        m_useNormalMap = !m_useNormalMap;
        std::cout << "useNormalMap = " << m_useNormalMap << std::endl;
        update();  // repaint
    }

    // WASD control snake direction (velocity)
    if (key == Qt::Key_W) {
        m_snakeForceDir = glm::vec3(0.f, 0.f, -1.f); // forward (-Z)
    } else if (key == Qt::Key_S) {
        m_snakeForceDir = glm::vec3(0.f, 0.f,  1.f); // back (+Z)
    } else if (key == Qt::Key_A) {
        m_snakeForceDir = glm::vec3(-1.f, 0.f, 0.f); // left (-X)
    } else if (key == Qt::Key_D) {
        m_snakeForceDir = glm::vec3( 1.f, 0.f, 0.f); // right (+X)
    } else {
        m_keyMap[key] = true;
    }
}

void Realtime::keyReleaseEvent(QKeyEvent *event) {
    Qt::Key key = Qt::Key(event->key());
    m_keyMap[key] = false;

    // If all WASD are up, stop applying input force
    if (!m_keyMap[Qt::Key_W] &&
        !m_keyMap[Qt::Key_A] &&
        !m_keyMap[Qt::Key_S] &&
        !m_keyMap[Qt::Key_D]) {
        m_snakeForceDir = glm::vec3(0.f);
    }
}

void Realtime::mousePressEvent(QMouseEvent *event) {
    if (event->buttons().testFlag(Qt::LeftButton)) {
        m_mouseDown = true;
        m_prev_mouse_pos = glm::vec2(event->position().x(), event->position().y());
    }
}

void Realtime::mouseReleaseEvent(QMouseEvent *event) {
    if (!event->buttons().testFlag(Qt::LeftButton)) {
        m_mouseDown = false;
    }
}

void Realtime::mouseMoveEvent(QMouseEvent *event) {

    if (m_followSnake) {
        // Ignore manual orbit when camera is locked onto the snake
        return;
    }
    if (m_mouseDown) {
        int posX = event->position().x();
        int posY = event->position().y();
        int deltaX = posX - m_prev_mouse_pos.x;
        int deltaY = posY - m_prev_mouse_pos.y;
        m_prev_mouse_pos = glm::vec2(posX, posY);

        if (m_mouseDown) {
            const float sensitivity = 0.005f; // radians per pixel

            float yaw   = -deltaX * sensitivity; // left/right
            float pitch = -deltaY * sensitivity; // up/down

            glm::vec3 worldUp(0.f, 1.f, 0.f);

            // to rotate around world up
            if (yaw != 0.f) {
                m_camLook = rotateAroundAxis(m_camLook, worldUp, yaw);
                m_camUp   = rotateAroundAxis(m_camUp,   worldUp, yaw);
            }

            // recompute right after yaw
            glm::vec3 right = glm::normalize(glm::cross(m_camLook, m_camUp));

            // to rotate around camera's right axis
            if (pitch != 0.f) {
                m_camLook = rotateAroundAxis(m_camLook, right, pitch);
                m_camUp   = rotateAroundAxis(m_camUp,   right, pitch);
            }

            // Re-orthonormalize basis
            m_camLook = glm::normalize(m_camLook);
            right     = glm::normalize(glm::cross(m_camLook, m_camUp));
            m_camUp   = glm::normalize(glm::cross(right, m_camLook));

            m_camera.setViewMatrix(m_camPos, m_camLook, m_camUp);
            update();
        }
    }
}

void Realtime::timerEvent(QTimerEvent *event) {
    Q_UNUSED(event);

    int   elapsedms = m_elapsedTimer.elapsed();
    float deltaTime = elapsedms * 0.001f;
    m_elapsedTimer.restart();

    //snake update
    if (!m_snakeDead) {
        // --- 1a) Integrate phsyics motion ---
        // Forces
        glm::vec3 F_input = m_snakeForceDir * m_snakeForceMag;
        glm::vec3 F_fric  = -m_snake.vel * m_snakeFriction;   // velocity damping
        glm::vec3 F_total = F_input + F_fric;

        // Acceleration
        glm::vec3 a = F_total / m_snakeMass;

        // Integrate velocity and clamp speed
        m_snake.vel += a * deltaTime;

        float speed = glm::length(m_snake.vel);
        if (speed > m_snakeMaxSpeed) {
            m_snake.vel = (m_snake.vel / speed) * m_snakeMaxSpeed;
        }

        // Integrate position
        glm::vec3 proposed = m_snake.pos + m_snake.vel * deltaTime;
        proposed.y = 0.5f; // keep snake on the ground plane

        int gx = std::round(proposed.x);
        int gz = std::round(proposed.z);

        if (!cellBlocked(gx, gz)) {
            m_snake.pos = proposed;
        } else {
            // hit wall / hill => die and start squash timer
            m_snakeDead      = true;
            m_snakeDeathTime = 0.f;
        }

        // --- 1b) Update head trail ---
        float stepDist = glm::length(m_snake.pos - m_lastTrailPos);
        m_trailAccumDist += stepDist;
        if (m_trailAccumDist >= m_trailSampleDist) {
            m_snakeTrail.push_front(m_snake.pos);
            m_lastTrailPos   = m_snake.pos;
            m_trailAccumDist = 0.f;

            // keep trail reasonably short
            size_t maxTrail = (m_snakeBody.size() + 5) * 8;
            while (m_snakeTrail.size() > maxTrail) {
                m_snakeTrail.pop_back();
            }
        }

        // --- 1c) Position body segments along the trail ---
        for (size_t i = 0; i < m_snakeBody.size(); ++i) {
            size_t idx = (i + 1) * 6; // spacing along trail
            if (idx < m_snakeTrail.size()) {
                m_snakeBody[i] = m_snakeTrail[idx];
            } else {
                m_snakeBody[i] = m_snake.pos;
            }
        }

        // --- 1d) Food collision ---
        if (m_hasFood) {
            float d = glm::length(m_snake.pos - m_foodPos);
            if (d < m_foodRadius) {
                // grow: add one segment at the end
                glm::vec3 newSegPos = m_snake.pos;
                if (!m_snakeBody.empty())
                    newSegPos = m_snakeBody.back();
                m_snakeBody.push_back(newSegPos);

                m_hasFood = false;
                spawnFood();
            }
        }
    } else {
        //dead snake animation
        m_snakeDeathTime += deltaTime;
        if (m_snakeDeathTime > 0.6f) {   // same timing as your squash in paintGL
            resetSnake();                // puts snake back in center + clears body + food
        }
    }

    //door timer
    if (!m_doorOpened) {
        m_doorTimer += deltaTime;
        if (m_doorTimer >= m_doorOpenDelay) {
            openFrontDoor();          // removes cubes in front wall at z = -10
            buildInitialPathStrip();  // lays down the Minecraft-style strip + trees
            m_pathMode = true;
        }
    }

    //camera follow
    if (m_followSnake) {
        m_camPos  = m_snake.pos + m_camOffsetFromSnake;
        m_camLook = glm::normalize(-m_camOffsetFromSnake);
        m_camUp   = glm::vec3(0.f, 1.f, 0.f);
        m_camera.setViewMatrix(m_camPos, m_camLook, m_camUp);
    }

    update();
}





// DO NOT EDIT
void Realtime::saveViewportImage(std::string filePath) {
    // Make sure we have the right context and everything has been drawn
    makeCurrent();

    int fixedWidth = 1024;
    int fixedHeight = 768;

    // Create Frame Buffer
    GLuint fbo;
    glGenFramebuffers(1, &fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);

    // Create a color attachment texture
    GLuint texture;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, fixedWidth, fixedHeight, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texture, 0);

    // Optional: Create a depth buffer if your rendering uses depth testing
    GLuint rbo;
    glGenRenderbuffers(1, &rbo);
    glBindRenderbuffer(GL_RENDERBUFFER, rbo);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, fixedWidth, fixedHeight);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, rbo);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        std::cerr << "Error: Framebuffer is not complete!" << std::endl;
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        return;
    }

    // Render to the FBO
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    glViewport(0, 0, fixedWidth, fixedHeight);

    // Clear and render your scene here
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    paintGL();

    // Read pixels from framebuffer
    std::vector<unsigned char> pixels(fixedWidth * fixedHeight * 3);
    glReadPixels(0, 0, fixedWidth, fixedHeight, GL_RGB, GL_UNSIGNED_BYTE, pixels.data());

    // Unbind the framebuffer to return to default rendering to the screen
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // Convert to QImage
    QImage image(pixels.data(), fixedWidth, fixedHeight, QImage::Format_RGB888);
    QImage flippedImage = image.mirrored(); // Flip the image vertically

    // Save to file using Qt
    QString qFilePath = QString::fromStdString(filePath);
    if (!flippedImage.save(qFilePath)) {
        std::cerr << "Failed to save image to " << filePath << std::endl;
    }

    // Clean up
    glDeleteTextures(1, &texture);
    glDeleteRenderbuffers(1, &rbo);
    glDeleteFramebuffers(1, &fbo);
}
