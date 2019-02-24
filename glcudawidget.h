#ifndef GLCUDAWIDGET_H
#define GLCUDAWIDGET_H

#include "cuda.h"
#include "cuda_runtime.h"

#include <QOpenGLWidget>
#include <QOpenGLTexture>
#include <QOpenGLFunctions>
#include <QOpenGLShaderProgram>

#include "utils/scimage.h"
#include "utils/sharedbufferpool.h"

class GlCudaWidget : public QOpenGLWidget, protected QOpenGLFunctions
{
    Q_OBJECT
public:
    explicit GlCudaWidget(QWidget *parent = 0);
    ~GlCudaWidget();

    void setWid(int wid);
    void setTextureSize(int width, int height);

signals:
    void signal_gpuImagePoolReady(SharedBufferPool<ScGPUImage *> *, int);

public slots:
    void updateWidget();

protected:
    void initializeGL();
    void paintGL();
    void resizeGL(int w, int h);
//    void paintEvent(QPaintEvent *e);

private:
    int m_id;
    int m_width;
    int m_height;

    QOpenGLShaderProgram *m_glProgram;

    cudaError m_err;
    static const int GPU_BUFERR_NUM = 4;
    ScGPUImage *m_cur_image;
    SharedBufferPool<ScGPUImage *> m_image_pool;

    ScGPUImage *createGPUImage(bool unmap = false);
};

#endif // GLCUDAWIDGET_H
