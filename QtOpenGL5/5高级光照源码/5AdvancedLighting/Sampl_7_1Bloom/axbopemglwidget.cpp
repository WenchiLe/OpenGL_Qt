#include "axbopemglwidget.h"
#include "vertices.h"
const unsigned int timeOutmSec=50;

QVector3D viewInitPos(0.0f,0.0f,5.0f);
float _near=0.1f,_far=100.0f;
QMatrix4x4 model;
QMatrix4x4 view;
QMatrix4x4 projection;
QPoint lastPos;

// lighting info
// -------------
// positions
std::vector<QVector3D> lightPositions;
// colors
std::vector<QVector3D> lightColors;

unsigned int hdrFBO;
unsigned int colorBuffers[2];

unsigned int pingpongFBO[2];
unsigned int pingpongColorbuffers[2];
unsigned int rboDepth;
AXBOpemglWidget::AXBOpemglWidget(QWidget *parent) : QOpenGLWidget(parent)
{
    connect(&m_timer,SIGNAL(timeout()),this,SLOT(on_timeout()));
    m_timer.start(timeOutmSec);
    m_time.start();
    m_camera.Position=viewInitPos;
    setFocusPolicy(Qt::StrongFocus);
    setMouseTracking(true);

    lightPositions.push_back(QVector3D( 0.0f, 0.5f,  1.5f)); // back light
    lightPositions.push_back(QVector3D(-4.0f, 0.5f, -3.0f));
    lightPositions.push_back(QVector3D( 3.0f, 0.5f,  1.0f));
    lightPositions.push_back(QVector3D( -.8f,  2.4f, -1.0f));

    lightColors.push_back(QVector3D(5.0f,   5.0f,  5.0f));
    lightColors.push_back(QVector3D(10.0f,  0.0f,  0.0f));
    lightColors.push_back(QVector3D(0.0f,   0.0f,  15.0f));
    lightColors.push_back(QVector3D(0.0f,   5.0f,  0.0f));
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
    int SCR_WIDTH=width();
    int SCR_HEIGHT=height();
    initializeOpenGLFunctions();

    // configure (floating point) framebuffers
    // ---------------------------------------
    glGenFramebuffers(1, &hdrFBO);
    glBindFramebuffer(GL_FRAMEBUFFER, hdrFBO);
    // create 2 floating point color buffers (1 for normal rendering, other for brightness threshold values)
    glGenTextures(2, colorBuffers);
    for (unsigned int i = 0; i < 2; i++)
    {
        glBindTexture(GL_TEXTURE_2D, colorBuffers[i]);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, SCR_WIDTH, SCR_HEIGHT, 0, GL_RGBA, GL_FLOAT, NULL);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);  // we clamp to the edge as the blur filter would otherwise sample repeated texture values!
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        // attach texture to framebuffer
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + i, GL_TEXTURE_2D, colorBuffers[i], 0);
    }
    // create and attach depth buffer (renderbuffer)

    glGenRenderbuffers(1, &rboDepth);
    glBindRenderbuffer(GL_RENDERBUFFER, rboDepth);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, SCR_WIDTH, SCR_HEIGHT);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, rboDepth);
    // tell OpenGL which color attachments we'll use (of this framebuffer) for rendering
    unsigned int attachments[2] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1 };
    glDrawBuffers(2, attachments);
    // finally check if framebuffer is complete
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        qDebug()<< "Framebuffer not complete!" ;
    glBindFramebuffer(GL_FRAMEBUFFER, defaultFramebufferObject());


    // ping-pong-framebuffer for blurring
    glGenFramebuffers(2, pingpongFBO);
    glGenTextures(2, pingpongColorbuffers);
    for (unsigned int i = 0; i < 2; i++)
    {
        glBindFramebuffer(GL_FRAMEBUFFER, pingpongFBO[i]);
        glBindTexture(GL_TEXTURE_2D, pingpongColorbuffers[i]);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, SCR_WIDTH, SCR_HEIGHT, 0, GL_RGBA, GL_FLOAT, NULL);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE); // we clamp to the edge as the blur filter would otherwise sample repeated texture values!
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, pingpongColorbuffers[i], 0);
        // also check if framebuffers are complete (no need for depth buffer)
        if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
            qDebug()<< "Framebuffer not complete!";
    }

    bool success;
    m_LightingProgram.addShaderFromSourceFile(QOpenGLShader::Vertex,":/shaders/shaders/lighting.vert");
    m_LightingProgram.addShaderFromSourceFile(QOpenGLShader::Fragment,":/shaders/shaders/lighting.frag");
    success=m_LightingProgram.link();
    if(!success) qDebug()<<"ERR:"<<m_LightingProgram.log();

    m_LightProgram.addShaderFromSourceFile(QOpenGLShader::Vertex,":/shaders/shaders/light.vert");
    m_LightProgram.addShaderFromSourceFile(QOpenGLShader::Fragment,":/shaders/shaders/light.frag");
    success=m_LightProgram.link();
    if(!success) qDebug()<<"ERR:"<<m_LightProgram.log();

    m_BlurProgram.addShaderFromSourceFile(QOpenGLShader::Vertex,":/shaders/shaders/blur.vert");
    m_BlurProgram.addShaderFromSourceFile(QOpenGLShader::Fragment,":/shaders/shaders/blur.frag");
    success=m_BlurProgram.link();
    if(!success) qDebug()<<"ERR:"<<m_BlurProgram.log();


    m_ShaderProgram.addShaderFromSourceFile(QOpenGLShader::Vertex,":/shaders/shaders/shapes.vert");
    m_ShaderProgram.addShaderFromSourceFile(QOpenGLShader::Fragment,":/shaders/shaders/shapes.frag");
    success=m_ShaderProgram.link();
    if(!success) qDebug()<<"ERR:"<<m_ShaderProgram.log();

    m_LightingProgram.bind();
    for (unsigned int i = 0; i < lightPositions.size(); i++){
        std::string str="lights[" + std::to_string(i) + "].Position";
        m_LightingProgram.setUniformValue(str.c_str(), lightPositions[i]);
        str="lights[" + std::to_string(i) + "].Color";
        m_LightingProgram.setUniformValue(str.c_str(), lightColors[i]);
    }
    m_LightingProgram.release();

    // m_WallTex=new QOpenGLTexture(QImage(":/images/images/wall.jpg").mirrored());
    QImage wall = QImage(":/images/images/container2.png").convertToFormat(QImage::Format_RGB888);
    m_WallTex=new QOpenGLTexture(QOpenGLTexture::Target2D);
    glBindTexture(GL_TEXTURE_2D, m_WallTex->textureId());
    glTexImage2D(GL_TEXTURE_2D, 0, GL_SRGB, wall.width(), wall.height(),0, GL_RGB , GL_UNSIGNED_BYTE, wall.bits());
    glGenerateMipmap(GL_TEXTURE_2D);
    m_CubeMesh=processMesh(vertices,36,m_WallTex->textureId());
}

