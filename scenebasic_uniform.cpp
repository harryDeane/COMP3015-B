#include "scenebasic_uniform.h"

#include <glm/gtc/matrix_transform.hpp>
using glm::vec3;
using glm::mat4;
using glm::vec4;
using glm::mat3;

#include <cstdio>
#include <cstdlib>

#include <string>
using std::string;

#include <sstream>
#include <iostream>
using std::cerr;
using std::endl;

#include "helper/glutils.h"
#include "helper/texture.h"

#include <glm/gtx/intersect.hpp>

SceneBasic_Uniform::SceneBasic_Uniform() :
      tPrev(0), angle(90.0f), rotSpeed(0.0f), sky(100.0f), time(0.01f), plane(50.0f,50.0f,50.0f, 50.0f), meteorYPosition(15.0f), fallSpeed(3.0f), particleLifeTime(20.0f), nParticles(1000){
      meteor = ObjMesh::load("media/rocknew.obj", false, true);
      city = ObjMesh::load("media/smallcity.obj", false, true);

      // Initialize random number generator
      std::srand(static_cast<unsigned>(std::time(nullptr)));

      // Initialize ogres
      for (int i = 0; i < NUM_METEORS; i++) {
          spawnNewMeteor();
      }
}

// Add mouse click callback
void SceneBasic_Uniform::mouseButtonCallback(GLFWwindow* window, int button, int action, int mods) {
    if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS) {
        SceneBasic_Uniform* scene = static_cast<SceneBasic_Uniform*>(glfwGetWindowUserPointer(window));
        if (scene) {
            scene->checkMeteorClick();
        }
    }
}

// Add this method to check for meteor clicks
void SceneBasic_Uniform::checkMeteorClick() {
    // Get mouse position
    double xpos, ypos;
    glfwGetCursorPos(glfwGetCurrentContext(), &xpos, &ypos);

    // Convert screen coordinates to normalized device coordinates
    float x = (2.0f * xpos) / width - 1.0f;
    float y = 1.0f - (2.0f * ypos) / height;

    // Create ray in world space
    glm::vec4 rayClip(x, y, -1.0f, 1.0f);
    glm::vec4 rayEye = glm::inverse(projection) * rayClip;
    rayEye = glm::vec4(rayEye.x, rayEye.y, -1.0f, 0.0f);
    glm::vec3 rayWorld = glm::normalize(glm::vec3(glm::inverse(view) * rayEye));

    glm::vec3 rayOrigin = keyboardController.getCameraPosition();

    // Check intersection with each meteor
    for (auto& meteorInstance : meteors) {
        // Simple sphere collision (adjust radius as needed)
        float radius = 2.0f; // Meteor bounding sphere radius

        glm::vec3 oc = rayOrigin - meteorInstance.position;
        float a = glm::dot(rayWorld, rayWorld);
        float b = 2.0f * glm::dot(oc, rayWorld);
        float c = glm::dot(oc, oc) - radius * radius;
        float discriminant = b * b - 4 * a * c;

        if (discriminant >= 0) {
            // Hit! Start explosion
            meteorInstance.exploding = true;
            meteorInstance.explosionTime = 0.0f;
            meteorInstance.explosionSpeed = 1.0f / explosionDuration;

            // Move to exploding list (optional)
            explodingMeteors.push_back(meteorInstance);
            meteorInstance = meteors.back();
            meteors.pop_back();
            break; // Only explode one meteor per click
        }
    }
}

