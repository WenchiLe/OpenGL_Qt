#include "axbopemglwidget.h"
#include "vertices.h"
#include <random>

const unsigned int timeOutmSec=50;

QVector3D viewInitPos(0.0f,0.0f,5.0f);
float _near=0.1f,_far=100.0f;
QMatrix4x4 model;
QMatrix4x4 view;
QMatrix4x4 projection;
QPoint lastPos;

unsigned int gBuffer;
unsigned int gPosition, gNormal, gAlbedo;
unsigned int ssaoFBO, ssaoBlurFBO;
std::vector<QVector3D> ssaoKernel;
unsigned int noiseTexture;
unsigned int ssaoColorBuffer, ssaoColorBufferBlur;
float lerp(float a, float b, float f)
{
    return a + f * (b - a);
}
QVector3D lightPos = QVector3D(2.0f, 4.0f, -2.0f);
QVector3D lightColor = QVector3D(0.2f, 0.2f, 0.7f);
AXBOpemglWidget::AXBOpemglWidget(QWidget *parent) : QOpenGLWidget(parent)
{
    connect(&m_timer,SIGNAL(timeout()),this,SLOT(on_timeout()));
    m_timer.start(timeOutmSec);
    m_time.start();
    m_camera.Position=viewInitPos;
    setFocusPolicy(Qt::StrongFocus);
    setMouseTracking(true);
}

AXBOpemglWidget::~AXBOpemglWidget()
{
    for(auto iter=m_Models.begin();iter!=m_Models.end();iter++){
        ModelInfo *modelInfo=&iter.value();
        delete modelInfo->model;
    }
}

void AXBOpemglWidget::loadModel(string path)
{
    static int i=0;
    makeCurrent();
    Model * _model=new Model(QOpenGLContext::currentContext()->versionFunctions<QOpenGLFunctions_3_3_Core>()
                             ,path.c_str());
    //m_camera.Position=cameraPosInit(_model->m_maxY,_model->m_minY);
    m_Models["Julian"+QString::number(i++)]=
            ModelInfo{_model,QVector3D(0,0-_model->m_minY,0)
            ,0.0,0.0,0.0,false,QString::fromLocal8Bit("张三")+QString::number(i++)};
    doneCurrent();
}

