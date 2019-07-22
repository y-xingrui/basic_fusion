#include "system_new.h"
#include "utils/safe_call.h"
#include "tracking/cuda_imgproc.h"

namespace fusion
{

SystemNew::SystemNew(const IntrinsicMatrix K, const int NUM_PYR)
    : initialized(false), current_frame_id(0)
{
    this->K = Eigen::Matrix3f::Identity();
    this->K(0, 0) = K.fx;
    this->K(1, 1) = K.fy;
    this->K(0, 2) = K.cx;
    this->K(1, 2) = K.cy;

    mapper = std::make_shared<DenseMapping>(K);
    tracker = std::make_shared<DenseTracking>(K, NUM_PYR);
}

void SystemNew::spawn_work(const cv::Mat &depth, const cv::Mat &image)
{
    populate_current_data(depth, image);

    if (!initialized)
    {
        current_pose = Sophus::SE3d();
        last_tracked = keyframe = current;
        mapper->update(current->depth, current->image, Sophus::SE3d());
        initialized = true;
        return;
    }

    mapper->raycast(vmap_cast, image_cast, current_pose);

    cv::Mat img(vmap_cast);
    cv::imshow("img", img);
    cv::waitKey(1);

    // tracker->set_reference_vmap(vmap_cast);

    tracker->set_reference_depth(cv::cuda::GpuMat(last_tracked->depth));
    tracker->set_reference_image(cv::cuda::GpuMat(last_tracked->image));
    tracker->set_source_depth(cv::cuda::GpuMat(current->depth));
    tracker->set_source_image(cv::cuda::GpuMat(current->image));

    TrackingContext context;
    context.max_iterations_ = {10, 5, 3, 3, 3};
    context.use_initial_guess_ = true;
    context.initial_estimate_ = Sophus::SE3d();
    auto result = tracker->compute_transform(context);

    if (result.sucess)
    {
        current_pose = current_pose * result.update;
        mapper->update(current->depth, current->image, current_pose);

        last_tracked = current;

        current_frame_id += 1;
    }
}

void SystemNew::populate_current_data(cv::Mat depth, cv::Mat image)
{
    cv::Mat depth_meters;
    depth.convertTo(depth_meters, CV_32FC1, 1 / 1000.f);

    current = std::make_shared<RgbdFrame>(depth_meters, image, current_frame_id);
}

void SystemNew::reset()
{
}

void SystemNew::write_map_to_disk(const std::string) const
{
}

void SystemNew::read_map_from_disk(const std::string)
{
}

void SystemNew::create_keyframe()
{
}

bool SystemNew::check_keyframe_critera() const
{
}

Eigen::Matrix3f SystemNew::get_intrinsics() const
{
    return K;
}

Eigen::Matrix4f SystemNew::get_current_pose() const
{
    return current_pose.cast<float>().matrix();
}

} // namespace fusion