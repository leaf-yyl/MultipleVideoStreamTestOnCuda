#include "scglwidget.h"

ScGLWidget::ScGLWidget(QWidget *parent) : QOpenGLWidget(parent)
{
    m_glTexture = NULL;
}

ScGLWidget::~ScGLWidget()
{
    makeCurrent();
    if (NULL != m_glTexture) {
        delete m_glTexture;
    }

    if (NULL != m_glProgram) {
        delete m_glProgram;
    }
    doneCurrent();
}

void ScGLWidget::loadImage(QString path)
{
    m_imageToLoad = path;
}

void ScGLWidget::initializeGL()
{
    initializeOpenGLFunctions();

    QOpenGLShader *vshader = new QOpenGLShader(QOpenGLShader::Vertex, this);
    const char *vsrc =
        "attribute highp vec4 vertices;\n"
        "attribute mediump vec4 texCoord;\n"
        "varying mediump vec4 texc;\n"
        "void main(void)\n"
        "{\n"
        "    gl_Position = vertices;\n"
        "    texc = texCoord;\n"
        "}\n";
    vshader->compileSourceCode(vsrc);

    QOpenGLShader *fshader = new QOpenGLShader(QOpenGLShader::Fragment, this);
    const char *fsrc =
        "uniform sampler2D texture;\n"
        "varying mediump vec4 texc;\n"
        "void main(void)\n"
        "{\n"
            "   vec4 color = texture2D(texture, texc.st);\n"
            "	float lu = 0.299*color.x + 0.587*color.y + 0.114*color.z;\n"
            "	gl_FragColor = vec4(lu, lu, lu, 0.0);\n"
        "}\n";
    fshader->compileSourceCode(fsrc);

    m_glProgram = new QOpenGLShaderProgram;
    m_glProgram->addShader(vshader);
    m_glProgram->addShader(fshader);
    m_glProgram->link();

    /* Here we only use single texture input, so texture id is always 0 */
    m_glProgram->setUniformValue("texture", 0);
}

void ScGLWidget::paintGL()
{
    if (!m_imageToLoad.isEmpty()) {
        m_image.load(m_imageToLoad);
        if (!m_image.isNull()) {
            delete m_glTexture;
            m_glTexture = new QOpenGLTexture(m_image);
            m_imageLoaded = m_imageToLoad;
        }
        m_imageToLoad.clear();
    }

    if (m_imageLoaded.isEmpty()) {
        return;
    }

    m_glTexture->bind(0);
    m_glProgram->bind();

    float vertices_value[] = {-1, -1, 1, -1, -1, 1, 1, 1};
    GLint vertices_location = m_glProgram->attributeLocation("vertices");
    m_glProgram->enableAttributeArray(vertices_location);
    m_glProgram->setAttributeArray(vertices_location, GL_FLOAT, vertices_value, 2);

    float texc_value[] = {0, 1, 1, 1, 0, 0, 1, 0};
    GLint texCoord_location = m_glProgram->attributeLocation("texCoord");
    m_glProgram->enableAttributeArray(texCoord_location);
    m_glProgram->setAttributeArray(texCoord_location, GL_FLOAT, texc_value, 2);

    glClearColor(0, 1, 0, 1);
    glClear(GL_COLOR_BUFFER_BIT);

    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

    m_glProgram->disableAttributeArray(0);

    m_glProgram->release();
    m_glTexture->release();
}

void ScGLWidget::resizeGL(int w, int h)
{
    glViewport(0, 0, w, h);
}