void AXBOpemglWidget::resizeGL(int w, int h)
{
    glViewport(0,0,w,h);
    for (unsigned int i = 0; i < 2; i++)
    {
        glBindTexture(GL_TEXTURE_2D, colorBuffers[i]);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, w, h, 0, GL_RGBA, GL_FLOAT, NULL);
    }

    for (unsigned int i = 0; i < 2; i++)
    {
        glBindTexture(GL_TEXTURE_2D, pingpongColorbuffers[i]);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, w, h, 0, GL_RGBA, GL_FLOAT, NULL);
    }

    glBindRenderbuffer(GL_RENDERBUFFER, rboDepth);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, w, h);
}

void AXBOpemglWidget::paintGL()
{

    model.setToIdentity();
    view.setToIdentity();
    projection.setToIdentity();
// 1. render scene into floating point framebuffer
// -----------------------------------------------
    glBindFramebuffer(GL_FRAMEBUFFER, hdrFBO);
    projection.perspective(m_camera.Zoom,(float)width()/height(),_near,_far);
    view=m_camera.GetViewMatrix();
    glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
    glEnable(GL_DEPTH_TEST);
    // glDepthFunc(GL_LESS);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    m_LightingProgram.bind();
    m_LightingProgram.setUniformValue("projection", projection);
    m_LightingProgram.setUniformValue("view", view);
    m_LightingProgram.setUniformValue("viewPos",m_camera.Position);


    model.setToIdentity();
    model.translate(QVector3D(0.0f, -1.0f, 0.0));
    model.scale(12.5f, 0.5f, 12.5f);
    m_LightingProgram.setUniformValue("model", model);
    m_CubeMesh->Draw(m_LightingProgram);
    // then create multiple cubes as the scenery
    model.setToIdentity();
    model.translate(QVector3D(0.0f, 1.5f, 0.0));
    model.scale(0.5f);
    m_LightingProgram.setUniformValue("model", model);
    m_CubeMesh->Draw(m_LightingProgram);

    model.setToIdentity();
    model.translate(QVector3D(2.0f, 0.0f, 1.0));
    model.scale(0.5f);
    m_LightingProgram.setUniformValue("model", model);
    m_CubeMesh->Draw(m_LightingProgram);

    model.setToIdentity();
    model.translate(QVector3D(-1.0f, -1.0f, 2.0));
    model.rotate(60.0f, QVector3D(1.0, 0.0, 1.0));
    m_LightingProgram.setUniformValue("model", model);
    m_CubeMesh->Draw(m_LightingProgram);

    model.setToIdentity();
    model.translate(QVector3D(0.0f, 2.7f, 4.0));
    model.rotate(23.0f,QVector3D(1.0, 0.0, 1.0).normalized());
    model.scale(1.25);
    m_LightingProgram.setUniformValue("model", model);
    m_CubeMesh->Draw(m_LightingProgram);

    model.setToIdentity();
    model.translate(QVector3D(-2.0f, 1.0f, -3.0));
    model.rotate(124.0f, QVector3D(1.0, 0.0, 1.0).normalized());
    m_LightingProgram.setUniformValue("model", model);
    m_CubeMesh->Draw(m_LightingProgram);

    model.setToIdentity();
    model.translate(QVector3D(-3.0f, 0.0f, 0.0));
    model.scale(0.5f);
    m_LightingProgram.setUniformValue("model", model);
    m_CubeMesh->Draw(m_LightingProgram);
    m_LightingProgram.release();

    //渲染灯源
    m_LightProgram.bind();
    m_LightProgram.setUniformValue("projection", projection);
    m_LightProgram.setUniformValue("view", view);
    for (unsigned int i = 0; i < lightPositions.size(); i++)
    {
        model.setToIdentity();
        model.translate(QVector3D(lightPositions[i]));
        model.scale(0.25f);
        m_LightProgram.setUniformValue("model", model);
        m_LightProgram.setUniformValue("lightColor", lightColors[i]);
        m_CubeMesh->Draw(m_LightProgram);
    }
    m_LightProgram.release();
    glBindFramebuffer(GL_FRAMEBUFFER, defaultFramebufferObject());

// 2. blur bright fragments with two-pass Gaussian Blur
// --------------------------------------------------
    bool horizontal = true, first_iteration = true;
    unsigned int amount = 10;
    m_BlurProgram.bind();
    for (unsigned int i = 0; i < amount; i++)
    {
        glBindFramebuffer(GL_FRAMEBUFFER, pingpongFBO[horizontal]);
        m_BlurProgram.setUniformValue("horizontal", horizontal);
        glBindTexture(GL_TEXTURE_2D, first_iteration ? colorBuffers[1] : pingpongColorbuffers[!horizontal]);  // bind texture of other framebuffer (or scene if first iteration)
        renderQuad();
        horizontal = !horizontal;
        if (first_iteration)
            first_iteration = false;
    }
    glBindFramebuffer(GL_FRAMEBUFFER, defaultFramebufferObject());
    m_BlurProgram.release();
// 3. now render floating point color buffer to 2D quad and tonemap HDR colors to default framebuffer's (clamped) color range
// --------------------------------------------------------------------------------------------------------------------------
    m_ShaderProgram.bind();
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, colorBuffers[0]);
    m_ShaderProgram.setUniformValue("bloomBlur",1);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, pingpongColorbuffers[!horizontal]);
    m_ShaderProgram.setUniformValue("bloom", m_Bloom);
    m_ShaderProgram.setUniformValue("exposure", m_exposure);
    renderQuad();
    m_ShaderProgram.release();

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

void AXBOpemglWidget::setHDR(bool checked)
{
    m_Bloom=checked;
}

void AXBOpemglWidget::setExposure(double exposure)
{
    m_exposure=exposure;
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