void SceneBasic_Uniform::initScene()
{
    view = glm::lookAt(keyboardController.getCameraPosition(),
        keyboardController.getCameraPosition() + keyboardController.getCameraDirection(),
        glm::vec3(0.0f, 1.0f, 0.0f));

    GLFWwindow* window = glfwGetCurrentContext();
    //glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    glfwSetCursorPosCallback(window, SceneBasic_Uniform::mouseCallback);

    glfwSetWindowUserPointer(window, this);
    glfwSetMouseButtonCallback(window, mouseButtonCallback);

    compile();
    

    glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    //glDepthMask(GL_FALSE);
    glDisable(GL_CULL_FACE);
    glEnable(GL_DEPTH_TEST);

   

    projection = mat4(1.0f);

    skyProg.use();
    GLuint cubeTex = Texture::loadHdrCubeMap("media/texture/cube/pisa-hdr/sky");
    

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_CUBE_MAP, cubeTex);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_CUBE_MAP, cubeTex);

    angle = glm::half_pi<float>();

    modelProg.use();
    modelProg.setUniform("Fog.Color", vec3(0.5f, 0.5f, 0.5f)); // Gray fog color
    modelProg.setUniform("Fog.MinDist", 30.0f); // Fog starts at 10 units
    modelProg.setUniform("Fog.MaxDist", 100.0f); // Fog fully covers at 50 units
    
    // Load textures
    GLuint mossTexture = Texture::loadTexture("media/texture/asteroidcolor.png");
    GLuint brickTexture = Texture::loadTexture("media/texture/asteroidnorm.png");

    asteroidColorTex = mossTexture;
    asteroidNormalTex = brickTexture;

    // Bind textures to texture units
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, mossTexture);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, brickTexture);
    

   
    // Load textures
    GLuint cityTexture = Texture::loadTexture("media/texture/smallcity.png");
    this->cityTexture = cityTexture;

    // Bind textures to texture units
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, cityTexture);


    initBuffers();
    prog.use();
    GLint texLoc = glGetUniformLocation(prog.getHandle(), "ParticleTex");
    if (texLoc != -1) {
        glUniform1i(texLoc, 0);  // Bind to texture unit 0
    }
    else {
        std::cerr << "Warning: ParticleTex uniform not found" << std::endl;
    }
    GLuint fireTexture = Texture::loadTexture("media/texture/smoke.png");
    this->fireTexture = fireTexture;
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, fireTexture);

    prog.setUniform("ParticleTex", 0);
    prog.setUniform("ParticleLifeTime", particleLifeTime);
    prog.setUniform("ParticleSize", 1.5f);
    prog.setUniform("Gravity", vec3(0.0f, -0.2f, 0.0f));
    prog.setUniform("EmitterPos", emitterPos);

    cameraPos = glm::vec3(0.0f, 2.0f, 5.0f);  // Start slightly above the ground
    cameraFront = glm::vec3(0.0f, 0.0f, -1.0f);  // Looking forward
    cameraUp = glm::vec3(0.0f, 1.0f, 0.0f);

    renderMeteors();
}

void SceneBasic_Uniform::compile()
{
	try {
		
        skyProg.compileShader("shader/skybox.vert");
        skyProg.compileShader("shader/skybox.frag");
        modelProg.compileShader("shader/model.vert");
        modelProg.compileShader("shader/model.frag");
        explosionProg.compileShader("shader/explosion.vert");
        explosionProg.compileShader("shader/explosion.geom");
        explosionProg.compileShader("shader/explosion.frag");
        prog.compileShader("shader/particles.vert");
        prog.compileShader("shader/particles.frag");
        modelProg.link();
        printShaderUniforms(modelProg);
        skyProg.link();
        printShaderUniforms(skyProg);
        explosionProg.link();
        printShaderUniforms(explosionProg);
        prog.link();
        printShaderUniforms(prog);
        
	} catch (GLSLProgramException &e) {
		cerr << e.what() << endl;
		exit(EXIT_FAILURE);
	}
}

