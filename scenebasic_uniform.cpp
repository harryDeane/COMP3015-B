#include "scenebasic_uniform.h"

#include <glm/gtc/matrix_transform.hpp>
using glm::vec3;
using glm::mat4;
using glm::vec4;
using glm::mat3;

#include <cstdio>
#include <cstdlib>
#include <ctime>
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
      tPrev(0), angle(0.0f),drawBuf(1),deltaT(0), rotSpeed(0.0f), sky(100.0f), time(0.01f), plane(50.0f,50.0f,50.0f, 50.0f), meteorYPosition(15.0f), fallSpeed(3.0f), particleLifetime(10.5f), nParticles(1000), emitterPos(1,0,0),emitterDir(-1,2,0){
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
        // Simple sphere collision
        float radius = 2.0f; // Meteor bounding sphere radius

        glm::vec3 oc = rayOrigin - meteorInstance.position;
        float a = glm::dot(rayWorld, rayWorld);
        float b = 2.0f * glm::dot(oc, rayWorld);
        float c = glm::dot(oc, oc) - radius * radius;
        float discriminant = b * b - 4 * a * c;

        if (discriminant >= 0) {
            // Start explosion
            meteorInstance.exploding = true;
            meteorInstance.explosionTime = 0.0f;
            meteorInstance.explosionSpeed = 1.0f / explosionDuration;

            // Move to exploding list
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

	model = mat4(1.0f);
    glActiveTexture(GL_TEXTURE0);
   
    prog.use();
	
    GLuint fireTexture = Texture::loadTexture("media/texture/smoke.png");
    glActiveTexture(GL_TEXTURE1);
    ParticleUtils::createRandomTex1D(nParticles * 3);
    glBindTexture(GL_TEXTURE_2D, fireTexture);
    initBuffers();
    prog.setUniform("RandomTex", 1);
    prog.setUniform("ParticleTex", 0);
    prog.setUniform("ParticleLifetime", particleLifetime);
    prog.setUniform("ParticleSize", 1.5f);
    prog.setUniform("Accel", vec3(0.0f, 0.2f, 0.0f));
    prog.setUniform("EmitterPos", emitterPos);
	prog.setUniform("EmitterBasis", ParticleUtils::makeArbitraryBasis(emitterDir));

    cameraPos = glm::vec3(0.0f, 2.0f, 5.0f);  // Start slightly above the ground
    cameraFront = glm::vec3(0.0f, 0.0f, -1.0f);  // Looking forward
    cameraUp = glm::vec3(0.0f, 1.0f, 0.0f);

    renderMeteors();


	spriteProg.use();
    numSprites = 50;
    locations = new float[numSprites*3];
    std::srand(static_cast<unsigned>(std::time(nullptr)));

    for (int i = 0; i < numSprites; i++) {
        vec3 p(
            ((float)std::rand() / RAND_MAX * 20.0f) - 10.0f,  // X: -10 to 10
            -28.0f,                                             // Y: 0 (ground level)
            ((float)std::rand() / RAND_MAX * 20.0f) - 10.0f   // Z: -10 to 10
        );
        locations[i * 3] = p.x;
        locations[i * 3 + 1] = p.y;
        locations[i * 3 + 2] = p.z;
    }

    GLuint handle;
	glGenBuffers(1, &handle);
	glBindBuffer(GL_ARRAY_BUFFER, handle);
	glBufferData(GL_ARRAY_BUFFER, numSprites * 3 * sizeof(float), locations, GL_STATIC_DRAW);
	delete[] locations;
	glGenVertexArrays(1, &sprites);
	glBindVertexArray(sprites);
	glBindBuffer(GL_ARRAY_BUFFER, handle);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, ((GLubyte*)NULL + (0)));
	glEnableVertexAttribArray(0);
	glBindVertexArray(0);
	const char* texName = "media/texture/fire.png";
	Texture::loadTexture(texName);
	spriteProg.setUniform("SpriteTex", 0);
    spriteProg.setUniform("Size2", 0.15f);
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
		spriteProg.compileShader("shader/sprite.vert");
        spriteProg.compileShader("shader/sprite.frag");
        spriteProg.compileShader("shader/sprite.gs");

		GLuint progHandle = prog.getHandle();
		const char* outputNames[] = { "Position", "Velocity", "Age" };
		glTransformFeedbackVaryings(progHandle, 3, outputNames, GL_SEPARATE_ATTRIBS);

        modelProg.link();
        printShaderUniforms(modelProg);
        skyProg.link();
        printShaderUniforms(skyProg);
        explosionProg.link();
        printShaderUniforms(explosionProg);
        prog.link();
        printShaderUniforms(prog);
		spriteProg.link();
		printShaderUniforms(spriteProg);
        
	} catch (GLSLProgramException &e) {
		cerr << e.what() << endl;
		exit(EXIT_FAILURE);
	}
}

