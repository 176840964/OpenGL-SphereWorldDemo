//
//  main.cpp
//  视觉demo
//

#include "GLTools.h"
#include "GLShaderManager.h"
#include "GLFrustum.h"
#include "GLBatch.h"
#include "GLMatrixStack.h"
#include "GLGeometryTransform.h"
#include "StopWatch.h"

#include <math.h>
#include <stdio.h>

#ifdef __APPLE__
#include <glut/glut.h>
#else
#define FREEGLUT_STATIC
#include <GL/glut.h>
#endif

GLShaderManager shaderManager;
GLMatrixStack modelViewMatrix;
GLMatrixStack projectionMatrix;
GLFrustum viewFrustum;
GLGeometryTransform transformPipeline;

GLTriangleBatch torusBatch;
GLTriangleBatch sphereBatch;
GLBatch floorBatch;

GLFrame cameraFrame;
GLFrame objectFrame;

#define NUM_SPHERES 50
GLFrame spheres[NUM_SPHERES];

GLuint uiTextures[3];

/**
 加载TGA纹理
 szFileName：TGA文件名
 minFilter：缩小过滤器
 magFileter：放大过滤器
 wrapMode：环绕过滤器
 */
bool LoadTGATexture(const char *szFileName, GLenum minFilter, GLenum magFileter, GLenum wrapMode){
    //定义局部变量
    GLbyte *pBits;
    int nWidth, nHeight, nComponents;
    GLenum eFormat;
    
    //加载TGA文件，获取TGA文件的宽、高、组件、格式，并返回一个纹理图像数据的指针
    pBits = gltReadTGABits(szFileName, &nWidth, &nHeight, &nComponents, &eFormat);
    if (pBits == NULL) {
        return false;
    }
    
    //设置纹理的S、T的环绕方式
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, wrapMode);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, wrapMode);
    
    //设置纹理放大、缩小的过滤方式
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, minFilter);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, magFileter);
    
    /**
     载入纹理：
     参数1：纹理2D模式
     参数2：mip贴图层次，通常设置为0
     参数3：纹理单元存储成分，使用压缩RGB格式
     参数4、5：纹理宽、高
     参数6：纹理深度
     参数7：像素格式
     参数8：像素数据的数据类型（GL_UNSIGNED_BYTE，每个颜色分量都是一个8位无符号整数）
     参数9：指向纹理图像数据的指针
     */
    glTexImage2D(GL_TEXTURE_2D, 0, GL_COMPRESSED_RGB, nWidth, nHeight, 0, eFormat, GL_UNSIGNED_BYTE, pBits);
    free(pBits);
    
    /*
     只有minFilter等于这四种模式的时候，纹理生成所有mip层。
     因为GL_LINEAR和GL_NEAREST模式下只有基层纹理会加载，所以生成所有mip层也没用
     */
    if (minFilter == GL_LINEAR_MIPMAP_LINEAR ||
        minFilter == GL_LINEAR_MIPMAP_NEAREST ||
        minFilter == GL_NEAREST_MIPMAP_LINEAR ||
        minFilter == GL_NEAREST_MIPMAP_NEAREST) {
        glGenerateMipmap(GL_TEXTURE_2D);
    }
    
    return true;
}

