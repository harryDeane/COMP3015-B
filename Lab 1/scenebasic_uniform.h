#ifndef SCENEBASIC_UNIFORM_H
#define SCENEBASIC_UNIFORM_H

#include "helper/scene.h"

#include "helper/torus.h"
#include "helper/teapot.h"
#include "helper/plane.h"
#include "helper/cube.h"
#include "helper/objmesh.h"

#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"

#include <glad/glad.h>
#include "helper/glslprogram.h"

class SceneBasic_Uniform : public Scene
{
private:

    Cube cube;
    std::unique_ptr<ObjMesh> spot;
    GLuint fboHandle;

    float tPrev;
    float angle;
    float rotSpeed;

    GLSLProgram prog;
    void setMatrices();
    void compile();

    void setupFBO();
    void renderToTexture();
    void renderScene();

public:
    SceneBasic_Uniform();

    void initScene();
    void update( float t );
    void render();
    void resize(int, int);
};

#endif // SCENEBASIC_UNIFORM_H
