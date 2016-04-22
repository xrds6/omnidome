/* Copyright (c) 2014-2015 "Omnidome" by cr8tr
 * Dome Mapping Projection Software (http://omnido.me).
 * Omnidome was created by Michael Winkelmann aka Wilston Oreo (@WilstonOreo)
 *
 * This file is part of Omnidome.
 *
 * Omnidome is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as
 * published by the Free Software Foundation, either version 3 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 * You should have received a copy of the GNU Affero General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */


#include "AboutGL.h"

#include <QDesktopServices>
#include <QUrl>
#include <QFile>
#include <QImage>
#include <QPainter>
#include <QTimer>
#include <chrono>
#include <type_traits>
#include <omni/util.h>
#include <omni/visual/util.h>
#include <omni/visual/Rectangle.h>


using namespace omni::ui;

AboutGL::AboutGL(QWidget *_parent) :
  QOpenGLWidget(_parent)
{
  QTimer *timer = new QTimer(this);

  connect(timer, SIGNAL(timeout()), this, SLOT(update()));
  timer->start(20);

  /// "Random" start time
  startTime_ = visual::util::now() +
    double(reinterpret_cast<long long>(timer) & ((1 << 16) - 1));
}

AboutGL::~AboutGL()
{

}

void AboutGL::initializeGL()
{
  glEnable(GL_DEPTH_TEST);
  glEnable(GL_POINT_SMOOTH);
  glEnable(GL_LINE_SMOOTH);

  glDepthFunc(GL_LEQUAL);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);
  glHint(GL_POINT_SMOOTH_HINT, GL_NICEST);
  glHint(GL_POLYGON_SMOOTH_HINT, GL_NICEST);

  /// Initialize Shader
  static QString _vertSrc     = util::fileToStr(":/shaders/frustum.vert");
  static QString _fragmentSrc = util::fileToStr(":/shaders/slow_fractal.frag");

  shader_ = new QOpenGLShaderProgram(this);
  shader_->addShaderFromSourceCode(QOpenGLShader::Vertex, _vertSrc);
  shader_->addShaderFromSourceCode(QOpenGLShader::Fragment, _fragmentSrc);
  shader_->link();

  QImage _image(1024, 1024, QImage::Format_ARGB32);

  QPainter _p(&_image);
  {
    _p.setBrush(QBrush("#000000"));
    _p.setPen(QPen("#FFFFFF"));
    QFont _font;
    _font.setPixelSize(44);
    _p.setFont(_font);
    _p.drawText(8,60,"Omnidome is a product by CR8TR.");
    _p.drawText(8,124,"Code written by Michael Winkelmann.");
    _p.drawText(8,188,"GUI design by Brook Cronin + Michael Winkelmann.");
    _p.drawText(8,252,QString("Version ") + OMNIDOME_VERSION_STRING);
    _p.drawText(8,316,"Copyright (C) 2016");
  }
  _p.end();
  tex_.reset(new QOpenGLTexture(_image.mirrored()));
}

void AboutGL::resizeGL(int _w, int _h)
{
  _w = _w & ~1;
  _h = _h & ~1;
  glViewport(0, 0, (GLint)_w, (GLint)_h);
  glClearColor(0.0, 0.0, 0.0, 1.0);
}

void AboutGL::mousePressEvent(QMouseEvent *)
{
  QDesktopServices::openUrl(QUrl("http://omnido.me", QUrl::TolerantMode));
}

void AboutGL::paintGL()
{
  if (!shader_) return;

  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  glDisable(GL_CULL_FACE);

  /// Setup orthogonal projection
  glMatrixMode(GL_PROJECTION);
  {
    glLoadIdentity();
    QMatrix4x4 _m;
    _m.ortho(-0.5, 0.5, -0.5, 0.5, -1.0, 1.0);
    glMultMatrixf(_m.constData());
  }

  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();

  visual::viewport(this);

  double _time =
                 std::chrono::high_resolution_clock::now().time_since_epoch().
                 count() / 1000000000.0 - startTime_;

  tex_->bind();
  shader_->bind();
  {
    shader_->setUniformValue("time", GLfloat(_time));
    shader_->setUniformValue("resolution", GLfloat(width()), GLfloat(height()));

    glActiveTexture(GL_TEXTURE0);
    shader_->setUniformValue("tex",0);

    glBegin(GL_QUADS);
    {
      glTexCoord2f(0.0f, 0.0f);
      glVertex2f(-0.5f, -0.5f);
      glTexCoord2f(1.0f, 0.0f);
      glVertex2f(0.5f, -0.5f);
      glTexCoord2f(1.0f, 1.0f);
      glVertex2f(0.5f, 0.5f);
      glTexCoord2f(0.0f, 1.0f);
      glVertex2f(-0.5f, 0.5f);
    }
    glEnd();
  }
  shader_->release();
  tex_->release();
}
