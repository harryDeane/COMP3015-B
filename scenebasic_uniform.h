#ifndef SCENEBASIC_UNIFORM_H
#define SCENEBASIC_UNIFORM_H

#include "helper/scene.h"

#include <glad/glad.h>
#include "helper/glslprogram.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "helper/torus.h"
#include "helper/teapot.h"
#include "helper/plane.h"
#include "helper/objmesh.h"
#include "helper/cube.h"

#include "helper/skybox.h"
#include "KeyboardController.h"

#include "helper/random.h"
#include "helper/grid.h"
#include "helper/particleutils.h"

class SceneBasic_Uniform : public Scene
{
private:
    KeyboardController keyboardController;
    Plane plane;
   //Teapot teapot;
   // Cube cube;
    SkyBox sky;
   std::unique_ptr<ObjMesh> meteor;
   std::unique_ptr<ObjMesh> city;
  
   float rotSpeed;
    float tPrev;
    float angle;
    float time;
    float particleLifeTime;

    GLuint asteroidColorTex;
    GLuint asteroidNormalTex;
    GLuint cityTexture;
    GLuint fireTexture;
   

    float meteorYPosition; // Track meteor's Y position
    float fallSpeed;     // Falling speed
  
    glm::vec3 cameraPos; // Correct declaration of cameraPos
    glm::vec3 cameraUp;
    glm::vec3 cameraFront;

    // New members for multiple falling meteors
    struct FallingMeteor {
        glm::vec3 position;
        glm::vec3 rotationAxis;
        float rotationAngle;
        float fallSpeed;
        bool exploding = false;
        float explosionTime = 0.0f;
        float explosionSpeed = 0.0f;

        // particle system
        glm::vec3 emitterPos;
        glm::vec3 emitterDir;
        GLuint particlesVAO;
        GLuint initVelBuffer;
        GLuint startTimeBuffer;
        int nParticles = 1000;
    };

    std::vector<FallingMeteor> meteors;
    const int NUM_METEORS = 5;          // Number of meterors to spawn
    const float SPAWN_HEIGHT = 40.0f; // Highest spawn point (Y coordinate)
    const float GROUND_LEVEL = -30.0f; // Where they stop falling

    std::vector<FallingMeteor> explodingMeteors;
    float explosionDuration = 2.0f; // How long explosions last

   // GLuint vaoHandle; 
    GLSLProgram  explosionProg, skyProg, modelProg, prog;
    Random rand;
    GLuint initVel, startTime, particles, nParticles;
    Grid grid;
    glm::vec3 emitterPos, emitterDir;

   // glm::mat4 rotationMatrix;

    //Torus torus;
    void setMatrices(GLSLProgram &p);
    void printShaderUniforms(GLSLProgram& p);
    void compile();

    void spawnNewMeteor(); // New function to spawn ogres
    void checkMeteorClick(); // Check for meteor clicks

    void initBuffers();
    float randFloat();
    void renderMeteors();

    void initMeteorParticleSystem(FallingMeteor& meteor);

    // Static callback functions
    static void mouseCallback(GLFWwindow* window, double xpos, double ypos);
    static void mouseButtonCallback(GLFWwindow* window, int button, int action, int mods);

public:
    SceneBasic_Uniform();

    void initScene();
    void update( float t );
    void render();
    
    void resize(int, int);
};

#endif // SCENEBASIC_UNIFORM_H
