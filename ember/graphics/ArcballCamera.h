#pragma once

#include <Eigen/Dense>

namespace ember::graphics {

    class ArcballCamera {
    public:
        ArcballCamera(
            const Eigen::Vector3f& position,
            const Eigen::Vector3f& focus = Eigen::Vector3f::Zero()
        );
        Eigen::Affine3f view_matrix();
        void add_rotation(const Eigen::AngleAxisf& rotation);
        void add_distance(float distance);
        void set_min_distance(float distance);
        void set_max_distance(float distance);
    private:
        Eigen::Translation3f m_position;
        Eigen::Quaternionf m_rotation;
        Eigen::Affine3f m_view_matrix;
        float m_min_distance, m_max_distance;
        bool m_matrix_out_of_date;

        void compute_view_matrix();
    };

}