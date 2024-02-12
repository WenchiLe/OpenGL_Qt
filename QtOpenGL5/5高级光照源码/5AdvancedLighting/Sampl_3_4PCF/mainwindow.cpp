#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    setCentralWidget(ui->openGLWidget);
    connect(ui->openGLWidget,SIGNAL(mousePickingPos(QVector3D)),
            this,SLOT(getMousePickingPos(QVector3D)));
}

MainWindow::~MainWindow()
{
    delete ui;
}
#include <QFileDialog>
void MainWindow::on_actLoadModel_triggered()
{
    QString str=QFileDialog::getOpenFileName(this,"选择模型文件","",
                                             "OBJ (*.obj);;FBX(*.fbx);;ALL FILES( *.* ) ");
    //ui->openGLWidget->loadModel(str.toStdString());
}

void MainWindow::getMousePickingPos(QVector3D pos)
{
    ui->statusBar->setStyleSheet("font: 14pt ");
    ui->statusBar->showMessage(" 世界坐标:    X:"
                               +QString::number(pos.x(),'f', 2)
                               +" Y:"+QString::number(pos.y(),'f', 2)
                               +" Z:"+QString::number(pos.z(),'f', 2)
                               );
}
