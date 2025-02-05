

#include "vp_embeddings_properties_socket_broker_node.h"


namespace vp_nodes {
        
    vp_embeddings_properties_socket_broker_node::vp_embeddings_properties_socket_broker_node(std::string node_name,
                                                        std::string des_ip,
                                                        int des_port,
                                                        std::string cropped_dir,
                                                        int min_crop_width,
                                                        int min_crop_height,
                                                        vp_broke_for broke_for, 
                                                        int broking_cache_warn_threshold, 
                                                        int broking_cache_ignore_threshold):
                                                        vp_msg_broker_node(node_name, broke_for, broking_cache_warn_threshold, broking_cache_ignore_threshold),
                                                        des_ip(des_ip),
                                                        des_port(des_port),
                                                        cropped_dir(cropped_dir),
                                                        min_crop_width(min_crop_width),
                                                        min_crop_height(min_crop_height) {
        // only for vp_frame_target                                                    
        assert(broke_for == vp_broke_for::NORMAL);
        udp_writer = kissnet::udp_socket(kissnet::endpoint(des_ip, des_port));
        VP_INFO(vp_utils::string_format("[%s] [message broker] set des_ip as `%s` and des_port as [%d]", node_name.c_str(), des_ip.c_str(), des_port));
        this->initialized();
    }
    
    vp_embeddings_properties_socket_broker_node::~vp_embeddings_properties_socket_broker_node() {

    }

    void vp_embeddings_properties_socket_broker_node::format_msg(const std::shared_ptr<vp_objects::vp_frame_meta>& meta, std::string& msg) {
        /* format:
        line 0, <--
        line 1, 1st cropped image's path
        line 2, 1st properties, multi property labels splited by ','
        line 3, 1st sub target info, empty line if sub target detected (sub target may be vehicle plate)
        line 4, 1st embeddings
        line 5, -->
        line 6, <--
        line 7, 2nd cropped image's path
        line 8, 2nd properties, multi property labels splited by ','
        line 9, 2nd sub target info, empty line if sub target detected (sub target may be vehicle plate)
        line 10, 2nd embeddings
        line 11, -->
        line 12, ...
        */

        std::stringstream msg_stream;
        auto format_embeddings = [&](const std::vector<float>& embeddings) {
            for (int i = 0; i < embeddings.size(); i++) {
                msg_stream << embeddings[i];
                if (i != embeddings.size() - 1) {
                    msg_stream << ",";
                }
            }
            msg_stream << std::endl;
        };
        auto format_sub_target = [&](const std::string& sub_target_info) {
            msg_stream << sub_target_info << std::endl;
        };
        auto format_properties = [&](const std::vector<std::string>& secondary_labels) {
            for (int i = 0; i < secondary_labels.size(); i++) {
                msg_stream << secondary_labels[i];
                if (i != secondary_labels.size() - 1) {
                    msg_stream << ",";
                }
            }
            msg_stream << std::endl;
        };
        auto save_cropped_image = [&](cv::Mat& frame, cv::Rect rect, std::string name) {
            auto cropped = frame(rect);
            cv::imwrite(name, cropped);
            msg_stream << name << std::endl;
        };
        if (broke_for == vp_broke_for::NORMAL) {
            for (int i = 0; i < meta->targets.size(); i++) {
                auto& t = meta->targets[i];
                // size filter
                if (t->width < min_crop_width || t->height < min_crop_height) {
                    continue;
                }
                
                auto name = cropped_dir + "/" + std::to_string(t->channel_index) + "_" + std::to_string(t->frame_index) + "_" + std::to_string(i) + ".jpg";
                // start flag
                msg_stream << "<--" << std::endl;
                // save small cropped image
                save_cropped_image(meta->frame, cv::Rect(t->x, t->y, t->width, t->height), name);
                // format properties
                format_properties(t->secondary_labels);
                // format sub target (vehicle plate here), just using the first sub target's label is enough.
                format_sub_target(t->sub_targets.size() > 0 ? t->sub_targets[0]->label : "");
                // format embeddings
                format_embeddings(t->embeddings);
                // end flag
                msg_stream << "-->";
                
                if (i != meta->targets.size() - 1) {
                    msg_stream << std::endl;  // not the last one
                }
            }
        }
        
        msg = msg_stream.str();   
    }

    void vp_embeddings_properties_socket_broker_node::broke_msg(const std::string& msg) {
        // broke msg to socket by udp
        auto bytes_2_send = reinterpret_cast<const std::byte*>(msg.c_str());
        auto bytes_2_send_len = msg.size();
        udp_writer.send(bytes_2_send, bytes_2_send_len);
    }
}