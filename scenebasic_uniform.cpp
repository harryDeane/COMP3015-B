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
      tPrev(0), angle(90.0f), rotSpeed(0.0f), sky(100.0f), time(0.01f), plane(50.0f,50.0f,50.0f, 50.0f), meteorYPosition(15.0f), fallSpeed(3.0f){
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
    glEnable(GL_DEPTH_TEST);
    projection = mat4(1.0f);

    modelProg.use();
    modelProg.setUniform("Fog.Color", vec3(0.5f, 0.5f, 0.5f)); // Gray fog color
    modelProg.setUniform("Fog.MinDist", 30.0f); // Fog starts at 10 units
    modelProg.setUniform("Fog.MaxDist", 100.0f); // Fog fully covers at 50 units

    GLuint cubeTex = Texture::loadHdrCubeMap("media/texture/cube/pisa-hdr/sky");
    

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_CUBE_MAP, cubeTex);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_CUBE_MAP, cubeTex);

    // Load textures
    GLuint mossTexture = Texture::loadTexture("media/texture/asteroidcolor.png");
    GLuint brickTexture = Texture::loadTexture("media/texture/asteroidnorm.png");

    // Bind textures to texture units
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, mossTexture);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, brickTexture);
 

    // Load textures
    GLuint cityTexture = Texture::loadTexture("media/texture/smallcity.png");

    // Bind textures to texture units
    glActiveTexture(GL_TEXTURE3);
    glBindTexture(GL_TEXTURE_2D, cityTexture);

    cameraPos = glm::vec3(0.0f, 2.0f, 5.0f);  // Start slightly above the ground
    cameraFront = glm::vec3(0.0f, 0.0f, -1.0f);  // Looking forward
    cameraUp = glm::vec3(0.0f, 1.0f, 0.0f);

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
        modelProg.link();
        skyProg.link();
        explosionProg.link();
        
	} catch (GLSLProgramException &e) {
		cerr << e.what() << endl;
		exit(EXIT_FAILURE);
	}
}

void SceneBasic_Uniform::update(float t)
{
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
    for (auto& meteorInstance : meteors) {
        // Only fall if above ground
        if (meteorInstance.position.y > GROUND_LEVEL) {
            meteorInstance.position.y -= meteorInstance.fallSpeed * deltaT;
        }
        else {
            // Respawn at top when hitting ground
            spawnNewMeteor();
            meteorInstance = meteors.back();
            meteors.pop_back();
        }
        meteorInstance.rotationAngle += deltaT * 45.0f; // Rotate 45 degrees per second
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
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    vec3 cameraPos = vec3(7.0f*cos(angle), 2.0f, 7.0f*sin(angle));
    skyProg.use();
    model = mat4(1.0f);
    setMatrices(skyProg);
    sky.render();

    modelProg.use();
    modelProg.setUniform("IsPlane", true); // Indicate that this is the plane
    modelProg.setUniform("cityTexture", 0); // Texture unit 0
    model = mat4(1.0f);
    model = glm::translate(model, vec3(0.0f, -10.0f, 0.0f)); // Position the plane underneath the model
    model = glm::scale(model, vec3(50.0f)); 
    setMatrices(modelProg);
    city->render();

    modelProg.use();
    modelProg.setUniform("IsPlane", false); // Indicate that this is not the plane
    modelProg.setUniform("mossTexture", 0); // Texture unit 0
    modelProg.setUniform("brickTexture", 1); // Texture unit 1

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

    // Render regular meteors
    modelProg.use();
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
        (std::rand() % 40) - 20.0f,  // Random X between -20 and 20
        SPAWN_HEIGHT,
        (std::rand() % 40) - 20.0f   // Random Z between -20 and 20
    );
    newMeteor.rotationAxis = glm::normalize(glm::vec3(
        (std::rand() % 100) / 100.0f,
        (std::rand() % 100) / 100.0f,
        (std::rand() % 100) / 100.0f
    ));
    newMeteor.rotationAngle = 0.0f;
    newMeteor.fallSpeed = 3.0f + (std::rand() % 100) / 50.0f; // Between 3-5
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
    p.setUniform("ProjectionMatrix", projection);
    //prog.setUniform("NormalMatrix", glm::mat3(vec3(mv[01]), vec3(mv[1]), vec3(mv[2])));
    p.setUniform("MVP",projection* mv);
}

