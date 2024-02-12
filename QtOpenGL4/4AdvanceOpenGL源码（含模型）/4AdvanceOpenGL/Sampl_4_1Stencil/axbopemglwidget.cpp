#include "axbopemglwidget.h"
#include "vertices.h"
const unsigned int timeOutmSec=50;

QVector3D viewInitPos(0.0f,5.0f,20.0f);
float _near=0.1f,_far=100.0f;
QMatrix4x4 model;
QMatrix4x4 view;
QMatrix4x4 projection;
QPoint lastPos;
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
    //创建VBO和VAO对象，并赋予ID
    bool success;
    m_ShaderProgram.addShaderFromSourceFile(QOpenGLShader::Vertex,":/shaders/shaders/shapes.vert");
    m_ShaderProgram.addShaderFromSourceFile(QOpenGLShader::Fragment,":/shaders/shaders/shapes.frag");
    success=m_ShaderProgram.link();
    if(!success) qDebug()<<"ERR:"<<m_ShaderProgram.log();

    m_SingleColorShaderProgram.addShaderFromSourceFile(QOpenGLShader::Vertex,":/shaders/shaders/shapes.vert");
    m_SingleColorShaderProgram.addShaderFromSourceFile(QOpenGLShader::Fragment,":/shaders/shaders/singleColor.frag");
    success=m_SingleColorShaderProgram.link();
    if(!success) qDebug()<<"ERR:"<<m_SingleColorShaderProgram.log();

    m_BoxDiffuseTex=new
            QOpenGLTexture(QImage(":/images/images/container2.png").mirrored());
    m_PlaneDiffuseTex=new
            QOpenGLTexture(QImage(":/images/images/wall.jpg").mirrored());

    m_PlaneMesh=processMesh(planeVertices,6,m_PlaneDiffuseTex->textureId());
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
    glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
    glEnable(GL_DEPTH_TEST);
glEnable(GL_STENCIL_TEST);
glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);
glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
glStencilMask(0x00);
    m_ShaderProgram.bind();
    m_ShaderProgram.setUniformValue("projection", projection);
    m_ShaderProgram.setUniformValue("view", view);
    //model.rotate(time, 1.0f, 1.0f, 0.5f);

    m_ShaderProgram.setUniformValue("viewPos",m_camera.Position);

    // light properties, note that all light colors are set at full intensity
    m_ShaderProgram.setUniformValue("light.ambient", 0.7f, 0.7f, 0.7f);
    m_ShaderProgram.setUniformValue("light.diffuse", 0.9f, 0.9f, 0.9f);
    m_ShaderProgram.setUniformValue("light.specular", 1.0f, 1.0f, 1.0f);
    // material properties
    m_ShaderProgram.setUniformValue("material.shininess", 32.0f);
    m_ShaderProgram.setUniformValue("light.direction", -0.2f, -1.0f, -0.3f);
    m_ShaderProgram.setUniformValue("model", model);


    m_PlaneMesh->Draw(m_ShaderProgram);

    m_SingleColorShaderProgram.bind();
    m_SingleColorShaderProgram.setUniformValue("projection", projection);
    m_SingleColorShaderProgram.setUniformValue("view", view);
    m_SingleColorShaderProgram.release();
    foreach(auto modelInfo,m_Models){
        model.setToIdentity();
        model.translate(modelInfo.worldPos);

        model.rotate(modelInfo.pitch,QVector3D(1.0,0.0,0.0));
        model.rotate(modelInfo.yaw,QVector3D(0.0,1.0,0.0));
        model.rotate(modelInfo.roll,QVector3D(0.0,0.0,1.0));

glStencilFunc(GL_ALWAYS, 1, 0xFF);
glStencilMask(0xFF);
        m_ShaderProgram.bind();
        m_ShaderProgram.setUniformValue("model", model);
        modelInfo.model->Draw(m_ShaderProgram);
        m_ShaderProgram.release();
        if(modelInfo.isSelected==false) continue;
glStencilFunc(GL_NOTEQUAL, 1, 0xFF);
glStencilMask(0x00);
float height=modelInfo.model->m_maxY-modelInfo.model->m_minY;
float width=modelInfo.model->m_maxX-modelInfo.model->m_minX;
        if(modelInfo.model->m_minY>=0)
            model.translate(0.0f,height/2,0.0f);
        model.scale(1.1f,1.0+0.1*(width/height));
        if(modelInfo.model->m_minY>=0)
            model.translate(0.0f,-height/2,0.0f);
        m_SingleColorShaderProgram.bind();
        m_SingleColorShaderProgram.setUniformValue("model", model);
        modelInfo.model->Draw(m_SingleColorShaderProgram);
        m_SingleColorShaderProgram.release();
glStencilMask(0xFF);
glStencilFunc(GL_ALWAYS, 1, 0xFF);

    }

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
        vert.Position[0]=vertices[i*5+0];
        vert.Position[1]=vertices[i*5+1];
        vert.Position[2]=vertices[i*5+2];
        vert.TexCoords[0]=vertices[i*5+3];
        vert.TexCoords[1]=vertices[i*5+4];
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
