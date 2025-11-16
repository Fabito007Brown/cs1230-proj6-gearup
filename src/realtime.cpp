#include "realtime.h"

#include <QCoreApplication>
#include <QMouseEvent>
#include <QKeyEvent>
#include <iostream>
#include "settings.h"
#include <algorithm>
#include <glm/gtc/matrix_transform.hpp>

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
}

void Realtime::finish() {
    killTimer(m_timer);
    this->makeCurrent();

    // Students: anything requiring OpenGL calls when the program exits should be done here

    cleanupVAOs();
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
    glEnable(GL_CULL_FACE);
    glViewport(0, 0, size().width() * m_devicePixelRatio, size().height() * m_devicePixelRatio);

    // Load shader
    m_shader = ShaderLoader::createShaderProgram(
        ":/resources/shaders/default.vert",
        ":/resources/shaders/default.frag"
        );

    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
}


void Realtime::paintGL() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    if (!m_shader) return;

    glUseProgram(m_shader);


    glm::mat4 view = m_camera.getViewMatrix();
    glm::mat4 proj = m_camera.getProjMatrix();

    GLint viewLoc = glGetUniformLocation(m_shader, "view");
    GLint projLoc = glGetUniformLocation(m_shader, "proj");
    glUniformMatrix4fv(viewLoc, 1, GL_FALSE, &view[0][0]);
    glUniformMatrix4fv(projLoc, 1, GL_FALSE, &proj[0][0]);

    // --- camera position ---
    GLint camPosLoc = glGetUniformLocation(m_shader, "camPos");
    glUniform3fv(camPosLoc, 1, &m_camPos[0]);

    // --- global coefficients ---
    const SceneGlobalData &G = m_renderData.globalData;
    glUniform1f(glGetUniformLocation(m_shader, "k_a"), G.ka);
    glUniform1f(glGetUniformLocation(m_shader, "k_d"), G.kd);
    glUniform1f(glGetUniformLocation(m_shader, "k_s"), G.ks);


    // --- lights ---
    int numLights = std::min<int>(m_renderData.lights.size(), 8);
    glUniform1i(glGetUniformLocation(m_shader, "numLights"), numLights);

    for (int i = 0; i < numLights; ++i) {
        const SceneLightData &L = m_renderData.lights[i];

        // 0 point, 1 directional, 2 spot
        int typeInt = 0;
        if (L.type == LightType::LIGHT_DIRECTIONAL) typeInt = 1;
        else if (L.type == LightType::LIGHT_SPOT)  typeInt = 2;


        auto loc = [&](const std::string &field) {
            std::string name = "lights[" + std::to_string(i) + "]." + field;
            return glGetUniformLocation(m_shader, name.c_str());
        };

        glUniform1i(loc("type"), typeInt);
        glUniform3fv(loc("color"), 1, &glm::vec3(L.color)[0]);
        glUniform3fv(loc("pos"),   1, &glm::vec3(L.pos)[0]);
        glUniform3fv(loc("dir"),   1, &glm::vec3(L.dir)[0]);
        glUniform3fv(loc("atten"), 1, &glm::vec3(L.function)[0]);
        glUniform1f(loc("angle"),      L.angle);
        glUniform1f(loc("penumbra"),   L.penumbra);
    }

    // single point light: use first light in the scene
    GLint lightPosLoc   = glGetUniformLocation(m_shader, "lightPos");
    GLint lightColorLoc = glGetUniformLocation(m_shader, "lightColor");

    glm::vec3 lightPos(5.f, 5.f, 5.f);
    glm::vec3 lightCol(1.f, 1.f, 1.f);

    if (!m_renderData.lights.empty()) {
        const SceneLightData &L0 = m_renderData.lights[0];
        lightPos = glm::vec3(L0.pos);
        lightCol = glm::vec3(L0.color);
    }

    glUniform3fv(lightPosLoc,   1, &lightPos[0]);
    glUniform3fv(lightColorLoc, 1, &lightCol[0]);


    for (size_t i = 0; i < m_renderData.shapes.size(); i++) {
        const RenderShapeData &shape = m_renderData.shapes[i];
        const ShapeVAO &vao = m_shapeVAOs[i];

        // model matrix
        GLint modelLoc = glGetUniformLocation(m_shader, "model");
        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, &shape.ctm[0][0]);

        // material
        const SceneMaterial &mat = shape.primitive.material;

        glm::vec3 cA = glm::vec3(mat.cAmbient);
        glm::vec3 cD = glm::vec3(mat.cDiffuse);
        glm::vec3 cS = glm::vec3(mat.cSpecular);

        glUniform3fv(glGetUniformLocation(m_shader, "cAmbient"),  1, &cA[0]);
        glUniform3fv(glGetUniformLocation(m_shader, "cDiffuse"),  1, &cD[0]);
        glUniform3fv(glGetUniformLocation(m_shader, "cSpecular"), 1, &cS[0]);
        glUniform1f(glGetUniformLocation(m_shader, "shininess"),  mat.shininess);

        // draw
        glBindVertexArray(vao.vao);
        glDrawArrays(GL_TRIANGLES, 0, vao.vertexCount);
        glBindVertexArray(0);
    }

    glUseProgram(0);
}

void Realtime::resizeGL(int w, int h) {
    // Tells OpenGL how big the screen is
    glViewport(0, 0, size().width() * m_devicePixelRatio, size().height() * m_devicePixelRatio);

    // Students: anything requiring OpenGL calls when the program starts should be done here

    // Keep camera projection in sync with window size
    float aspect = (h > 0) ? float(w) / float(h) : 1.f;
    float fovY = !m_renderData.shapes.empty()
                     ? m_renderData.cameraData.heightAngle
                     : 45.f * 3.1415926535f / 180.f;

    m_camera.setProjectionMatrix(
        aspect,
        settings.nearPlane,
        settings.farPlane,
        fovY
        );
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
    m_keyMap[Qt::Key(event->key())] = true;
}

void Realtime::keyReleaseEvent(QKeyEvent *event) {
    m_keyMap[Qt::Key(event->key())] = false;
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
    int elapsedms   = m_elapsedTimer.elapsed();
    float deltaTime = elapsedms * 0.001f;
    m_elapsedTimer.restart();

    // Use deltaTime and m_keyMap here to move around

    const float speed = 5.f; // world units per second

    glm::vec3 forward = glm::normalize(m_camLook);
    glm::vec3 right   = glm::normalize(glm::cross(forward, m_camUp));
    glm::vec3 worldUp(0.f, 1.f, 0.f);

    glm::vec3 moveDir(0.f);

    // W/S: along look direction
    if (m_keyMap[Qt::Key_W]) moveDir += forward;
    if (m_keyMap[Qt::Key_S]) moveDir -= forward;

    // A/D: strafe left/right
    if (m_keyMap[Qt::Key_D]) moveDir += right;
    if (m_keyMap[Qt::Key_A]) moveDir -= right;

    // Space / Ctrl: vertical in world space
    if (m_keyMap[Qt::Key_Space])   moveDir += worldUp;
    if (m_keyMap[Qt::Key_Control]) moveDir -= worldUp;

    if (glm::length(moveDir) > 0.f) {
        moveDir = glm::normalize(moveDir);
        m_camPos += moveDir * speed * deltaTime;

        // Push updated camera to Camera object
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
