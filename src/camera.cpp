#include "camera.hpp"

#include <grf/imgui.h>

const Camera::Matrices& Camera::matrices() const {
  return m_matrices;
}

bool Camera::needsUpdate() const {
  return m_dirty;
}

f32 Camera::sensitivity() const {
  return m_sensitivity;
}

f32 Camera::speed() const {
  return m_speed;
}

void Camera::settingsGUI() {
  ImGui::Begin("Camera Settings");
    ImGui::InputFloat("sensitivity", &m_sensitivity, 0.05f, 0.25f, "%.2f");
    ImGui::InputFloat("speed", &m_speed, 1.0f, 5.0f, "%.0f");
    m_dirty |= ImGui::SliderFloat("fov", &m_fov, 30.0f, 120.0f, "%.0f");
    m_dirty |= ImGui::InputFloat("near", &m_near, 0.001f, 0.1f, "%0.3f");
    m_dirty |= ImGui::InputFloat("far", &m_far, 1.0f, 1000.0f, "%.0f");
  ImGui::End();
}

void Camera::update() {
  if (!m_dirty) return;

  m_right   = m_rotation * vec3(1.0, 0.0, 0.0);
  m_up      = m_rotation * vec3(0.0, 1.0, 0.0);
  m_forward = m_rotation * vec3(0.0, 0.0, -1.0);

  m_matrices.proj     = glm::perspective(glm::radians(m_fov), m_aspect, m_near, m_far);
  m_matrices.view     = glm::lookAt(m_position, m_position + m_forward, m_up);
  m_matrices.position = vec4(m_position, 0.0);

  m_dirty = false;
}

void Camera::translate(vec3 delta) {
  m_position += m_rotation * delta;
  m_dirty = true;
}

void Camera::rotate(f32 yaw, f32 pitch) {
  const f32 limit = glm::radians(89.0f);
  f32 clamped = glm::clamp(m_pitch + pitch, -limit, limit);
  pitch = clamped - m_pitch;
  m_pitch = clamped;

  m_rotation = glm::normalize(
    glm::angleAxis(yaw, vec3(0.0, 1.0, 0.0)) *
    m_rotation *
    glm::angleAxis(pitch, vec3(1.0, 0.0, 0.0))
  );
  m_dirty = true;
}

void Camera::setAspect(f32 ar) {
  m_aspect = ar;
  m_dirty = true;
}