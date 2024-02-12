#include "axbopenglwidget.h"

unsigned int VBO, VAO,EBO;
float vertices[] = { // positions // colors
                     0.5f, 0.5f, 0.0f, 1.0f, 0.0f, 0.0f,	// top right
                     0.5f, -0.5f, 0.0f, 0.0f, 1.0f, 0.0f,	// bottom right
                     -0.5f, -0.5f, 0.0f, 0.0f, 0.0f, 1.0f, 	// bottom left
                     -0.5f, 0.5f, 0.0f, 0.5f, 0.5f, 0.5f,	// top left
                   };

unsigned int indices[] = { // note that we start from 0!

                           1, 2, 3, // second triangle
                           0, 1, 3, // first triangle
                         };

AXBOpenGLWidget::AXBOpenGLWidget(QWidget *parent) : QOpenGLWidget(parent)
{

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
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    //开启VAO管理的第一个属性值
    glEnableVertexAttribArray(0);

    //告知显卡如何解析缓冲里的属性值
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
    //开启VAO管理的第一个属性值
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
    glBindVertexArray(0);

    shaderProgram.bind();
    shaderProgram.setUniformValue("xOffset",0.4f);
}

void AXBOpenGLWidget::resizeGL(int w, int h)
{
    Q_UNUSED(w);Q_UNUSED(h);
    //glViewport(0, 0, w, h);
}

void AXBOpenGLWidget::paintGL()
{
    glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    shaderProgram.bind();
    glBindVertexArray(VAO);

    switch (m_shape) {
    case Rect:
        glDrawElements(GL_TRIANGLES, 3, GL_UNSIGNED_INT, NULL);
        break;
    default:
        break;
    }
}