void SceneBasic_Uniform::update(float t)
{
    time = t;
    angle = std::fmod(angle+0.01f,glm::two_pi<float>());
    float deltaT = t - tPrev;
    if (tPrev == 0.0f) deltaT = 0.0f;
    tPrev = t;

    angle += rotSpeed * deltaT;
    if (angle > glm::two_pi<float>()) {
        angle -= glm::two_pi<float>();
    }
    // Update the time variable
    time += deltaT;

    // Update all ogres
    for (auto& meteor : meteors) {
        if (meteor.position.y > GROUND_LEVEL) {
            meteor.position.y -= meteor.fallSpeed * deltaT;
            meteor.emitterPos = meteor.position; // Update particle emitter position
        }
        else {
            // Instead of respawning, recycle the meteor
            meteor.position = glm::vec3(
                (std::rand() % 40) - 20.0f,
                SPAWN_HEIGHT,
                (std::rand() % 40) - 20.0f
            );
            meteor.emitterPos = meteor.position;
            meteor.rotationAngle = 0.0f;
            meteor.fallSpeed = 3.0f + (std::rand() % 100) / 50.0f;

            // Reinitialize particle system
            initMeteorParticleSystem(meteor);
        }
        meteor.rotationAngle += deltaT * 45.0f;
    }

    // Update exploding meteors
    for (auto it = explodingMeteors.begin(); it != explodingMeteors.end(); ) {
        it->explosionTime += it->explosionSpeed * deltaT;

        if (it->explosionTime >= 1.0f) {
            // Explosion complete - respawn
            spawnNewMeteor();
            it = explodingMeteors.erase(it);
        }
        else {
            ++it;
        }
    }

    float deltaTime = 0.01f;
    keyboardController.processInput(glfwGetCurrentContext(), deltaTime);

    view = glm::lookAt(keyboardController.getCameraPosition(),
        keyboardController.getCameraPosition() + keyboardController.getCameraDirection(),
        glm::vec3(0.0f, 1.0f, 0.0f));
}

void SceneBasic_Uniform::render()
{
    prog.setUniform("Time", time);


    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    vec3 cameraPos = vec3(7.0f*cos(angle), 2.0f, 7.0f*sin(angle));
    skyProg.use();
    model = mat4(1.0f);
    setMatrices(skyProg);
    sky.render();

    // Render particles for each meteor
    prog.use();
    prog.setUniform("Time", time);
    prog.setUniform("ProjectionMatrix", projection);
    prog.setUniform("ParticleSize", 5.0f);

    // Bind texture
    GLuint fireTexture = Texture::loadTexture("media/texture/smoke.png");
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, fireTexture);
    prog.setUniform("ParticleTex", 0);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDepthMask(GL_FALSE);

    for (const auto& meteorInstance : meteors) {
        // Update emitter position to follow the meteor
        prog.setUniform("EmitterPos", meteorInstance.position);

        // Set ModelView matrix
        mat4 modelView = view * glm::translate(mat4(1.0f), meteorInstance.position);
        prog.setUniform("ModelViewMatrix", modelView);

        // Draw particles for this meteor
        glBindVertexArray(meteorInstance.particlesVAO);
        glDrawArraysInstanced(GL_TRIANGLES, 0, 6, meteorInstance.nParticles);
    }

    glBindVertexArray(0);
    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);


    modelProg.use();
    modelProg.setUniform("IsPlane", true); // Indicate that this is the plane

    // Bind city texture
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, cityTexture);
    modelProg.setUniform("cityTexture", 0); // Texture unit 0

    model = mat4(1.0f);
    model = glm::translate(model, vec3(0.0f, -10.0f, 0.0f));
    model = glm::scale(model, vec3(50.0f));
    setMatrices(modelProg);
    city->render();


    modelProg.use();

    #define MAX_LIGHTS 3
    vec3 lightPositions[MAX_LIGHTS] = {
        vec3(0.5f, 1.0f, 0.5f),  // left backward
        vec3(0.0f, 1.0f, 0.0f), // Slightly to the left
        vec3(0.5f, 1.0f, -0.5f)   // Centered above
    };

    vec3 lightDirections[MAX_LIGHTS] = {
        normalize(vec3(0.0f) - lightPositions[0]), // Point toward origin
        normalize(vec3(0.0f) - lightPositions[1]),
        normalize(vec3(0.0f) - lightPositions[2])
    };

    // Set light positions and directions
    for (int i = 0; i < MAX_LIGHTS; i++) {
        std::string lightName = "Lights[" + std::to_string(i) + "]";
        modelProg.setUniform((lightName + ".Position").c_str(), vec4(lightPositions[i], 1.0f));
        modelProg.setUniform((lightName + ".La").c_str(), vec3(0.1f)); // Ambient light intensity
        modelProg.setUniform((lightName + ".L").c_str(), vec3(1.0f)); // Diffuse and specular light intensity
        modelProg.setUniform((lightName + ".Direction").c_str(), lightDirections[i]);
        modelProg.setUniform((lightName + ".Cutoff").c_str(), glm::radians(50.0f)); // Spotlight cutoff angle
    }

    // Set material uniforms
    modelProg.setUniform("Material.Kd", vec3(0.8f)); // Diffuse reflectivity
    modelProg.setUniform("Material.Ka", vec3(0.2f)); // Ambient reflectivity
    modelProg.setUniform("Material.Ks", vec3(0.5f)); // Specular reflectivity
    modelProg.setUniform("Material.Shininess", 32.0f); // Specular shininess

    renderMeteors();
}

