#include "axbopemglwidget.h"
#include "vertices.h"
const unsigned int timeOutmSec=50;

QVector3D viewInitPos(0.0f,0.0f,0.0f);
float _near=0.1f,_far=100.0f;
QMatrix4x4 model;
QMatrix4x4 view;
QMatrix4x4 projection;
QPoint lastPos;

const unsigned int SHADOW_WIDTH = 10240, SHADOW_HEIGHT = 10240;
unsigned int depthMapFBO;
QVector3D lightPos(0.0f, 0.0f, 0.0f);
unsigned int depthCubemap;
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

}

void AXBOpemglWidget::initializeGL()
{
    initializeOpenGLFunctions();
    //创建VBO和VAO对象，并赋予ID
    bool success;
    m_ShaderProgram.addShaderFromSourceFile(QOpenGLShader::Vertex,":/shaders/shaders/shapes.vert");
    m_ShaderProgram.addShaderFromSourceFile(QOpenGLShader::Fragment,":/shaders/shaders/shapes.frag");
    success=m_ShaderProgram.link();
    if(!success) qDebug()<<"ERR:"<<m_ShaderProgram.log();

    m_DepthMapShaderProgram.addShaderFromSourceFile(QOpenGLShader::Vertex,":/shaders/shaders/depthMap.vert");
    m_DepthMapShaderProgram.addShaderFromSourceFile(QOpenGLShader::Geometry,":/shaders/shaders/depthMap.geom");
    m_DepthMapShaderProgram.addShaderFromSourceFile(QOpenGLShader::Fragment,":/shaders/shaders/depthMap.frag");
    success=m_DepthMapShaderProgram.link();
    if(!success) qDebug()<<"ERR:"<<m_DepthMapShaderProgram.log();

    m_DiffuseTex=new QOpenGLTexture(QImage(":/images/images/wall.jpg").mirrored());

    // configure depth map FBO // -----------------------
    glGenFramebuffers(1, &depthMapFBO);
    // create depth cubemap texture

    glGenTextures(1, &depthCubemap);
    glBindTexture(GL_TEXTURE_CUBE_MAP, depthCubemap);
    for (unsigned int i = 0; i < 6; ++i)
        glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_DEPTH_COMPONENT, SHADOW_WIDTH, SHADOW_HEIGHT, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);

    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    // glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    // attach depth texture as FBO's depth buffer
    glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO);
    glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, depthCubemap, 0);
    glDrawBuffer(GL_NONE);
    glReadBuffer(GL_NONE);
    glBindFramebuffer(GL_FRAMEBUFFER, defaultFramebufferObject());

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        qDebug() << "ERROR::FRAMEBUFFER:: Framebuffer is not complete!" << endl;
    glBindFramebuffer(GL_FRAMEBUFFER, defaultFramebufferObject() );

    m_CubeMesh=processMesh(vertices,36,m_DiffuseTex->textureId());
    glEnable(GL_CULL_FACE);
}

void AXBOpemglWidget::resizeGL(int w, int h)
{
    Q_UNUSED(w);
    Q_UNUSED(h);
}

