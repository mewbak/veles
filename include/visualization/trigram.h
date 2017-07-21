/*
 * Copyright 2016 CodiLime
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */
#pragma once

#include "visualization/trigram.h"

#include <stdint.h>

#include <vector>

#include <QOpenGLWidget>
#include <QOpenGLFunctions_3_2_Core>
#include <QOpenGLShaderProgram>
#include <QOpenGLTexture>
#include <QOpenGLBuffer>
#include <QOpenGLVertexArrayObject>
#include <QBasicTimer>
#include <QPushButton>
#include <QSlider>
#include <QCheckBox>
#include <QTime>
#include <QToolBar>
#include <QAction>

#include "util/settings/shortcuts.h"
#include "visualization/base.h"
#include "visualization/manipulator.h"

namespace veles {
namespace visualization {

extern const int k_minimum_brightness;
extern const int k_maximum_brightness;

/*****************************************************************************/
/* LabelPositionMixer */
/*****************************************************************************/

class LabelPositionMixer {
 public:
  LabelPositionMixer(){}
  LabelPositionMixer(
      QVector4D s0c0p0, QVector4D s1c0p0,
      QVector4D s0c1p0, QVector4D s0c1p1,
      QVector4D s0c0p1, QVector4D s1c0p1) {
    s0c0p0_ = s0c0p0;
    s1c0p0_ = s1c0p0;
    s0c1p0_ = s0c1p0;
    s0c1p1_ = s0c1p1;
    s0c0p1_ = s0c0p1;
    s1c0p1_ = s1c0p1;
  }

  QVector4D interpolate(QVector4D a, QVector4D b, float f) const {
    return (1.f - f) * a + f * b;
  }

  QVector4D mix(float sph, float cyl, float pos) const {
    QVector4D sp0 = interpolate(s0c0p0_, s1c0p0_, sph);
    QVector4D cp0 = interpolate(s0c0p0_, s0c1p0_, cyl);
    QVector4D sp1 = interpolate(s0c0p1_, s1c0p1_, sph);
    QVector4D cp1 = interpolate(s0c0p1_, s0c1p1_, cyl);

    float f = 0.f;
    if (sph + cyl != 0.f) {
      f = cyl / (sph + cyl);
    }

    QVector4D p0 = interpolate(sp0, cp0, f);
    QVector4D p1 = interpolate(sp1, cp1, f);
    return interpolate(p0, p1, pos);
  }

 private:
  QVector4D s0c0p0_;
  QVector4D s1c0p0_;
  QVector4D s0c1p0_;
  QVector4D s0c1p1_;
  QVector4D s0c0p1_;
  QVector4D s1c0p1_;
};

/*****************************************************************************/
/* TrigramWidget */
/*****************************************************************************/

class TrigramWidget : public VisualizationWidget {
  Q_OBJECT

 public:
  enum class EVisualizationShape {CUBE, CYLINDER, SPHERE};
  enum class EVisualizationMode {TRIGRAM, LAYERED_DIGRAM};

  explicit TrigramWidget(QWidget* parent = 0);
  ~TrigramWidget();

  virtual void prepareOptions(QMainWindow* visualization_window) override;
  void setMode(EVisualizationMode mode, bool animate = true);

  static float vfovDeg(float min_fov_deg, float aspect_ratio);

  struct BrightnessData : public AdditionalResampleData {
    int brightness;
  };

 public slots:
  void brightnessSliderMoved(int value);

 protected:
  void refresh(AdditionalResampleDataPtr ad) override;
  bool initializeVisualizationGL() override;

  bool event(QEvent* event) override;
  void timerEvent(QTimerEvent*) override;

  void resizeGLImpl(int w, int h) override;
  void paintGLImpl() override;

  AdditionalResampleData* onAsyncResample() override;

  void paintLabels(QMatrix4x4& scene_mp, QMatrix4x4& scene_m);
  void paintLabel(const LabelPositionMixer& mixer, const QMatrix4x4& scene_to_screen,
      const QMatrix4x4& screen_mp, QOpenGLTexture* texture);
  void initLabels();
  void releaseLabels();
  QVector3D calcScreenPosForLabel(QVector3D world_pos,
      const QMatrix4x4& scene_to_screen, int width, int height);
  void paintRF(QMatrix4x4& mvp);
  void initRF();
  void releaseRF();
  void initLabelPositionMixers();

  void initShaders();
  void initTextures();
  void initGeometry();

  QAction* createAction(util::settings::shortcuts::ShortcutType type, const QIcon& icon, Manipulator* manipulator);
  QAbstractButton* createActionButton(QAction* action);
  virtual void prepareManipulatorToolbar(QMainWindow* visualization_window);

 signals:
  void manipulatorChanged(Manipulator* manipulator);

 private slots:
  void playPause();
  void setShape(EVisualizationShape shape);
  void setUseBrightnessHeuristic(int state);
  void setManipulator(Manipulator* manipulator);

 private:
  int brightness_;  // has to be set through setBrightness()
  void setBrightness(int value);

  int suggestBrightness();  // heuristic
  void autoSetBrightness();

  QBasicTimer timer_;
  QOpenGLShaderProgram program_;
  QOpenGLTexture* texture_;
  QOpenGLBuffer* databuf_;

  QOpenGLVertexArrayObject vao_;
  float c_sph_, c_cyl_, c_pos_;
  int width_, height_;
  EVisualizationShape shape_;
  EVisualizationMode mode_;

  QAction* cube_action_;
  QAction* cylinder_action_;
  QAction* sphere_action_;
  QSlider* brightness_slider_;
  QCheckBox* use_heuristic_checkbox_;
  bool is_playing_, use_brightness_heuristic_;
  QCheckBox* show_labels_and_rf_checkbox_;
  bool show_labels_;

  QList<Manipulator*> manipulators_;
  Manipulator* current_manipulator_;
  SpinManipulator* spin_manipulator_;
  TrackballManipulator* trackball_manipulator_;
  FreeManipulator* free_manipulator_;
  QTime time_;

  QOpenGLShaderProgram label_program_;
  QOpenGLVertexArrayObject label_vao_;
  QOpenGLBuffer label_vb_;
  QOpenGLShaderProgram rf_program_;
  QOpenGLVertexArrayObject rf_vao_;
  QOpenGLBuffer rf_vb_;
  QOpenGLTexture* texture_0_;
  QOpenGLTexture* texture_1_;
  QOpenGLTexture* texture_2_;
  QOpenGLTexture* texture_3_;
  QOpenGLTexture* texture_pos_;
  QOpenGLTexture* texture_N0_;
  QOpenGLTexture* texture_0_digram_;
  QOpenGLTexture* texture_1_digram_;
  QOpenGLTexture* texture_2_digram_;
  QOpenGLTexture* texture_N0_digram_;

  LabelPositionMixer lpm_0_, lpm_1_, lpm_2_, lpm_3_, lpm_pos_, lpm_N0_,
      lpm_0_digram_, lpm_1_digram_, lpm_2_digram_, lpm_N0_digram_;
};

}  // namespace visualization
}  // namespace veles
