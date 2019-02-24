#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QLabel>
#include <QGridLayout>
#include <QMainWindow>
#include <QPixmap>

#include "glcudawidget.h"
#include "components/controller.h"

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(int w, int h, QWidget *parent = 0);
    ~MainWindow();

    void setGlTextureSizeTemporal(int w, int h);

public slots:
    void slot_update(int id);
    void slot_openfile();

protected:
    void resizeEvent(QResizeEvent *event) Q_DECL_OVERRIDE;

private:
    Ui::MainWindow *ui;

    static const int NUM_WIDGETS = 9;
    QLabel*      mLabel[NUM_WIDGETS];
    GlCudaWidget *m_glcudaWidgets[NUM_WIDGETS];
    QGridLayout  *mLayout;

    QPixmap *ma_pixmap[NUM_WIDGETS];

    Controller   m_controller;
};

#endif // MAINWINDOW_H
