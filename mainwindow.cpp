
#include <QFileDialog>

#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(int w, int h, QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    QMenu *menu     = NULL;
    QAction *action = NULL;

    menu = new QMenu("File", ui->menuBar);
    action = new QAction("Open", menu);
    connect(action, SIGNAL(triggered(bool)), this, SLOT(slot_openfile()));

    menu->addAction(action);
    ui->menuBar->addMenu(menu);

    mLayout = new QGridLayout(ui->centralWidget);
    mLayout->setHorizontalSpacing(5);
    mLayout->setVerticalSpacing(5);
    mLayout->setColumnStretch(2, 0);
    for(int i = 0; i < NUM_WIDGETS; i++)
    {
        m_glcudaWidgets[i] = new GlCudaWidget(this);
        m_glcudaWidgets[i]->setWid(i);
        m_glcudaWidgets[i]->setTextureSize(w, h);
        m_glcudaWidgets[i]->setMinimumSize(320, 180);
        m_glcudaWidgets[i]->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

        connect(m_glcudaWidgets[i], SIGNAL(signal_gpuImagePoolReady(SharedBufferPool<ScGPUImage*>*,int)),
                &m_controller, SLOT(slot_setGpuImagePool(SharedBufferPool<ScGPUImage*>*,int)));

        mLayout->addWidget(m_glcudaWidgets[i]);

//        mLabel[i] = new QLabel(this);
//        mLabel[i]->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
//        mLabel[i]->setAlignment(Qt::AlignCenter);
//        mLabel[i]->setMinimumSize(320, 180);
//        ma_pixmap[i] = new QPixmap("/home/leaf_yyl/Pictures/magic.jpg");
//        mLayout->addWidget(mLabel[i]);
    }

    connect(&m_controller, SIGNAL(signal_update(int)),
            this, SLOT(slot_update(int)));
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::setGlTextureSizeTemporal(int w, int h)
{

}

void MainWindow::slot_update(int id)
{
    m_glcudaWidgets[id]->updateWidget();
}

void MainWindow::slot_openfile()
{
    QString filename = QFileDialog::getOpenFileName(this, "Open video file", "/home");
    if (filename.isEmpty()) {
        return;
    }

    m_controller.addInputFile(filename);
}

void MainWindow::resizeEvent(QResizeEvent * /* event */)
{
    return;
    for (int i = 0; i < NUM_WIDGETS; i++)
    {
        QSize scaledSize = ma_pixmap[i]->size();
        scaledSize.scale(mLabel[i]->size(), Qt::KeepAspectRatio);
        if (!mLabel[i]->pixmap() || scaledSize != mLabel[i]->pixmap()->size())
            mLabel[i]->setPixmap(ma_pixmap[i]->scaled(mLabel[i]->size(),
                                                             Qt::KeepAspectRatio,
                                                             Qt::SmoothTransformation));
    }
}
