#include "ArcballCamera.h"

namespace ember::graphics {

    ArcballCamera::ArcballCamera(
        const Eigen::Vector3f& position,
        const Eigen::Vector3f& focus
    ):
        m_position(-Eigen::Vector3f::UnitZ() * position.norm()),
        m_rotation(Eigen::Quaternionf::FromTwoVectors(Eigen::Vector3f::UnitZ(), position - focus)),
        m_min_distance(0.0),
        m_max_distance(-INFINITY),
        m_matrix_out_of_date(true)
    { }

    Eigen::Affine3f ArcballCamera::view_matrix() {
        if (m_matrix_out_of_date) {
            compute_view_matrix();
            m_matrix_out_of_date = false;
        }
        return m_view_matrix;
    }

    void ArcballCamera::add_rotation(const Eigen::AngleAxisf& rotation) {
        m_rotation = rotation * m_rotation;
        m_matrix_out_of_date = true;
    }

    void ArcballCamera::add_distance(float distance) {
        m_position.z() -= distance;
        m_position.z() = std::clamp(m_position.z(), m_max_distance, m_min_distance);
        m_matrix_out_of_date = true;
    }

    void ArcballCamera::compute_view_matrix() {
        m_view_matrix = (m_rotation.toRotationMatrix() * m_position).inverse();
    }

    void ArcballCamera::set_min_distance(float distance) {
        m_min_distance = -distance;
        m_position.z() = std::clamp(m_position.z(), m_max_distance, m_min_distance);
        m_matrix_out_of_date = true;
    }

    void ArcballCamera::set_max_distance(float distance) {
        m_max_distance = -distance;
        m_position.z() = std::clamp(m_position.z(), m_max_distance, m_min_distance);
        m_matrix_out_of_date = true;
    }

}