void SceneBasic_Uniform::renderMeteors() {
    // Render regular meteors
    modelProg.use();

    modelProg.setUniform("IsPlane", false); // Indicate that this is not the plane

    // Bind asteroid textures
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, asteroidColorTex);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, asteroidNormalTex);

    modelProg.setUniform("ColorTex", 0); // Texture unit 0
    modelProg.setUniform("NormalMapTex", 1); // Texture unit 1

#define MAX_LIGHTS 3
    vec3 lightPositions[MAX_LIGHTS] = {
        vec3(0.5f, 1.0f, 0.5f),  // left backward
        vec3(0.0f, 1.0f, 0.0f), // Slightly to the left
        vec3(0.5f, 1.0f, -0.5f)   // Centered above
    };

    vec3 lightDirections[MAX_LIGHTS] = {
        normalize(vec3(0.0f) - lightPositions[0]), // Point toward origin
        normalize(vec3(0.0f) - lightPositions[1]),
        normalize(vec3(0.0f) - lightPositions[2])
    };

    // Set light positions and directions
    for (int i = 0; i < MAX_LIGHTS; i++) {
        std::string lightName = "Lights[" + std::to_string(i) + "]";
        modelProg.setUniform((lightName + ".Position").c_str(), vec4(lightPositions[i], 1.0f));
        modelProg.setUniform((lightName + ".La").c_str(), vec3(0.1f)); // Ambient light intensity
        modelProg.setUniform((lightName + ".L").c_str(), vec3(1.0f)); // Diffuse and specular light intensity
        modelProg.setUniform((lightName + ".Direction").c_str(), lightDirections[i]);
        modelProg.setUniform((lightName + ".Cutoff").c_str(), glm::radians(50.0f)); // Spotlight cutoff angle
    }

    // Set material uniforms
    modelProg.setUniform("Material.Kd", vec3(0.8f)); // Diffuse reflectivity
    modelProg.setUniform("Material.Ka", vec3(0.2f)); // Ambient reflectivity
    modelProg.setUniform("Material.Ks", vec3(0.5f)); // Specular reflectivity
    modelProg.setUniform("Material.Shininess", 32.0f); // Specular shininess

    for (const auto& meteorInstance : meteors) {
        model = mat4(1.0f);
        model = glm::translate(model, meteorInstance.position);
        model = glm::scale(model, vec3(2.0f));
        model = glm::rotate(model,
            glm::radians(meteorInstance.rotationAngle),
            meteorInstance.rotationAxis);

        setMatrices(modelProg);
        meteor->render();
    }

    // Render exploding meteors with geometry shader
    explosionProg.use();
    explosionProg.setUniform("mossTexture", 0);
    explosionProg.setUniform("brickTexture", 1);

    for (const auto& meteorInstance : explodingMeteors) {
        model = mat4(1.0f);
        model = glm::translate(model, meteorInstance.position);
        model = glm::scale(model, vec3(2.0f));
        model = glm::rotate(model,
            glm::radians(meteorInstance.rotationAngle),
            meteorInstance.rotationAxis);

        explosionProg.setUniform("ExplosionFactor", meteorInstance.explosionTime);
        setMatrices(explosionProg);
        meteor->render();
    }

}