void AXBOpemglWidget::initializeGL()
{
    initializeOpenGLFunctions();
    glEnable(GL_DEPTH_TEST);
    // configure g-buffer framebuffer
    // ------------------------------
    glGenFramebuffers(1, &gBuffer);
    glBindFramebuffer(GL_FRAMEBUFFER, gBuffer);

    // position color buffer
    glGenTextures(1, &gPosition);
    glBindTexture(GL_TEXTURE_2D, gPosition);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, width(), height(), 0, GL_RGBA, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, gPosition, 0);
    // normal color buffer
    glGenTextures(1, &gNormal);
    glBindTexture(GL_TEXTURE_2D, gNormal);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, width(), height(), 0, GL_RGBA, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, gNormal, 0);
    // color + specular color buffer
    glGenTextures(1, &gAlbedo);
    glBindTexture(GL_TEXTURE_2D, gAlbedo);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width(), height(), 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT2, GL_TEXTURE_2D, gAlbedo, 0);
    // tell OpenGL which color attachments we'll use (of this framebuffer) for rendering
    unsigned int attachments[3] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2 };
    glDrawBuffers(3, attachments);
    // create and attach depth buffer (renderbuffer)
    unsigned int rboDepth;
    glGenRenderbuffers(1, &rboDepth);
    glBindRenderbuffer(GL_RENDERBUFFER, rboDepth);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, width(), height());
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, rboDepth);
    // finally check if framebuffer is complete
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        qDebug() << "Framebuffer not complete!";
    glBindFramebuffer(GL_FRAMEBUFFER, defaultFramebufferObject());

    // also create framebuffer to hold SSAO processing stage
    // -----------------------------------------------------

    glGenFramebuffers(1, &ssaoFBO);  glGenFramebuffers(1, &ssaoBlurFBO);
    glBindFramebuffer(GL_FRAMEBUFFER, ssaoFBO);

    // SSAO color buffer
    glGenTextures(1, &ssaoColorBuffer);
    glBindTexture(GL_TEXTURE_2D, ssaoColorBuffer);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, width(), height(), 0, GL_RED, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, ssaoColorBuffer, 0);
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        qDebug() << "SSAO Framebuffer not complete!";
    // and blur stage
    glBindFramebuffer(GL_FRAMEBUFFER, ssaoBlurFBO);
    glGenTextures(1, &ssaoColorBufferBlur);
    glBindTexture(GL_TEXTURE_2D, ssaoColorBufferBlur);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, width(), height(), 0, GL_RED, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, ssaoColorBufferBlur, 0);
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        qDebug() << "SSAO Blur Framebuffer not complete!";
    glBindFramebuffer(GL_FRAMEBUFFER, defaultFramebufferObject());

    // generate sample kernel
    // ----------------------
    std::uniform_real_distribution<GLfloat> randomFloats(0.0, 1.0); // generates random floats between 0.0 and 1.0
    std::default_random_engine generator;

    for (unsigned int i = 0; i < 64; ++i)
    {
        QVector3D sample(randomFloats(generator) * 2.0 - 1.0, randomFloats(generator) * 2.0 - 1.0, randomFloats(generator));
        sample.normalize();;
        sample *= randomFloats(generator);
        float scale = float(i) / 64.0;

        // scale samples s.t. they're more aligned to center of kernel
        scale = lerp(0.1f, 1.0f, scale * scale);
        sample *= scale;
        ssaoKernel.push_back(sample);
    }

    // generate noise texture
    // ----------------------
    std::vector<QVector3D> ssaoNoise;
    for (unsigned int i = 0; i < 16; i++)
    {
        QVector3D noise(randomFloats(generator) * 2.0 - 1.0, randomFloats(generator) * 2.0 - 1.0, 0.0f); // rotate around z-axis (in tangent space)
        ssaoNoise.push_back(noise);
    }

    glGenTextures(1, &noiseTexture);
    glBindTexture(GL_TEXTURE_2D, noiseTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, 4, 4, 0, GL_RGB, GL_FLOAT, &ssaoNoise[0]);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    // lighting info
    // -------------



    bool success;
    m_GeometryPassShaderProgram.addShaderFromSourceFile(QOpenGLShader::Vertex,":/shaders/shaders/g_buffer.vert");
    m_GeometryPassShaderProgram.addShaderFromSourceFile(QOpenGLShader::Fragment,":/shaders/shaders/g_buffer.frag");
    success=m_GeometryPassShaderProgram.link();
    if(!success) qDebug()<<"ERR:"<<m_GeometryPassShaderProgram.log();

    m_LightingPassShaderProgram.addShaderFromSourceFile(QOpenGLShader::Vertex,":/shaders/shaders/SSAO.vert");
    m_LightingPassShaderProgram.addShaderFromSourceFile(QOpenGLShader::Fragment,":/shaders/shaders/SSAO_lighting.frag");
    success=m_LightingPassShaderProgram.link();
    if(!success) qDebug()<<"ERR:"<<m_LightingPassShaderProgram.log();

    m_SSAOShaderProgram.addShaderFromSourceFile(QOpenGLShader::Vertex,":/shaders/shaders/SSAO.vert");
    m_SSAOShaderProgram.addShaderFromSourceFile(QOpenGLShader::Fragment,":/shaders/shaders/SSAO.frag");
    success=m_SSAOShaderProgram.link();
    if(!success) qDebug()<<"ERR:"<<m_SSAOShaderProgram.log();

    m_BlurrShaderProgram.addShaderFromSourceFile(QOpenGLShader::Vertex,":/shaders/shaders/SSAO.vert");
    m_BlurrShaderProgram.addShaderFromSourceFile(QOpenGLShader::Fragment,":/shaders/shaders/blurr.frag");
    success=m_BlurrShaderProgram.link();
    if(!success) qDebug()<<"ERR:"<<m_BlurrShaderProgram.log();

    // shader configuration
    // --------------------
    m_LightingPassShaderProgram.bind();
    m_LightingPassShaderProgram.setUniformValue("gPosition", 0);
    m_LightingPassShaderProgram.setUniformValue("gNormal", 1);
    m_LightingPassShaderProgram.setUniformValue("gAlbedo", 2);
    m_LightingPassShaderProgram.setUniformValue("ssao", 3);
    m_SSAOShaderProgram.bind();
    m_SSAOShaderProgram.setUniformValue("gPosition", 0);
    m_SSAOShaderProgram.setUniformValue("gNormal", 1);
    m_SSAOShaderProgram.setUniformValue("texNoise", 2);
    m_SSAOShaderProgram.setUniformValue("_width", width());
    m_SSAOShaderProgram.setUniformValue("_height", height());
    m_BlurrShaderProgram.bind();
    m_BlurrShaderProgram.setUniformValue("ssaoInput", 0);

    //load lights
    m_CubeMesh=processMesh(vertices,36,0);
    // load models
    // -----------
    loadModel("D:/Qt/5AdvancedLighting/backpack/backpack.obj");




}

