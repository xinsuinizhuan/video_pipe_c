#pragma once

#include "../vp_secondary_infer_node.h"
#include "../../third_party/trt_vehicle/models/vehicle_color_classifier.h"

namespace vp_nodes {
    // vehicle color classifier based on tensorrt using trt_vehicle library
    // update secondary_class_ids/secondary_labels/secondary_scores of vp_frame_target.
    class vp_trt_vehicle_color_classifier: public vp_secondary_infer_node
    {
    private:
        /* data */
        std::shared_ptr<trt_vehicle::VehicleColorClassifier> vehicle_color_classifier = nullptr;
    protected:
        // we need a totally new logic for the whole infer combinations
        // no separate step pre-defined needed in base class
        virtual void run_infer_combinations(const std::vector<std::shared_ptr<vp_objects::vp_frame_meta>>& frame_meta_with_batch) override;
        // override pure virtual method, for compile pass
        virtual void postprocess(const std::vector<cv::Mat>& raw_outputs, const std::vector<std::shared_ptr<vp_objects::vp_frame_meta>>& frame_meta_with_batch) override;
    public:
        vp_trt_vehicle_color_classifier(std::string node_name, std::string vehicle_color_cls_model_path = "", std::vector<int> p_class_ids_applied_to = std::vector<int>());
        ~vp_trt_vehicle_color_classifier();
    };
}