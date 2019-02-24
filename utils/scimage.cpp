
#include <QDebug>

#include "scimage.h"

ScGPUImage::ScGPUImage()
{
    m_data_width   = 0;
    m_data_height  = 0;
    m_pixel_width  = 0;
    m_pixel_height = 0;
    m_align    = DEFAULT_IMAGE_ALIGNMENT;
    m_pix_fmt  = ScPixFormat_Unknown;
    m_dev_addr = nullptr;
    m_resource = nullptr;
    m_cuda_surface = 0;
}

ScGPUImage::~ScGPUImage()
{
    if (nullptr != m_dev_addr) {
        cudaFree(m_dev_addr);
        m_dev_addr = nullptr;
    }
}

ScGPUImage::enScPixFormat ScGPUImage::getScPixFormat(AVPixelFormat format)
{
    enScPixFormat pix_fmt = ScPixFormat_Unknown;
    switch (format) {
    case AV_PIX_FMT_YUV420P:
        pix_fmt = ScPixFormat_YUV420P;
        break;
    case AV_PIX_FMT_RGBA:
        pix_fmt = ScPixFormat_RGBA;
        break;
    default:
        break;
    }

    return pix_fmt;
}

bool ScGPUImage::copyFromAVFrame(AVFrame *frame)
{
    bool ret = true;
    enScPixFormat pix_fmt = getScPixFormat(frame->format);
    switch (pix_fmt) {
    case ScPixFormat_YUV420P:
        resize(frame->width, frame->height, ScPixFormat_YUV420P);
        cudaError err = cudaMemcpy2D(m_dev_addr, m_data_width, frame->data[0], frame->linesize[0],
                frame->linesize[0], frame->height * 3 / 2, cudaMemcpyHostToDevice);
        if (cudaSuccess != err) {
            qDebug( cudaGetErrorString(err) );
            ret = false;
        }
        break;
    case ScPixFormat_RGBA:
        resize(frame->width, frame->height, ScPixFormat_RGBA);
        cudaMemcpy2D(m_dev_addr, m_data_width, frame->data[0], frame->linesize[0],
                frame->linesize[0], frame->height, cudaMemcpyHostToDevice);
        break;
    default:
        qDebug("Unknown pixel format-->%d", frame->format);
        ret = false;
        break;
    }

    return ret;
}


void ScGPUImage::resize(int pixel_w, int pixel_h, enScPixFormat format)
{
    int data_width = 0;
    int data_height = pixel_h;
    switch (format) {
    case ScPixFormat_YUV420P:
        data_height = data_height * 3 / 2;
        data_width = ((pixel_w - 1) / m_align + 1) * m_align;
        break;
    case ScPixFormat_RGBA:
        data_width = ((pixel_w - 1) / m_align + 1) * m_align * 4;
    default:
        break;
    }

    if (m_data_width < data_width || m_data_height < data_height) {
        if (nullptr != m_dev_addr) {
            cudaFree(m_dev_addr);
        }
        cudaMalloc(&m_dev_addr, data_width * data_height);
        m_data_width  = data_width;
        m_data_height = data_height;
    }

    m_pix_fmt = format;
    m_pixel_width  = pixel_w;
    m_pixel_height = pixel_h;
}