void AXBOpemglWidget::resizeGL(int w, int h)
{
    Q_UNUSED(w);
    Q_UNUSED(h);
}

void AXBOpemglWidget::paintGL()
{
    model.setToIdentity();
    view.setToIdentity();
    projection.setToIdentity();
    // float time=m_time.elapsed()/50.0;
    projection.perspective(m_camera.Zoom,(float)width()/height(),_near,_far);
    view=m_camera.GetViewMatrix();
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // 1. geometry pass: render scene's geometry/color data into gbuffer
    // -----------------------------------------------------------------
    glBindFramebuffer(GL_FRAMEBUFFER, gBuffer);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    m_GeometryPassShaderProgram.bind();
    m_GeometryPassShaderProgram.setUniformValue("projection", projection);
    m_GeometryPassShaderProgram.setUniformValue("view", view);
    // room cube
    model.setToIdentity();
    model.translate(QVector3D(0.0, 7.0f, 0.0f));
    model.scale(7.5f, 7.5f, 7.5f);
    m_GeometryPassShaderProgram.setUniformValue("model", model);
    m_GeometryPassShaderProgram.setUniformValue("invertedNormals", 1); // invert normals as we're inside the cube
    m_CubeMesh->Draw(m_GeometryPassShaderProgram);
    m_GeometryPassShaderProgram.setUniformValue("invertedNormals", 0);
    // backpack model on the floor
    model.setToIdentity();
    model.translate(QVector3D(0.0f, 0.5f, 0.0));
    model.rotate(-90.0f, QVector3D(1.0, 0.0, 0.0));
    model.scale(1.0f);
    m_GeometryPassShaderProgram.setUniformValue("model", model);
    m_Models.first().model->Draw(m_GeometryPassShaderProgram);
    glBindFramebuffer(GL_FRAMEBUFFER, defaultFramebufferObject());


    // 2. generate SSAO texture
    // ------------------------
    glBindFramebuffer(GL_FRAMEBUFFER, ssaoFBO);
    glClear(GL_COLOR_BUFFER_BIT);
    m_SSAOShaderProgram.bind();
    // Send kernel + rotation
    for (unsigned int i = 0; i < 64; ++i){
        std::string str="samples[" + std::to_string(i) + "]";
        m_SSAOShaderProgram.setUniformValue(str.c_str(), ssaoKernel[i]);
    }
    m_SSAOShaderProgram.setUniformValue("projection", projection);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, gPosition);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, gNormal);
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, noiseTexture);
    renderQuad();
    glBindFramebuffer(GL_FRAMEBUFFER, defaultFramebufferObject());


    // 3. blur SSAO texture to remove noise
    // ------------------------------------
    glBindFramebuffer(GL_FRAMEBUFFER, ssaoBlurFBO);
    glClear(GL_COLOR_BUFFER_BIT);
    m_BlurrShaderProgram.bind();
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, ssaoColorBuffer);
    renderQuad();
    glBindFramebuffer(GL_FRAMEBUFFER, defaultFramebufferObject());


    // 4. lighting pass: traditional deferred Blinn-Phong lighting with added screen-space ambient occlusion
    // -----------------------------------------------------------------------------------------------------
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    m_LightingPassShaderProgram.bind();
    // send light relevant uniforms
    QVector3D lightPosView = QVector3D(m_camera.GetViewMatrix() * QVector4D(lightPos, 1.0));
    m_LightingPassShaderProgram.setUniformValue("light.Position", lightPosView);
    m_LightingPassShaderProgram.setUniformValue("light.Color", lightColor);
    // Update attenuation parameters
    const float linear    = 0.09f;
    const float quadratic = 0.032f;
    m_LightingPassShaderProgram.setUniformValue("light.Linear", linear);
    m_LightingPassShaderProgram.setUniformValue("light.Quadratic", quadratic);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, gPosition);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, gNormal);
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, gAlbedo);
    glActiveTexture(GL_TEXTURE3); // add extra SSAO texture to lighting pass
    glBindTexture(GL_TEXTURE_2D, ssaoColorBufferBlur);
    renderQuad();

}

