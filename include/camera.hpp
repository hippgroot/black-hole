#pragma once

#include <grf/math.hpp>

using namespace grf;

class Camera {
public:
  struct Matrices {
    mat4 view;
    mat4 proj;
  };

private:
  Matrices  m_matrices;

  quat      m_rotation = quat(vec3(0.0));
  vec3      m_position = vec3(0.0, 0.0, 10.0);
  vec3      m_right = vec3(1.0, 0.0, 0.0);
  vec3      m_up = vec3(0.0, 1.0, 0.0);
  vec3      m_forward = vec3(0.0, 0.0, -1.0);
  f32       m_pitch = 0.0f;

  // cursorDelta is normalized (full-screen swipe = ±2), not pixels
  f32       m_sensitivity = 1.5f;
  f32       m_speed = 10.0f;
  f32       m_aspect = 0.0f;
  f32       m_fov = 60.0f;
  f32       m_near = 0.01f;
  f32       m_far = 10000.0f;

  bool      m_dirty = true;

public:
  const Matrices& matrices() const;
  bool needsUpdate() const;
  f32 sensitivity() const;
  f32 speed() const;

  void settingsGUI();

  void update();
  void translate(vec3 delta);
  void rotate(f32 yaw, f32 pitch);
  void setAspect(f32);
};