void SetupRC() {
    glClearColor(0.0f,0.0f,0.0f,1.0f);
    shaderManager.InitializeStockShaders();
    
//    objectFrame.MoveForward(5);
    
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    
    //大球
    gltMakeSphere(torusBatch, 0.4, 40, 80);
    
    //小球
    gltMakeSphere(sphereBatch, 0.1f, 26, 13);
    
    //地板
    GLfloat texSize = 10;
    floorBatch.Begin(GL_TRIANGLE_FAN, 4, 1);
    floorBatch.MultiTexCoord2f(0, 0, 0);
    floorBatch.Vertex3f(-20, -0.41, 20);
    
    floorBatch.MultiTexCoord2f(0, texSize, 0);
    floorBatch.Vertex3f(20, -0.41, 20);
    
    floorBatch.MultiTexCoord2f(0, texSize, texSize);
    floorBatch.Vertex3f(20, -0.41, -20);
    
    floorBatch.MultiTexCoord2f(0, 0, texSize);
    floorBatch.Vertex3f(-20, -0.41, -20);
    floorBatch.End();
    
    //随机小球位置 NUM_SPHERES个
    for (int i = 0; i < NUM_SPHERES; i++) {
        GLfloat x = (GLfloat)((rand() % 400) - 200) * 0.1;
        GLfloat z = (GLfloat)((rand() % 400) - 200) * 0.1;
        
        spheres[i].SetOrigin(x, 0.0f, z);
    }
    
    //申请3个纹理对象
    glGenTextures(3, uiTextures);
    
    //设置纹理状态模式：2D
    glBindTexture(GL_TEXTURE_2D, uiTextures[0]);
    //调用自定义方法
    LoadTGATexture("Marble.tga", GL_LINEAR_MIPMAP_LINEAR, GL_LINEAR, GL_REPEAT);
    
    glBindTexture(GL_TEXTURE_2D, uiTextures[1]);
    LoadTGATexture("Marslike.tga", GL_LINEAR_MIPMAP_LINEAR, GL_LINEAR, GL_CLAMP_TO_EDGE);
    
    glBindTexture(GL_TEXTURE_2D, uiTextures[2]);
    LoadTGATexture("MoonLike.tga", GL_LINEAR_MIPMAP_LINEAR, GL_LINEAR, GL_CLAMP_TO_EDGE);
    
}

void drawSomething(GLfloat yRot) {
    static GLfloat vWhite[] = {1, 1, 1, 1};
    static GLfloat vLightPos[] = {0, 3, 0, 1};
    
    glBindTexture(GL_TEXTURE_2D, uiTextures[2]);
    for (int i = 0; i < NUM_SPHERES; i++) {
        modelViewMatrix.PushMatrix();
        modelViewMatrix.MultMatrix(spheres[i]);
        shaderManager.UseStockShader(GLT_SHADER_TEXTURE_POINT_LIGHT_DIFF, modelViewMatrix.GetMatrix(), transformPipeline.GetProjectionMatrix(), vLightPos, vWhite, 0);
        sphereBatch.Draw();
        modelViewMatrix.PopMatrix();
    }
    
    modelViewMatrix.Translate(0, 0.2, -2.5);
    modelViewMatrix.PushMatrix();
    modelViewMatrix.Rotate(yRot, 0, 1, 0);
    glBindTexture(GL_TEXTURE_2D, uiTextures[1]);
    shaderManager.UseStockShader(GLT_SHADER_TEXTURE_POINT_LIGHT_DIFF, modelViewMatrix.GetMatrix(), transformPipeline.GetProjectionMatrix(), vLightPos, vWhite, 0);
    torusBatch.Draw();
    modelViewMatrix.PopMatrix();
    
    modelViewMatrix.PushMatrix();
    modelViewMatrix.Rotate(yRot * -2, 0, 1, 0);
    modelViewMatrix.Translate(0.8, 0, 0);
    glBindTexture(GL_TEXTURE_2D, uiTextures[2]);
    shaderManager.UseStockShader(GLT_SHADER_TEXTURE_POINT_LIGHT_DIFF, modelViewMatrix.GetMatrix(), transformPipeline.GetProjectionMatrix(), vLightPos, vWhite, 0);
    sphereBatch.Draw();
    modelViewMatrix.PopMatrix();
}