void AXBOpemglWidget::wheelEvent(QWheelEvent *event)
{
    m_camera.ProcessMouseScroll(event->angleDelta().y()/120);
}

void AXBOpemglWidget::keyPressEvent(QKeyEvent *event)
{
    float deltaTime=timeOutmSec/1000.0f;

    switch (event->key()) {
    case Qt::Key_W: m_camera.ProcessKeyboard(FORWARD,deltaTime);break;
    case Qt::Key_S: m_camera.ProcessKeyboard(BACKWARD,deltaTime);break;
    case Qt::Key_D: m_camera.ProcessKeyboard(RIGHT,deltaTime);break;
    case Qt::Key_A: m_camera.ProcessKeyboard(LEFT,deltaTime);break;
    case Qt::Key_Q: m_camera.ProcessKeyboard(DOWN,deltaTime);break;
    case Qt::Key_E: m_camera.ProcessKeyboard(UP,deltaTime);break;
    case Qt::Key_Space: m_camera.Position=viewInitPos;break;

    default:break;
    }
}

void AXBOpemglWidget::mouseMoveEvent(QMouseEvent *event)
{
    makeCurrent();
    if(m_modelMoving){
        for(auto iter=m_Models.begin();iter!=m_Models.end();iter++){
            ModelInfo *modelInfo=&iter.value();
            if(!modelInfo->isSelected) continue;
            modelInfo->worldPos=
                    QVector3D(worldPosFromViewPort(event->pos().x(),event->pos().y()));
        }
    }else
        if(event->buttons() & Qt::RightButton
                || event->buttons() & Qt::LeftButton
                || event->buttons() & Qt::MiddleButton){

            auto currentPos=event->pos();
            QPoint deltaPos=currentPos-lastPos;
            lastPos=currentPos;
            if(event->buttons() & Qt::RightButton)
                m_camera.ProcessMouseMovement(deltaPos.x(),-deltaPos.y());
            else
                for(auto iter=m_Models.begin();iter!=m_Models.end();iter++){
                    ModelInfo *modelInfo=&iter.value();
                    if(!modelInfo->isSelected) continue;
                    if(event->buttons() & Qt::MiddleButton){
                        modelInfo->roll+=deltaPos.x();
                    }
                    else if(event->buttons() & Qt::LeftButton){
                        modelInfo->yaw+=deltaPos.x();
                        modelInfo->pitch+=deltaPos.y();
                    }
                }

        }
    doneCurrent();
}

void AXBOpemglWidget::mousePressEvent(QMouseEvent *event)
{
    bool hasSelected=false;
    makeCurrent();
    lastPos=event->pos();
    if(event->buttons()&Qt::LeftButton){

        QVector4D wolrdPostion=worldPosFromViewPort(event->pos().x(),
                                                    event->pos().y());
        mousePickingPos(QVector3D(wolrdPostion));

        for(QMap<QString, ModelInfo>::iterator iter=m_Models.begin();iter!=m_Models.end();iter++){
            ModelInfo *modelInfo=&iter.value();
            float r=(modelInfo->model->m_maxY-modelInfo->model->m_minY)/2;
            if(modelInfo->worldPos.distanceToPoint(QVector3D(wolrdPostion))<r
                    &&!hasSelected){
                modelInfo->isSelected=true;
                hasSelected=true;
            }
            else
                modelInfo->isSelected=false;
            //            qDebug()<<modelInfo->worldPos.distanceToPoint(QVector3D(wolrdPostion))
            //             <<"<"<<r<<"="<<modelInfo->isSelected;
        }
    }

    doneCurrent();
}

