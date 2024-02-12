#ifndef AXBOPENGLWIDGET_H
#define AXBOPENGLWIDGET_H

#include <QOpenGLWidget>
#include <QOpenGLFunctions_3_3_Core>
#include <QOpenGLShaderProgram>
#include <QOpenGLTexture>
#include <QTimer>
#include <QTime>
#include "camera.h"
class AXBOpenGLWidget : public QOpenGLWidget,QOpenGLFunctions_3_3_Core
{
    Q_OBJECT
public:
    enum Shape{None,Rect,Circle,Triangle};
    explicit AXBOpenGLWidget(QWidget *parent = nullptr);
    ~AXBOpenGLWidget();

    void drawShape(Shape shape);
    void setWirefame(bool wireframe);
protected:
    virtual void initializeGL();
    virtual void resizeGL(int w, int h);
    virtual void paintGL();
    void keyPressEvent(QKeyEvent *event);
    void mouseMoveEvent(QMouseEvent *event);
    void wheelEvent(QWheelEvent *event);
signals:

public slots:
    void on_timeout();
private:
    Shape m_shape;
    QOpenGLShaderProgram shaderProgram;
    QOpenGLTexture * textureWall;
    QOpenGLTexture * textureSmile;
    QOpenGLTexture * textureSmall;
    QTimer timer;
    QTime m_time;
    Camera m_camera;
};



#endif // AXBOPENGLWIDGET_H