//开始渲染
void RenderScene(void) {
    static GLfloat vFloorColor[] = {1.0, 1.0, 0.0, 0.75};
    
    static CStopWatch rotTimer;
    float yRot = rotTimer.GetElapsedSeconds() * 60;
    
    glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
    
    modelViewMatrix.PushMatrix();
    
    M3DMatrix44f mCamera;
    cameraFrame.GetCameraMatrix(mCamera);
    modelViewMatrix.MultMatrix(mCamera);
    
    modelViewMatrix.PushMatrix(mCamera);
    //倒影
    modelViewMatrix.Scale(1, -1, 1);
    modelViewMatrix.Translate(0, 0.8, 0);
    glFrontFace(GL_CW);
    drawSomething(yRot);
    glFrontFace(GL_CCW);
    modelViewMatrix.PopMatrix();
    
    //地面-与倒影进行了混合
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glBindTexture(GL_TEXTURE_2D, uiTextures[0]);
    shaderManager.UseStockShader(GLT_SHADER_TEXTURE_MODULATE, transformPipeline.GetModelViewProjectionMatrix(), vFloorColor, 0);
    floorBatch.Draw();
    glDisable(GL_BLEND);
    
    //地面上面的部分
    drawSomething(yRot);
    
    modelViewMatrix.PopMatrix();
    
    //将在后台缓冲区进行渲染，然后在结束时交换到前台
    glutSwapBuffers();
    //9触发重新绘制
    glutPostRedisplay();
}

//窗口大小改变时接受新的宽度和高度，其中0，0代表窗口中视图的左下角坐标，w，h代表像素
void ChangeSize(int width, int height)
{
    glViewport(0, 0, width, height);
    
    viewFrustum.SetPerspective(35.0f, float(width)/float(height), 1.0f, 100.0f);
    
    projectionMatrix.LoadMatrix(viewFrustum.GetProjectionMatrix());
    
    transformPipeline.SetMatrixStacks(modelViewMatrix, projectionMatrix);
}

void SpecialKeys(int key, int x, int y) {
    float liner = 0.1f;
    float angular = float(m3dDegToRad(5));
    
    if (key == GLUT_KEY_UP) {
        cameraFrame.MoveForward(liner);
//        objectFrame.MoveForward(liner);
    }
    
    if (key == GLUT_KEY_DOWN) {
        cameraFrame.MoveForward(-liner);
//        objectFrame.MoveForward(-liner);
    }
    
    if (key == GLUT_KEY_LEFT) {
        cameraFrame.RotateWorld(angular, 0, 1, 0);
//        objectFrame.RotateWorld(angular, 0, 1, 0);
    }
    
    if (key == GLUT_KEY_RIGHT) {
        cameraFrame.RotateWorld(-angular, 0, 1, 0);
//        objectFrame.RotateWorld(-angular, 0, 1, 0);
    }
}

void ShutdownRC(void)
{
    glDeleteTextures(3, uiTextures);
}

int main(int argc,char* argv[]) {

    //设置当前工作目录，针对MAC OS X
    gltSetWorkingDirectory(argv[0]);
    
    //初始化GLUT库
    glutInit(&argc, argv);
    /*初始化双缓冲窗口，其中标志GLUT_DOUBLE、GLUT_RGBA、GLUT_DEPTH、GLUT_STENCIL分别指
     双缓冲窗口、RGBA颜色模式、深度测试、模板缓冲区
     */
    glutInitDisplayMode(GLUT_DOUBLE|GLUT_RGBA|GLUT_DEPTH);
    
    //GLUT窗口大小
    glutInitWindowSize(800,600);
    glutCreateWindow("OpenGL SphereWorld");
    
    //注册回调函数
    glutReshapeFunc(ChangeSize);
    glutDisplayFunc(RenderScene);
    glutSpecialFunc(SpecialKeys);

    //驱动程序的初始化中没有出现任何问题
    GLenum err = glewInit();
    if(GLEW_OK != err) {
        fprintf(stderr,"glew error:%s\n",glewGetErrorString(err));
        return 1;
    }

    //调用SetupRC
    SetupRC();
    glutMainLoop();
    ShutdownRC();
    return 0;
}
