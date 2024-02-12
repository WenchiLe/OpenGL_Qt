#include "axbopemglwidget.h"
#include "vertices.h"
const unsigned int timeOutmSec=50;

QVector3D viewInitPos(0.0f,5.0f,20.0f);
float _near=0.1f,_far=100.0f;
QMatrix4x4 model;
QMatrix4x4 view;
QMatrix4x4 projection;
QPoint lastPos;

unsigned int amount = 10000;
QMatrix4x4 *modelMatrices;
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

    m_RockShaderProgram.addShaderFromSourceFile(QOpenGLShader::Vertex,":/shaders/shaders/rock.vert");
    m_RockShaderProgram.addShaderFromSourceFile(QOpenGLShader::Fragment,":/shaders/shaders/rock.frag");
    success=m_RockShaderProgram.link();
    if(!success) qDebug()<<"ERR:"<<m_RockShaderProgram.log();

    m_BoxDiffuseTex=new
            QOpenGLTexture(QImage(":/images/images/container2.png").mirrored());
    m_PlaneDiffuseTex=new
            QOpenGLTexture(QImage(":/images/images/wall.jpg").mirrored());


    modelMatrices = new QMatrix4x4[amount];
    QVector4D * qvector4d=new QVector4D[4*amount];
    qsrand(QTime(0,0,0).secsTo(QTime::currentTime()));
    float radius = 50.0;
    float offset = 7.5f;
    for(unsigned int i = 0; i < amount; i++) {
        QMatrix4x4 model;
        // 1. translation: displace along circle with 'radius' in range [-offset, offset]
        float angle = (float)i / (float)amount * 360.0f;
        float displacement = (qrand() % (int)(2 * offset * 100)) / 100.0f - offset;
        float x = sin(angle) * radius + displacement;
        displacement = (qrand() % (int)(2 * offset * 100)) / 100.0f - offset;
        float y = displacement * 0.4f; // keep height of field smaller compared to width of x and z
        displacement = (qrand() % (int)(2 * offset * 100)) / 100.0f - offset;
        float z = cos(angle) * radius + displacement;
        model .translate(QVector3D(x, y, z));

        // 2. scale: scale between 0.05 and 0.25f
        float scale = (qrand() % 20) / 100.0f + 0.05;
        model.scale(scale);

        // 3. rotation: add random rotation around a (semi)randomly picked rotation axis vector
        float rotAngle = (qrand() % 360);
        model.rotate(rotAngle, QVector3D(0.4f, 0.6f, 0.8f));

        // 4. now add to list of matrices
        modelMatrices[i] = model;
        qvector4d[i*4+0]=model.column(0); qvector4d[i*4+1]=model.column(1); qvector4d[i*4+2]=model.column(2); qvector4d[i*4+3]=model.column(3);
    }
    m_planet=new
            Model(QOpenGLContext::currentContext()->versionFunctions<QOpenGLFunctions_3_3_Core>() ,
                  "D:/Qt/4AdvanceOpenGL/Sampl_12_3Asteroid/planet/planet.obj");
    m_rock=new
            Model(QOpenGLContext::currentContext()->versionFunctions<QOpenGLFunctions_3_3_Core>() ,
                  "D:/Qt/4AdvanceOpenGL/Sampl_12_3Asteroid/rock/rock.obj");

    // vertex buffer object
    unsigned int buffer;
    glGenBuffers(1, &buffer);
    glBindBuffer(GL_ARRAY_BUFFER, buffer);
    glBufferData(GL_ARRAY_BUFFER, amount * 64, &qvector4d[0], GL_STATIC_DRAW);
    for(unsigned int i = 0; i < m_rock->meshes.size(); i++) {
        unsigned int VAO = m_rock->meshes[i].VAO;
        glBindVertexArray(VAO); // vertex attributes
        std::size_t vec4Size = sizeof(QVector4D);
        glEnableVertexAttribArray(3);
        glVertexAttribPointer(3, 4, GL_FLOAT, GL_FALSE, 4 * vec4Size, (void*)0); glEnableVertexAttribArray(4);
        glVertexAttribPointer(4, 4, GL_FLOAT, GL_FALSE, 4 * vec4Size, (void*)(1 * vec4Size));
        glEnableVertexAttribArray(5);
        glVertexAttribPointer(5, 4, GL_FLOAT, GL_FALSE, 4 * vec4Size, (void*)(2 * vec4Size));
        glEnableVertexAttribArray(6);
        glVertexAttribPointer(6, 4, GL_FLOAT, GL_FALSE, 4 * vec4Size, (void*)(3 * vec4Size));
        glVertexAttribDivisor(3, 1);
        glVertexAttribDivisor(4, 1);
        glVertexAttribDivisor(5, 1);
        glVertexAttribDivisor(6, 1);
        glBindVertexArray(0);
    }

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

    m_planet->Draw(m_ShaderProgram);

    m_RockShaderProgram.bind();
    m_RockShaderProgram.setUniformValue("projection", projection);
    m_RockShaderProgram.setUniformValue("view", view);

    //for(unsigned int i = 0; i < amount; i++) {
    //    m_ShaderProgram.setUniformValue("model", modelMatrices[i]);
    for (unsigned int i = 0; i < m_rock->meshes.size(); i++) {
        glBindTexture(GL_TEXTURE_2D, m_rock->meshes[i].textures[0].id);
        glBindVertexArray(m_rock->meshes[i].VAO);
        glDrawArraysInstanced(GL_TRIANGLES, 0,m_rock->meshes[i].vertices.size(),amount );
        glBindVertexArray(0);
    }
    // }
    m_RockShaderProgram.release();

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