void SceneBasic_Uniform::update(float t)
{
    deltaT = t - tPrev;  // Make sure this is set before rendering
    

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

    spriteProg.use();
    model = mat4(1.0f);
    setMatrices(spriteProg);
    GLuint spriteTex = Texture::loadTexture("media/texture/fire.png");
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, spriteTex);
    spriteProg.setUniform("SpriteTex", 0);
    spriteProg.setUniform("Size2", 0.15f);
   
    glBindVertexArray(sprites);
    glDrawArrays(GL_POINTS, 0, numSprites);
    glFinish();

    skyProg.use();
    model = mat4(1.0f);
    setMatrices(skyProg);
    sky.render();

   

    // Render particles
    prog.use();
    prog.setUniform("Time", time);
    prog.setUniform("DeltaT", deltaT);
    prog.setUniform("Pass", 1);

	glEnable(GL_RASTERIZER_DISCARD);
	glBindTransformFeedback(GL_TRANSFORM_FEEDBACK, feedback[drawBuf]);
	glBeginTransformFeedback(GL_POINTS);
	glBindVertexArray(particleArray[1 - drawBuf]);
	glVertexAttribDivisor(0, 0);
    glVertexAttribDivisor(1, 0);
    glVertexAttribDivisor(2, 0);
    glDrawArrays(GL_POINTS, 0, nParticles);
	glBindVertexArray(0);
	glEndTransformFeedback();
	glDisable(GL_RASTERIZER_DISCARD);

    prog.setUniform("Pass", 2);
    
    // Set required uniforms
    prog.setUniform("Time", time);
    prog.setUniform("ModelViewMatrix", view * model);
    prog.setUniform("ProjectionMatrix", projection);
    prog.setUniform("EmitterPos", emitterPos);

    // Bind texture
    GLuint fireTexture = Texture::loadTexture("media/texture/smoke.png");
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, fireTexture);
    prog.setUniform("ParticleTex", 0);

    // Render with blending
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDepthMask(GL_FALSE);

    glBindVertexArray(particleArray[drawBuf]);
    glVertexAttribDivisor(0, 1);
    glVertexAttribDivisor(1, 1);
    glVertexAttribDivisor(2, 1);
    glDrawArraysInstanced(GL_TRIANGLES, 0, 6, nParticles);
    glBindVertexArray(0);

    glDepthMask(GL_TRUE);
    drawBuf = 1 - drawBuf;
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
    p.setUniform("ModelViewMatrix", mv);
    p.setUniform("ProjectionMatrix", projection);
    //prog.setUniform("NormalMatrix", glm::mat3(vec3(mv[01]), vec3(mv[1]), vec3(mv[2])));
    p.setUniform("MVP",projection* mv);
}

void SceneBasic_Uniform::initBuffers() {
    glGenBuffers(2, posBuf);
    glGenBuffers(2, velBuf);
	glGenBuffers(2, age);

    int size = nParticles * 3*sizeof(GLfloat);
    glBindBuffer(GL_ARRAY_BUFFER, posBuf[0]);
    glBufferData(GL_ARRAY_BUFFER, size, 0, GL_DYNAMIC_COPY);
    glBindBuffer(GL_ARRAY_BUFFER, posBuf[1]);
    glBufferData(GL_ARRAY_BUFFER, size, 0, GL_DYNAMIC_COPY);
    glBindBuffer(GL_ARRAY_BUFFER, velBuf[0]);
    glBufferData(GL_ARRAY_BUFFER, size, 0, GL_DYNAMIC_COPY);
    glBindBuffer(GL_ARRAY_BUFFER, velBuf[1]);
    glBufferData(GL_ARRAY_BUFFER, size, 0, GL_DYNAMIC_COPY);
    glBindBuffer(GL_ARRAY_BUFFER, age[0]);
    glBufferData(GL_ARRAY_BUFFER, nParticles * sizeof(float), 0, GL_DYNAMIC_COPY);
    glBindBuffer(GL_ARRAY_BUFFER, age[1]);
    glBufferData(GL_ARRAY_BUFFER, nParticles * sizeof(float), 0, GL_DYNAMIC_COPY);

    std::vector<GLfloat>tempData(nParticles);
    float rate = particleLifetime / nParticles;
    for (int i = 0; i < nParticles; i++) {
        tempData[i] = rate * i + nParticles;  // Start with some positive ages
    }
    
    glBindBuffer(GL_ARRAY_BUFFER, age[0]);
    glBufferSubData(GL_ARRAY_BUFFER, 0, nParticles * sizeof(float), tempData.data());
    glBindBuffer(GL_ARRAY_BUFFER, 0);
   
    glGenVertexArrays(2, particleArray);
    
    // Particle Array 0
    glBindVertexArray(particleArray[0]);
    glBindBuffer(GL_ARRAY_BUFFER, posBuf[0]);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(0);

    glBindBuffer(GL_ARRAY_BUFFER, velBuf[0]);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(1);

    glBindBuffer(GL_ARRAY_BUFFER, age[0]);
    glVertexAttribPointer(2, 1, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(2);
   
    // Particle Array 1
    glBindVertexArray(particleArray[1]);
    glBindBuffer(GL_ARRAY_BUFFER, posBuf[1]);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(0);

    glBindBuffer(GL_ARRAY_BUFFER, velBuf[1]);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(1);

    glBindBuffer(GL_ARRAY_BUFFER, age[1]);
    glVertexAttribPointer(2, 1, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(2);

    glBindVertexArray(0);

	glGenTransformFeedbacks(2, feedback);
    glBindTransformFeedback(GL_TRANSFORM_FEEDBACK, feedback[0]);
	glBindBufferBase(GL_TRANSFORM_FEEDBACK_BUFFER, 0, posBuf[0]);
    glBindBufferBase(GL_TRANSFORM_FEEDBACK_BUFFER, 1, velBuf[0]);
    glBindBufferBase(GL_TRANSFORM_FEEDBACK_BUFFER, 2, age[0]);
    glBindTransformFeedback(GL_TRANSFORM_FEEDBACK, feedback[1]);
    glBindBufferBase(GL_TRANSFORM_FEEDBACK_BUFFER, 0, posBuf[1]);
    glBindBufferBase(GL_TRANSFORM_FEEDBACK_BUFFER, 1, velBuf[1]);
    glBindBufferBase(GL_TRANSFORM_FEEDBACK_BUFFER, 2, age[1]);
    glBindTransformFeedback(GL_TRANSFORM_FEEDBACK, 0);

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