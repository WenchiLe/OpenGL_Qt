#include "axbopenglwidget.h"

unsigned int VBO, VAO,EBO;
float vertices[] = {
-0.5f, -0.5f, -0.5f, 0.0f, 0.0f,
 0.5f, -0.5f, -0.5f, 1.0f, 0.0f,
 0.5f, 0.5f, -0.5f, 1.0f, 1.0f,
 0.5f, 0.5f, -0.5f, 1.0f, 1.0f,
-0.5f, 0.5f, -0.5f, 0.0f, 1.0f,
-0.5f, -0.5f, -0.5f, 0.0f, 0.0f,

-0.5f, -0.5f, 0.5f, 0.0f, 0.0f,
0.5f, -0.5f, 0.5f, 1.0f, 0.0f,
0.5f, 0.5f, 0.5f, 1.0f, 1.0f,
0.5f, 0.5f, 0.5f, 1.0f, 1.0f,
-0.5f, 0.5f, 0.5f, 0.0f, 1.0f,
-0.5f, -0.5f, 0.5f, 0.0f, 0.0f,

-0.5f, 0.5f, 0.5f, 1.0f, 0.0f,
-0.5f, 0.5f, -0.5f, 1.0f, 1.0f,
-0.5f, -0.5f, -0.5f, 0.0f, 1.0f,
-0.5f, -0.5f, -0.5f, 0.0f, 1.0f,
-0.5f, -0.5f, 0.5f, 0.0f, 0.0f,
-0.5f, 0.5f, 0.5f, 1.0f, 0.0f,

 0.5f, 0.5f, 0.5f, 1.0f, 0.0f,
 0.5f, 0.5f, -0.5f, 1.0f, 1.0f,
 0.5f, -0.5f, -0.5f, 0.0f, 1.0f,
 0.5f, -0.5f, -0.5f, 0.0f, 1.0f,
 0.5f, -0.5f, 0.5f, 0.0f, 0.0f,
 0.5f, 0.5f, 0.5f, 1.0f, 0.0f,

-0.5f, -0.5f, -0.5f, 0.0f, 1.0f,
 0.5f, -0.5f, -0.5f, 1.0f, 1.0f,
 0.5f, -0.5f, 0.5f, 1.0f, 0.0f,
 0.5f, -0.5f, 0.5f, 1.0f, 0.0f,
-0.5f, -0.5f, 0.5f, 0.0f, 0.0f,
-0.5f, -0.5f, -0.5f, 0.0f, 1.0f,

-0.5f, 0.5f, -0.5f, 0.0f, 1.0f,
 0.5f, 0.5f, -0.5f, 1.0f, 1.0f,
 0.5f, 0.5f, 0.5f, 1.0f, 0.0f,
 0.5f, 0.5f, 0.5f, 1.0f, 0.0f,
-0.5f, 0.5f, 0.5f, 0.0f, 0.0f,
-0.5f, 0.5f, -0.5f, 0.0f, 1.0f
};

QVector<QVector3D> cubePositions= {
QVector3D( 0.0f, 0.0f, 0.0f),
QVector3D( 2.0f, 5.0f, -15.0f),
QVector3D(-1.5f, -2.2f, -2.5f),
QVector3D(-3.8f, -2.0f, -12.3f),
QVector3D( 2.4f, -0.4f, -3.5f),
QVector3D(-1.7f, 3.0f, -7.5f),
QVector3D( 1.3f, -2.0f, -2.5f),
QVector3D( 1.5f, 2.0f, -2.5f),
QVector3D( 1.5f, 0.2f, -1.5f),
QVector3D(-1.3f, 1.0f, -1.5f)
};


unsigned int indices[] = { // note that we start from 0!
                           0, 1, 3, // first triangle
                           1, 2, 3 // second triangle
                         };
float ratio=0.5;
AXBOpenGLWidget::AXBOpenGLWidget(QWidget *parent) : QOpenGLWidget(parent)
{
    setFocusPolicy(Qt::StrongFocus);
    connect(&timer,SIGNAL(timeout()),this,SLOT(on_timeout()));
    timer.start(100);
    m_time.start();

    //Camera position
    cameraPos = QVector3D(0.0f, 0.0f, 2.0f);
    //Camera direction
    cameraTarget = QVector3D(0.0f, 0.0f, 0.0f);
    cameraDirection = QVector3D(cameraPos - cameraTarget);
    cameraDirection.normalize();
    //Right axis
    up = QVector3D(0.0f, 1.0f, 0.0f);
    cameraRight = QVector3D::crossProduct(up, cameraDirection);
    cameraRight.normalize();
    //Up axis
    cameraUp = QVector3D::crossProduct(cameraDirection, cameraRight);

}

