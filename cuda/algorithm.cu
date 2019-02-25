

#include "cuda.h"
#include "cuda_runtime.h"

#include "algorithm.cuh"
#include "utils/scimage.h"

__device__ u_char clamp(float t)
{
    if (t < 0) {
        return 0;
    } else if (t > 255){
        return 255;
    }

    return t;
}

inline unsigned int divUpper(int l, int dimension)
{
    return (l - 1) / dimension + 1;
}

__global__ void
kernel_colorSpaceYUV420PToRGBA(dev_t *src, dev_t *dst, int pitch_src, int pitch_dst, int w, int h)
{
    unsigned int dim_x = blockDim.x * blockIdx.x + threadIdx.x;
    unsigned int dim_y = blockDim.y * blockIdx.y + threadIdx.y;

    int r,g,b,y,u,v;
    if (dim_x < w && dim_y < h) {
        y = *((u_char*)src + dim_y * pitch_src + dim_x);
        u = *((u_char*)src + (h + dim_y / 4) * pitch_src + dim_x / 2);
        v = *((u_char*)src + (h * 5 + dim_y) / 4 * pitch_src + dim_x / 2);
        r = clamp(y + 1.402 * (v - 128) + 0.5);
        g = clamp(y - 0.34414 * (u - 128) - 0.71414 * (v - 128) + 0.5);
        b = clamp(y + 1.772 * (u - 128) + 0.5);
//        *((uint32_t*)dst + dim_y * pitch_dst / 4 + dim_x) = (r << 24) + (g << 16) + (b << 8);
        *((u_char*)dst + dim_y * pitch_dst + dim_x * 4) = r;
        *((u_char*)dst + dim_y * pitch_dst + dim_x * 4 + 1) = g;
        *((u_char*)dst + dim_y * pitch_dst + dim_x * 4 + 2) = b;
        *((u_char*)dst + dim_y * pitch_dst + dim_x * 4 + 3) = 255;
    }
}

__global__ void
kernel_colorSpaceRGBAToYUV420P(dev_t *src, dev_t *dst, int pitch_src, int pitch_dst, int w, int h)
{
    unsigned int dim_x = blockDim.x * blockIdx.x + threadIdx.x;
    unsigned int dim_y = blockDim.y * blockIdx.y + threadIdx.y;

    int r,g,b;
    unsigned int rgba;
    if (dim_x < w && dim_y < h) {
        rgba = *((uint32_t*)dst + dim_y * pitch_dst / 4 + dim_x);
        r = (rgba >> 24);
        g = (rgba >> 16) & 0xff;
        b = (rgba >> 8) & 0xff;
        *((u_char*)src + dim_y * pitch_src + dim_x) = clamp(0.299 * r + 0.587 * g + 0.114 * b);
        *((u_char*)src + (h + dim_y / 4) * pitch_src + dim_x / 2) = clamp(-0.1687 * r - 0.3313 * g + 0.5 * b + 128);
        *((u_char*)src + (h * 5 + dim_y) / 4 * pitch_src + dim_x / 2) = clamp(0.5 * r - 0.4187 * g - 0.0813 * b + 128);
    }
}

void ColorSpaceConvertion(ScGPUImage *src, ScGPUImage *dst)
{
    dim3 block(8, 8);
    dim3 grid((src->m_pixel_width - 1) / block.x + 1, (src->m_pixel_height - 1) / block.y + 1);
    if (ScGPUImage::ScPixFormat_YUV420P == src->m_pix_fmt)
    {
        switch (dst->m_pix_fmt)
        {
        case ScGPUImage::ScPixFormat_YUV420P:
            cudaMemcpy2D(dst->m_dev_addr, dst->m_data_width, src->m_dev_addr, src->m_data_width,
                         src->m_pixel_width, src->m_pixel_height * 3 / 2, cudaMemcpyDeviceToDevice);
            break;
        case ScGPUImage::ScPixFormat_RGBA:
            kernel_colorSpaceYUV420PToRGBA<<<grid, block>>>(src->m_dev_addr, dst->m_dev_addr,
                                                               src->m_data_width, dst->m_data_width,
                                                               src->m_pixel_width, src->m_pixel_height);
            break;
        }
    } else if (ScGPUImage::ScPixFormat_RGBA == src->m_pix_fmt){
        switch (dst->m_pix_fmt)
        {
        case ScGPUImage::ScPixFormat_YUV420P:
            kernel_colorSpaceRGBAToYUV420P<<<grid, block, 0>>>(src->m_dev_addr, dst->m_dev_addr,
                                                               src->m_data_width, dst->m_data_width,
                                                               src->m_pixel_width, src->m_pixel_height);
            break;
        case ScGPUImage::ScPixFormat_RGBA:
            cudaMemcpy2D(dst->m_dev_addr, dst->m_data_width, src->m_dev_addr, src->m_data_width,
                         src->m_pixel_width * 4, src->m_pixel_height, cudaMemcpyDeviceToDevice);
            break;
        }
    }
}

