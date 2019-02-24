

#include "glcudawidget.h"
#include "cuda_gl_interop.h"

GlCudaWidget::GlCudaWidget(QWidget *parent) : QOpenGLWidget(parent)
{
    m_id     = 0;
    m_cur_image = nullptr;
}

GlCudaWidget::~GlCudaWidget()
{
    makeCurrent();

    doneCurrent();
}

void GlCudaWidget::setWid(int wid)
{
    m_id = wid;
}

void GlCudaWidget::setTextureSize(int width, int height)
{
    m_width  = width;
    m_height = height;
}

void GlCudaWidget::updateWidget()
{
    makeCurrent();

    /* return current image */
    m_err = cudaGraphicsMapResources(1, &m_cur_image->m_resource, NULL);
    m_err = cudaGraphicsSubResourceGetMappedArray(&m_cur_image->m_array_addr, m_cur_image->m_resource, 0, 0);
    cudaResourceDesc viewCudaArrayResourceDesc;
    viewCudaArrayResourceDesc.resType = cudaResourceTypeArray;
    viewCudaArrayResourceDesc.res.array.array = m_cur_image->m_array_addr;
    m_err = cudaCreateSurfaceObject(&m_cur_image->m_cuda_surface, &viewCudaArrayResourceDesc);
    m_image_pool.returnFreeBuffer(m_cur_image);

    /* set up image */
    ScGPUImage *gpuimage = m_image_pool.getReadyBuffer();
    m_err = cudaDestroySurfaceObject(gpuimage->m_cuda_surface);
    m_err = cudaGraphicsUnmapResources(1, &gpuimage->m_resource, NULL);
    m_cur_image = gpuimage;
    doneCurrent();

    update();
}

void GlCudaWidget::initializeGL()
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
//    m_glProgram->setUniformValue("texture", 0);

    m_cur_image = createGPUImage(true);
    for (int i = 0; i < GPU_BUFERR_NUM; i++)
    {
        ScGPUImage *gpuimage = createGPUImage();
        m_image_pool.returnFreeBuffer(gpuimage);
    }

    emit signal_gpuImagePoolReady(&m_image_pool, m_id);
}

void GlCudaWidget::paintGL()
{
    if (nullptr == m_cur_image) {
        return;
    }

    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, m_cur_image->m_pbo);
    m_glProgram->bind();

    float vertices_value[] = {-1, -1, 1, -1, -1, 1, 1, 1};
    GLint vertices_location = m_glProgram->attributeLocation("vertices");
    m_glProgram->enableAttributeArray(vertices_location);
    m_glProgram->setAttributeArray(vertices_location, GL_FLOAT, vertices_value, 2);

    float texc_value[] = {0, 1, 1, 1, 0, 0, 1, 0};
    GLint texCoord_location = m_glProgram->attributeLocation("texCoord");
    m_glProgram->enableAttributeArray(texCoord_location);
    m_glProgram->setAttributeArray(texCoord_location, GL_FLOAT, texc_value, 2);

    glClearColor(0, 0, 255, 0);
    glClear(GL_COLOR_BUFFER_BIT);

    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

    m_glProgram->disableAttributeArray(0);

    m_glProgram->release();
    glBindTexture(GL_TEXTURE_2D, 0);
}

void GlCudaWidget::resizeGL(int w, int h)
{
    glViewport(0, 0, w, h);
}

ScGPUImage* GlCudaWidget::createGPUImage(bool unmap)
{
    ScGPUImage *gpuimage = new ScGPUImage();
    gpuimage->m_pixel_width  = m_width;
    gpuimage->m_pixel_height = m_height;
    gpuimage->m_data_width   = m_width * 4;
    gpuimage->m_data_height  = m_height;
    gpuimage->m_pix_fmt      = ScGPUImage::ScPixFormat_RGBA;

    glEnable(GL_TEXTURE_2D);
    glGenTextures(1, &gpuimage->m_pbo);
    glBindTexture(GL_TEXTURE_2D, gpuimage->m_pbo);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, m_width, m_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);

    m_err = cudaGraphicsGLRegisterImage(&gpuimage->m_resource, gpuimage->m_pbo, GL_TEXTURE_2D, cudaGraphicsMapFlagsNone);
    m_err = cudaGraphicsMapResources(1, &gpuimage->m_resource, NULL);
    m_err = cudaGraphicsSubResourceGetMappedArray(&gpuimage->m_array_addr, gpuimage->m_resource, 0, 0);
    cudaResourceDesc viewCudaArrayResourceDesc;
    viewCudaArrayResourceDesc.resType = cudaResourceTypeArray;
    viewCudaArrayResourceDesc.res.array.array = gpuimage->m_array_addr;
    m_err = cudaCreateSurfaceObject(&gpuimage->m_cuda_surface, &viewCudaArrayResourceDesc);

    if (unmap) {
        m_err = cudaGraphicsUnmapResources(1, &gpuimage->m_resource, NULL);
    }
    glBindTexture(GL_TEXTURE_2D, 0);

    return gpuimage;
}