void AXBOpemglWidget::paintGL()
{
    // move light position over time
   // lightPos.setZ(sin(m_time.elapsed()/500 ) * 3.0);
    model.setToIdentity();
    view.setToIdentity();
    projection.setToIdentity();

    glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // 0. create depth cubemap transformation matrices
    // -----------------------------------------------
    float near_plane = 1.0f;
    float far_plane  = 25.0f;
    QMatrix4x4 shadowProj;
    QMatrix4x4 shadowView;
    shadowProj.perspective(90.0f, (float)SHADOW_WIDTH / (float)SHADOW_HEIGHT, near_plane, far_plane);
    std::vector<QMatrix4x4> shadowTransforms;
    shadowView.lookAt(lightPos, lightPos + QVector3D( 1.0, 0.0, 0.0), QVector3D(0.0,-1.0, 0.0));
    shadowTransforms.push_back(shadowProj * shadowView);    shadowView.setToIdentity();
    shadowView.lookAt(lightPos, lightPos + QVector3D(-1.0, 0.0, 0.0), QVector3D(0.0,-1.0, 0.0));
    shadowTransforms.push_back(shadowProj * shadowView);    shadowView.setToIdentity();
    shadowView.lookAt(lightPos, lightPos + QVector3D( 0.0, 1.0, 0.0), QVector3D(0.0, 0.0, 1.0));
    shadowTransforms.push_back(shadowProj * shadowView);    shadowView.setToIdentity();
    shadowView.lookAt(lightPos, lightPos + QVector3D( 0.0,-1.0, 0.0), QVector3D(0.0, 0.0,-1.0));
    shadowTransforms.push_back(shadowProj * shadowView);    shadowView.setToIdentity();
    shadowView.lookAt(lightPos, lightPos + QVector3D( 0.0, 0.0, 1.0), QVector3D(0.0,-1.0, 0.0));
    shadowTransforms.push_back(shadowProj * shadowView);    shadowView.setToIdentity();
    shadowView.lookAt(lightPos, lightPos + QVector3D( 0.0, 0.0,-1.0), QVector3D(0.0,-1.0, 0.0));
    shadowTransforms.push_back(shadowProj * shadowView);

    // 1. render scene to depth cubemap
    // --------------------------------
    glViewport(0, 0, SHADOW_WIDTH, SHADOW_HEIGHT);
    glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO);
    glClear(GL_DEPTH_BUFFER_BIT);
    m_DepthMapShaderProgram.bind();
    glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO);
    glViewport(0, 0, SHADOW_WIDTH, SHADOW_HEIGHT);
    glClear(GL_DEPTH_BUFFER_BIT);
    for (unsigned int i = 0; i < 6; ++i){
        std::string str="shadowMatrices[" + std::to_string(i) + "]";
        m_DepthMapShaderProgram.setUniformValue(str.c_str(), shadowTransforms[i]);
    }
    m_DepthMapShaderProgram.setUniformValue("far_plane", far_plane);
    m_DepthMapShaderProgram.setUniformValue("lightPos", lightPos);
    renderScene(&m_DepthMapShaderProgram);
    glBindFramebuffer(GL_FRAMEBUFFER, defaultFramebufferObject());
    m_DepthMapShaderProgram.release();



    m_ShaderProgram.bind();
    projection.perspective(m_camera.Zoom,(float)width()/(float)height(),_near,_far);
    view=m_camera.GetViewMatrix();
    glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glViewport(0, 0, width(), height());
    m_ShaderProgram.setUniformValue("far_plane", far_plane);
    m_ShaderProgram.setUniformValue("lightPos", lightPos);
    //  m_ShaderProgram.setUniformValue("model", model);
    m_ShaderProgram.setUniformValue("shadows", true);
    m_ShaderProgram.setUniformValue("projection", projection);
    m_ShaderProgram.setUniformValue("view", view);
    m_ShaderProgram.setUniformValue("viewPos",m_camera.Position);
    m_ShaderProgram.setUniformValue("depthMap",1);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_CUBE_MAP, depthCubemap);
    renderScene(&m_ShaderProgram);
    m_ShaderProgram.release();
}
void AXBOpemglWidget::renderScene(QOpenGLShaderProgram* shader)
{
    model.setToIdentity();

    // room cube
    model.scale(5.0f);
    shader->setUniformValue("model", model);
    glDisable(GL_CULL_FACE); // note that we disable culling here since we render 'inside' the cube instead of the usual 'outside' which throws off the normal culling methods.
    shader->setUniformValue("reverse_normals", 1); // A small little hack to invert normals when drawing cube from the inside so lighting still works.
    m_CubeMesh->Draw(*shader);
    shader->setUniformValue("reverse_normals", 0); // and of course disable it
    glEnable(GL_CULL_FACE);

    // 三个箱子
    model.setToIdentity();
    model.translate(QVector3D(4.0f, -3.5f, 0.0));
    model.scale(0.5);
    shader->setUniformValue("model", model);
    m_CubeMesh->Draw(*shader);

    model.setToIdentity();
    model.translate(QVector3D(2.0f, 3.0f, 1.0));
    model.scale(0.75);
    shader->setUniformValue("model", model);
    m_CubeMesh->Draw(*shader);

    model.setToIdentity();
    model.translate(QVector3D(-3.0f, -1.0f, 0.0));
    model.scale(0.5);
    shader->setUniformValue("model", model);
    m_CubeMesh->Draw(*shader);

    model.setToIdentity();
    model.translate(QVector3D(-1.5f, 1.0f, 1.5));
    model.scale(0.5);
    //model.rotate(60,QVector3D(1.0, 0.0, 1.0));
    shader->setUniformValue("model", model);
    m_CubeMesh->Draw(*shader);

    model.setToIdentity();
    model.translate(QVector3D(-1.5f, 2.0f, -3.0));
    model.scale(0.75);
    model.rotate(60,QVector3D(1.0, 0.0, 1.0));
    shader->setUniformValue("model", model);
    m_CubeMesh->Draw(*shader);
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
