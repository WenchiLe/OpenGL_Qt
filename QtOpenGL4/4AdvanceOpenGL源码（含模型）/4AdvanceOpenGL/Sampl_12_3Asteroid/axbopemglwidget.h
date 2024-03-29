﻿#ifndef AXBOPEMGLWIDGET_H
#define AXBOPEMGLWIDGET_H

#include <QOpenGLWidget>
#include <QOpenGLFunctions_3_3_Core>
#include <QOpenGLShaderProgram>
#include <QTime>
#include <QTimer>
#include <QWheelEvent>
#include "camera.h"
#include <QOpenGLTexture>
#include "model.h"
class AXBOpemglWidget : public QOpenGLWidget,QOpenGLFunctions_3_3_Core
{
    Q_OBJECT
public:
    struct ModelInfo{
        Model *model;
        QVector3D worldPos;
        float pitch;
        float yaw;
        float roll;
        bool isSelected;
        QString name;
    };


    explicit AXBOpemglWidget(QWidget *parent = nullptr);
    ~AXBOpemglWidget();

    void loadModel(string path);
protected:
    virtual void initializeGL();
    virtual void resizeGL(int w, int h);
    virtual void paintGL();

    void wheelEvent(QWheelEvent *event);
    void keyPressEvent(QKeyEvent *event);
    void mouseMoveEvent(QMouseEvent *event);
    void mousePressEvent(QMouseEvent *event);
    void mouseDoubleClickEvent(QMouseEvent *event);
signals:
    void mousePickingPos(QVector3D wolrdpos);
public slots:
    void on_timeout();
private:
    QTime m_time;
    QTimer m_timer;
    QOpenGLShaderProgram m_ShaderProgram;
    QOpenGLShaderProgram m_RockShaderProgram;
    Camera m_camera;
    QOpenGLTexture * m_BoxDiffuseTex;
    QOpenGLTexture * m_PlaneDiffuseTex;
    Model * m_planet;
    Model * m_rock;
    QMap<QString, ModelInfo> m_Models;
    QVector3D cameraPosInit(float maxY,float minY);
    Mesh* processMesh(float *vertices, int size,unsigned int textureId);
    bool m_modelMoving=false;

    QVector4D worldPosFromViewPort(int posX,int posY);
};

#endif // AXBOPEMGLWIDGET_H