void SceneBasic_Uniform::spawnNewMeteor() {
    FallingMeteor newMeteor;
    newMeteor.position = glm::vec3(
        (std::rand() % 40) - 20.0f,
        SPAWN_HEIGHT,
        (std::rand() % 40) - 20.0f
    );
    newMeteor.rotationAxis = glm::normalize(glm::vec3(
        (std::rand() % 100) / 100.0f,
        (std::rand() % 100) / 100.0f,
        (std::rand() % 100) / 100.0f
    ));
    newMeteor.rotationAngle = 0.0f;
    newMeteor.fallSpeed = 3.0f + (std::rand() % 100) / 50.0f;

    // Initialize particle system for this meteor
    newMeteor.emitterPos = newMeteor.position;
    newMeteor.emitterDir = glm::vec3(0, -5, 0); // Or customize per meteor
    newMeteor.nParticles = 500; // Fewer particles per meteor

    // Initialize particle buffers for this meteor
    initMeteorParticleSystem(newMeteor);

    meteors.push_back(newMeteor);
}

void SceneBasic_Uniform::resize(int w, int h)
{
    width = w;
    height = h;
    glViewport(0,0,w,h);

    projection = glm::perspective(glm::radians(70.0f), (float)w / h, 0.3f, 100.0f);
}

void SceneBasic_Uniform::mouseCallback(GLFWwindow* window, double xpos, double ypos) {
    SceneBasic_Uniform* scene = static_cast<SceneBasic_Uniform*>(glfwGetWindowUserPointer(window));
    if (scene) {
        scene->keyboardController.processMouseMovement(xpos, ypos);
    }
}

void SceneBasic_Uniform::setMatrices(GLSLProgram &p)
{
    mat4 mv = view*model;

    p.setUniform("ModelMatrix", model);
    p.setUniform("ViewMatrix", view);
    p.setUniform("ModelViewMatrix", mv);
    p.setUniform("ProjectionMatrix", projection);
    //prog.setUniform("NormalMatrix", glm::mat3(vec3(mv[01]), vec3(mv[1]), vec3(mv[2])));
    p.setUniform("MVP",projection* mv);
}