AXBOpenGLWidget::~AXBOpenGLWidget()
{
    makeCurrent();
    glDeleteBuffers(1,&VBO);
    glDeleteBuffers(1,&EBO);
    glDeleteVertexArrays(1,&VAO);
    doneCurrent();
}

void AXBOpenGLWidget::drawShape(AXBOpenGLWidget::Shape shape)
{
    m_shape=shape;
    update();
}

void AXBOpenGLWidget::setWirefame(bool wireframe)
{
    makeCurrent();
    if(wireframe)
        glPolygonMode(GL_FRONT_AND_BACK,GL_LINE);
    else
        glPolygonMode(GL_FRONT_AND_BACK,GL_FILL);
    update();
    doneCurrent();
}

void AXBOpenGLWidget::initializeGL()
{
    initializeOpenGLFunctions();

    //创建VBO和VAO对象，并赋予ID
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);

    //绑定VBO和VAO对象
    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);

    //为当前绑定到target的缓冲区对象创建一个新的数据存储。
    //如果data不是NULL，则使用来自此指针的数据初始化数据存储
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    //告知显卡如何解析缓冲里的属性值
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
    //开启VAO管理的第一个属性值
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glBindBuffer(GL_ARRAY_BUFFER, 0);

    bool success;
    shaderProgram.addShaderFromSourceFile(QOpenGLShader::Vertex,":/shaders/shapes.vert");
    shaderProgram.addShaderFromSourceFile(QOpenGLShader::Fragment,":/shaders/shapes.frag");
    success=shaderProgram.link();
    if(!success)
        qDebug()<<"ERR:"<<shaderProgram.log();

    // glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    glGenBuffers(1, &EBO);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
    textureWall=new QOpenGLTexture(QImage(":/images/images/wall.jpg").mirrored());
    textureSmile=new QOpenGLTexture(QImage(":/images/images/awesomeface.png").mirrored());
    textureSmall=new QOpenGLTexture(QImage(":/images/images/small.png").mirrored());
    shaderProgram.bind();
    shaderProgram.setUniformValue("textureWall",0);
    shaderProgram.setUniformValue("textureSmile",1);
    shaderProgram.setUniformValue("textureSmall",2);
    textureSmall->generateMipMaps();


    textureSmile->bind(1);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_MIRRORED_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glBindVertexArray(0);

    QMatrix4x4 projection;
    projection.perspective(45,(float)width()/height(),0.1,100);
    shaderProgram.setUniformValue("projection", projection);
}

void AXBOpenGLWidget::resizeGL(int w, int h)
{
    Q_UNUSED(w);Q_UNUSED(h);
    //glViewport(0, 0, w, h);
}

void AXBOpenGLWidget::paintGL()
{
    QMatrix4x4 model;
    QMatrix4x4 view;

    const float radius = 10.0f;
    float time=m_time.elapsed()/1000.0;
    float camX = sin(time) * radius;
    float camZ = cos(time) * radius;
    view.lookAt(QVector3D(camX, 0.0, camZ), cameraTarget,up);


    //view.lookAt(cameraPos,cameraTarget,up);

    glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
    glEnable(GL_DEPTH_TEST);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    shaderProgram.bind();
    shaderProgram.setUniformValue("ratio",ratio);
    glBindVertexArray(VAO);

    switch (m_shape) {
    case Rect:
       // glActiveTexture(GL_TEXTURE0);
        textureWall->bind(0);
        //glActiveTexture(GL_TEXTURE1);
        textureSmile->bind(1);
        textureSmall->bind(2);

        shaderProgram.setUniformValue("view", view);

        foreach (auto item, cubePositions) {
            model.setToIdentity();
            model.translate(item);
            model.rotate(time, 1.0f, 5.0f, 0.5f);
            shaderProgram.setUniformValue("model", model);
            glDrawArrays(GL_TRIANGLES,0,36);
        }
        break;
    default:
        break;
    }
}
#include <QKeyEvent>
void AXBOpenGLWidget::keyPressEvent(QKeyEvent *event)
{
    switch (event->key()) {
    case Qt::Key_Up:
        ratio+=0.1;
        break;
    case Qt::Key_Down:
        ratio-=0.1;
        break;
    default:
        break;
    }
    if(ratio>1) ratio=1;
    if(ratio<0) ratio=0;

    shaderProgram.bind();
    shaderProgram.setUniformValue("ratio",ratio);
    update();
}

void AXBOpenGLWidget::on_timeout()
{
    update();
}