void AXBOpemglWidget::mouseDoubleClickEvent(QMouseEvent *event)
{
    Q_UNUSED(event);
    if(m_modelMoving){
        //再次双击取消移动
        m_modelMoving=false;
    }else
        foreach(auto modelInfo,m_Models){
            //双击启动移动
            if(modelInfo.isSelected==true)
                m_modelMoving=true;
            qDebug()<<modelInfo.name<<modelInfo.isSelected;
        }
}

void AXBOpemglWidget::on_timeout()
{
    update();
}

QVector3D AXBOpemglWidget::cameraPosInit(float maxY, float minY)
{
    QVector3D temp={0,0,0};
    float height=maxY-minY;
    temp.setZ(1.5*height);
    if(minY>=0)
        temp.setY(height/2.0);
    viewInitPos=temp;
    return temp;
}

Mesh* AXBOpemglWidget::processMesh(float *vertices, int size, unsigned int textureId)
{
    vector<Vertex> _vertices;
    vector<unsigned int> _indices;
    vector<Texture> _textures;
    //memcpy(&_vertices[0],vertices,8*size*sizeof(float));
    for(int i=0;i<size;i++){
        Vertex vert;
        vert.Position[0]=vertices[i*8+0];
        vert.Position[1]=vertices[i*8+1];
        vert.Position[2]=vertices[i*8+2];
        vert.Normal[0]=vertices[i*8+3];
        vert.Normal[1]=vertices[i*8+4];
        vert.Normal[2]=vertices[i*8+5];
        vert.TexCoords[0]=vertices[i*8+6];
        vert.TexCoords[1]=vertices[i*8+7];
        _vertices.push_back(vert);
        _indices.push_back(i);
    }
    Texture tex; tex.id=textureId;
    tex.type="texture_diffuse";
    _textures.push_back(tex);
    return new Mesh(
                QOpenGLContext::currentContext()->versionFunctions<QOpenGLFunctions_3_3_Core>()
                ,_vertices,_indices,_textures);
}


QVector4D AXBOpemglWidget::worldPosFromViewPort(int posX, int posY)
{
    float winZ;
    glReadPixels(
                posX,
                this->height()-posY
                ,1,1
                ,GL_DEPTH_COMPONENT,GL_FLOAT
                ,&winZ);
    float x=(2.0f*posX)/this->width()-1.0f;
    float y=1.0f-(2.0f*posY)/this->height();
    float z=winZ*2.0-1.0f;

    float w = (2.0 * _near * _far) / (_far + _near - z * (_far - _near));
    //float w= _near*_far/(_near*winZ-_far*winZ+_far);
    QVector4D wolrdPostion(x,y,z,1);
    wolrdPostion=w*wolrdPostion;
    return view.inverted()*projection.inverted()*wolrdPostion;
}
void AXBOpemglWidget::renderQuad()
{
    // renderQuad() renders a 1x1 XY quad in NDC
    // -----------------------------------------
    unsigned int quadVAO = 0;
    unsigned int quadVBO;
    if (quadVAO == 0)
    {
        float quadVertices[] = {
            // positions        // texture Coords
            -1.0f,  1.0f, 0.0f, 0.0f, 1.0f,
            -1.0f, -1.0f, 0.0f, 0.0f, 0.0f,
            1.0f,  1.0f, 0.0f, 1.0f, 1.0f,
            1.0f, -1.0f, 0.0f, 1.0f, 0.0f,
        };
        // setup plane VAO
        glGenVertexArrays(1, &quadVAO);
        glGenBuffers(1, &quadVBO);
        glBindVertexArray(quadVAO);
        glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), &quadVertices, GL_STATIC_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
    }
    glBindVertexArray(quadVAO);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    glBindVertexArray(0);
}