void SceneBasic_Uniform::initBuffers() {
    glGenBuffers(1, &initVel);
    glGenBuffers(1, &startTime);
    int size = nParticles * sizeof(float);
    glBindBuffer(GL_ARRAY_BUFFER, initVel);
    glBufferData(GL_ARRAY_BUFFER, size * 3, 0, GL_STATIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, startTime);
    glBufferData(GL_ARRAY_BUFFER, size, 0, GL_STATIC_DRAW);

    glm::mat3 emitterBasis = ParticleUtils::makeArbitraryBasis(emitterDir);
    vec3 v(0.0f);
    float velocity, theta, phi;
    std::vector<GLfloat>data(nParticles * 3);

    for (uint32_t i = 0; i < nParticles; i++) {
        theta = glm::mix(0.0f, glm::pi<float>() / 20.0f, randFloat());
        phi = glm::mix(0.0f, glm::two_pi<float>(), randFloat());
        v.x = sinf(theta) * cosf(phi);
        v.y = cosf(theta);
        v.z = sinf(theta) * sinf(phi);
        velocity = glm::mix(1.25f, 1.5f, randFloat());
        v = normalize(emitterBasis * v) * velocity;
        data[3 * i] = v.x;
        data[3 * i + 1] = v.y;
        data[3 * i + 2] = v.z;
    }
    glBindBuffer(GL_ARRAY_BUFFER, initVel);
    glBufferSubData(GL_ARRAY_BUFFER, 0, size * 3, data.data());
    float rate = particleLifeTime / nParticles;
    for (int i = 0; i < nParticles; i++) {
        data[i] = rate * i;
    }
    glBindBuffer(GL_ARRAY_BUFFER, startTime);
    glBufferSubData(GL_ARRAY_BUFFER, 0, nParticles * sizeof(float), data.data());
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glGenVertexArrays(1, &particles);
    glBindVertexArray(particles);

    // Bind initVel buffer, and set vertex attribute for velocity
    glBindBuffer(GL_ARRAY_BUFFER, initVel);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(0);

    // Bind startTime buffer, and set vertex attribute for time
    glBindBuffer(GL_ARRAY_BUFFER, startTime);
    glVertexAttribPointer(1, 1, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(1);

    // Enable instancing
    glVertexAttribDivisor(0, 1);
    glVertexAttribDivisor(1, 1);

    glBindVertexArray(0);  // Unbind VAO after setting everything up

}

float SceneBasic_Uniform::randFloat() {
    return rand.nextFloat();
}

void SceneBasic_Uniform::printShaderUniforms(GLSLProgram &prog) {
    GLint count;
    glGetProgramiv(prog.getHandle(), GL_ACTIVE_UNIFORMS, &count);

    printf("Active Uniforms: %d\n", count);

    GLint size;
    GLenum type;
    GLchar name[256];
    for (GLint i = 0; i < count; i++) {
        glGetActiveUniform(prog.getHandle(), i, sizeof(name), NULL, &size, &type, name);
        printf("Uniform #%d: %s (Type: %x, Size: %d)\n", i, name, type, size);
    }
}

void SceneBasic_Uniform::initMeteorParticleSystem(FallingMeteor& meteor) {
    // Generate buffers
    glGenBuffers(1, &meteor.initVelBuffer);
    glGenBuffers(1, &meteor.startTimeBuffer);

    int size = meteor.nParticles * sizeof(float);
    glBindBuffer(GL_ARRAY_BUFFER, meteor.initVelBuffer);
    glBufferData(GL_ARRAY_BUFFER, size * 3, 0, GL_STATIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, meteor.startTimeBuffer);
    glBufferData(GL_ARRAY_BUFFER, size, 0, GL_STATIC_DRAW);

    glm::mat3 emitterBasis = ParticleUtils::makeArbitraryBasis(meteor.emitterDir);
    vec3 v(0.0f);
    float velocity, theta, phi;
    std::vector<GLfloat> data(meteor.nParticles * 3);

    for (uint32_t i = 0; i < meteor.nParticles; i++) {
        theta = glm::mix(0.0f, glm::pi<float>() / 20.0f, randFloat());
        phi = glm::mix(0.0f, glm::two_pi<float>(), randFloat());
        v.x = sinf(theta) * cosf(phi);
        v.y = cosf(theta);
        v.z = sinf(theta) * sinf(phi);
        velocity = glm::mix(1.25f, 1.5f, randFloat());
        v = normalize(emitterBasis * v) * velocity;
        data[3 * i] = v.x;
        data[3 * i + 1] = v.y;
        data[3 * i + 2] = v.z;
    }

    glBindBuffer(GL_ARRAY_BUFFER, meteor.initVelBuffer);
    glBufferSubData(GL_ARRAY_BUFFER, 0, size * 3, data.data());

    float rate = particleLifeTime / meteor.nParticles;
    for (int i = 0; i < meteor.nParticles; i++) {
        data[i] = rate * i;
    }

    glBindBuffer(GL_ARRAY_BUFFER, meteor.startTimeBuffer);
    glBufferSubData(GL_ARRAY_BUFFER, 0, meteor.nParticles * sizeof(float), data.data());
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    // Create VAO
    glGenVertexArrays(1, &meteor.particlesVAO);
    glBindVertexArray(meteor.particlesVAO);

    glBindBuffer(GL_ARRAY_BUFFER, meteor.initVelBuffer);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(0);

    glBindBuffer(GL_ARRAY_BUFFER, meteor.startTimeBuffer);
    glVertexAttribPointer(1, 1, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(1);

    glVertexAttribDivisor(0, 1);
    glVertexAttribDivisor(1, 1);

    glBindVertexArray(0);
}