__global__ void
kernel_horizontalReversal(dev_t *src, dev_t *dst, uint pitch_src, uint pitch_dst, uint pixel_w, uint pixel_h)
{
    unsigned int dim_x = blockDim.x * blockIdx.x + threadIdx.x;
    unsigned int dim_y = blockDim.y * blockIdx.y + threadIdx.y;

    if (dim_x < pixel_w && dim_y < pixel_h) {
        unsigned int rgba = *((uint32_t*)src + dim_y * pitch_src / 4 + dim_x);
        *((uint32_t*)dst + dim_y * pitch_dst / 4 + pixel_w - dim_x) = rgba;
    }
}

__global__ void
kernel_verticalReversal(dev_t *src, dev_t *dst, uint pitch_src, uint pitch_dst, uint pixel_w, uint pixel_h)
{
    unsigned int dim_x = blockDim.x * blockIdx.x + threadIdx.x;
    unsigned int dim_y = blockDim.y * blockIdx.y + threadIdx.y;

    if (dim_x < pixel_w && dim_y < pixel_h) {
        unsigned int rgba = *((uint32_t*)src + dim_y * pitch_src / 4 + dim_x);
        *((uint32_t*)dst + (pixel_h - dim_y) * pitch_dst / 4 + dim_x) = rgba;
    }
}

void HorizontalReversalRGBA(ScGPUImage *src, ScGPUImage *dst)
{
    dim3 block(8, 8);
    dim3 grid(divUpper(src->m_pixel_width, block.x), divUpper(src->m_pixel_height, block.y));
    kernel_horizontalReversal<<<grid, block>>>(src->m_dev_addr, dst->m_dev_addr, src->m_data_width, dst->m_data_width,
                                               src->m_pixel_width, src->m_pixel_height);

}

void VerticalReversalRGBA(ScGPUImage *src, ScGPUImage *dst)
{
    dim3 block(8, 8);
    dim3 grid(divUpper(src->m_pixel_width, block.x), divUpper(src->m_pixel_height, block.y));
    kernel_verticalReversal<<<grid, block>>>(src->m_dev_addr, dst->m_dev_addr, src->m_data_width, dst->m_data_width,
                                             src->m_pixel_width, src->m_pixel_height);
}

__global__ void
kernel_renderRGBA2Surface(cudaSurfaceObject_t surface, dev_t *src, int pitch, int pixel_w, int pixel_h)
{
    unsigned int dim_x = blockDim.x * blockIdx.x + threadIdx.x;
    unsigned int dim_y = blockDim.y * blockIdx.y + threadIdx.y;

    if (dim_x < pixel_w && dim_y < pixel_h)
    {
        u_char r,g,b;
        r = *((u_char*)src + dim_y * pitch + dim_x * 4);
        g = *((u_char*)src + dim_y * pitch + dim_x * 4 + 1);
        b = *((u_char*)src + dim_y * pitch + dim_x * 4 + 2);
        uchar4 data = make_uchar4(r, g, b, 0xff);
        surf2Dwrite(data, surface, dim_x * sizeof(uchar4), dim_y);
    }
}

void RenderRGBAImageToSurface(ScGPUImage *image, cudaSurfaceObject_t surface)
{
    dim3 block(8, 8);
    dim3 grid((image->m_pixel_width - 1) / block.x + 1, (image->m_pixel_height - 1) / block.y + 1);
    kernel_renderRGBA2Surface<<<grid, block>>>(surface, image->m_dev_addr,
                                                image->m_data_width, image->m_pixel_width, image->m_pixel_height);
}
