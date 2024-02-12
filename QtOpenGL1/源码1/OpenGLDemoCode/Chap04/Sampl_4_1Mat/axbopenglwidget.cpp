#include "axbopenglwidget.h"
#include <QTime>
unsigned int VBO, VAO,EBO;
float vertices[] = {
    // positions      // colors        // texture coords
    0.3f, 0.3f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f, // top right
    0.3f, -0.3f, 0.0f, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f, // bottom right
    -0.3f, -0.3f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, // bottom left
    -0.3f, 0.3f, 0.0f, 1.0f, 1.0f, 0.0f, 0.0f, 1.0f // top left
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
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
    //开启VAO管理的第一个属性值
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);


    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);

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
}

void AXBOpenGLWidget::resizeGL(int w, int h)
{
    Q_UNUSED(w);Q_UNUSED(h);
    //glViewport(0, 0, w, h);
}

void AXBOpenGLWidget::paintGL()
{
    QMatrix4x4 matrix;
    unsigned int time=QTime::currentTime().msec();
    matrix.translate(0.5,-0.5,0);
    matrix.rotate(time, 0.0f, 0.0f, 1.0f);


    glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

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
        shaderProgram.setUniformValue("theMatrix", matrix);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, NULL);
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
