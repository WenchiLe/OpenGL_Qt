#include "axbopemglwidget.h"
#include "vertices.h"
const unsigned int timeOutmSec=50;

QVector3D viewInitPos(0.0f,0.0f,5.0f);
float _near=0.1f,_far=100.0f;
QMatrix4x4 model;
QMatrix4x4 view;
QMatrix4x4 projection;
AXBOpemglWidget::AXBOpemglWidget(QWidget *parent) : QOpenGLWidget(parent)
{
    connect(&m_timer,SIGNAL(timeout()),this,SLOT(on_timeout()));
    m_timer.start(timeOutmSec);
    m_time.start();
    m_camera.Position=viewInitPos;
    setFocusPolicy(Qt::StrongFocus);
    //setMouseTracking(true);
}

AXBOpemglWidget::~AXBOpemglWidget()
{

}

void AXBOpemglWidget::loadModel(string path)
{
    if(m_model !=NULL)
        delete m_model;

    m_model=NULL;
    makeCurrent();
    m_model=new Model(QOpenGLContext::currentContext()->versionFunctions<QOpenGLFunctions_3_3_Core>()
                      ,path.c_str());
    m_camera.Position=cameraPosInit(m_model->m_maxY,m_model->m_minY);
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
    m_BoxDiffuseTex=new
            QOpenGLTexture(QImage(":/images/images/container2.png").mirrored());
    m_PlaneDiffuseTex=new
            QOpenGLTexture(QImage(":/images/images/container2_specular.png").mirrored());

    m_CubeMesh=processMesh(cubeVertices,36,m_BoxDiffuseTex->textureId());
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
    glDepthFunc(GL_LESS);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
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
    m_CubeMesh->Draw(m_ShaderProgram);
    m_PlaneMesh->Draw(m_ShaderProgram);
    if(m_model==NULL) return;
    m_model->Draw(m_ShaderProgram);
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
    if(event->buttons() & Qt::RightButton){
        static QPoint lastPos(width()/2,height()/2);
        auto currentPos=event->pos();
        QPoint deltaPos=currentPos-lastPos;
        lastPos=currentPos;

        m_camera.ProcessMouseMovement(deltaPos.x(),-deltaPos.y());
    }
}

void AXBOpemglWidget::mousePressEvent(QMouseEvent *event)
{
    makeCurrent();
    if(event->buttons()&Qt::LeftButton){

        QVector4D wolrdPostion=worldPosFromViewPort(event->pos().x(),
                                                    event->pos().y());

        mousePickingPos(QVector3D(wolrdPostion));
    }

    doneCurrent();
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
