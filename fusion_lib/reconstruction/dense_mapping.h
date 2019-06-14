#ifndef DENSE_MAPPING_H
#define DENSE_MAPPING_H

#include <memory>
#include <cuda_runtime.h>
#include "rgbd_frame.h"
#include "map_struct.h"
#include "device_image.h"

namespace fusion
{

class DenseMapping
{
public:
  ~DenseMapping();
  DenseMapping(IntrinsicMatrix cam_params);
  void update(RgbdImagePtr frame);
  void update(cv::cuda::GpuMat depth, cv::cuda::GpuMat image, const Sophus::SE3d pose);
  void raycast(cv::cuda::GpuMat &vmap, cv::cuda::GpuMat &image, const Sophus::SE3d pose);

  void reset_mapping();

  size_t fetch_mesh_vertex_only(float3 *vertex);
  size_t fetch_mesh_with_normal(float3 *vertex, float3 *normal);
  size_t fetch_mesh_with_colour(float3 *vertex, uchar3 *normal);

private:
  IntrinsicMatrix cam_params;
  MapStruct device_map;

  // for map udate
  cv::cuda::GpuMat flag;
  cv::cuda::GpuMat pos_array;
  uint count_visible_block;
  HashEntry *visible_blocks;

  // for raycast
  cv::cuda::GpuMat zrange_x;
  cv::cuda::GpuMat zrange_y;
  uint count_rendering_block;
  RenderingBlock *rendering_blocks;
};

} // namespace fusion

#endif