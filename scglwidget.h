#ifndef SCGLWIDGET_H
#define SCGLWIDGET_H

#include <QOpenGLWidget>
#include <QOpenGLTexture>
#include <QOpenGLFunctions>
#include <QOpenGLShaderProgram>

class ScGLWidget : public QOpenGLWidget, protected QOpenGLFunctions
{
public:
    ScGLWidget(QWidget* parent = Q_NULLPTR);
    ~ScGLWidget();

    void loadImage(QString path);

protected:
    void initializeGL();
    void paintGL();
    void resizeGL(int w, int h);

private:
    QImage                m_image;
    QString               m_imageToLoad;
    QString               m_imageLoaded;
    QOpenGLTexture       *m_glTexture;
    QOpenGLShaderProgram *m_glProgram;
};

#endif // SCGLWIDGET_H
