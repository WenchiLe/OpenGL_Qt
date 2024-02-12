#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QVector3D>

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

private slots:
    void on_actLoadModel_triggered();
    void getMousePickingPos(QVector3D pos);
private:
    Ui::MainWindow *ui;
};

#endif // MAINWINDOW_H
