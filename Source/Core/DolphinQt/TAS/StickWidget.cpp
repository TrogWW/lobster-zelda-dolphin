// Copyright 2018 Dolphin Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "DolphinQt/TAS/StickWidget.h"

#include <algorithm>
#include <cmath>

#include <QMouseEvent>
#include <QPainter>

#include "Common/CommonTypes.h"

constexpr int PADDING = 1;

StickWidget::StickWidget(QWidget* parent, u16 max_x, u16 max_y)
    : QWidget(parent), m_max_x(max_x), m_max_y(max_y)
{
  setMouseTracking(true);
  setToolTip(tr("Left click to set the stick value.\n"
                "Right click to re-center it."));
  setFocusPolicy(Qt::StrongFocus);
  // If the widget gets too small, it will get deformed.
  setMinimumSize(QSize(64, 64));
}

void StickWidget::SetX(u16 x)
{
  m_x = std::min(m_max_x, x);

  update();
}

void StickWidget::SetY(u16 y)
{
  m_y = std::min(m_max_y, y);

  update();
}

void StickWidget::paintEvent(QPaintEvent* event)
{
  QPainter painter(this);

  painter.setRenderHint(QPainter::Antialiasing, true);
  painter.setRenderHint(QPainter::SmoothPixmapTransform, true);

  const int diameter = std::min(width(), height()) - PADDING * 2;

  // inscribe the StickWidget inside a square
  painter.fillRect(PADDING, PADDING, diameter, diameter, Qt::lightGray);

  painter.setBrush(Qt::white);
  painter.drawEllipse(PADDING, PADDING, diameter, diameter);

  painter.drawLine(PADDING, PADDING + diameter / 2, PADDING + diameter, PADDING + diameter / 2);
  painter.drawLine(PADDING + diameter / 2, PADDING, PADDING + diameter / 2, PADDING + diameter);

  // convert from value space to widget space
  u16 x = PADDING + ((m_x * diameter) / m_max_x);
  u16 y = PADDING + (diameter - (m_y * diameter) / m_max_y);

  painter.drawLine(PADDING + diameter / 2, PADDING + diameter / 2, x, y);

  painter.setBrush(Qt::blue);
  int neutral_radius = diameter / 30;
  painter.drawEllipse(x - neutral_radius, y - neutral_radius, neutral_radius * 2,
                      neutral_radius * 2);
}

void StickWidget::mousePressEvent(QMouseEvent* event)
{
  handleMouseEvent(event);
  m_ignore_movement = event->button() == Qt::RightButton;
}

void StickWidget::mouseReleaseEvent(QMouseEvent* event)
{
  handleMouseEvent(event);
  m_ignore_movement = event->button() == Qt::LeftButton || event->button() == Qt::RightButton;
}

void StickWidget::mouseMoveEvent(QMouseEvent* event)
{
  if (!m_ignore_movement)
    handleMouseEvent(event);
}
void StickWidget::keyReleaseEvent(QKeyEvent* event)
{
  //ESS Positions -- TODO (set as configurable hotkeys?)
  bool changed = false;
  if (event->modifiers() == Qt::NoModifier)
  {
    switch (event->key())
    {
      case Qt::Key_J:
        m_x = 110;
        m_y = 128;
        changed = true;
        break;
      case Qt::Key_I:
        m_x = 128;
        m_y = 146;
        changed = true;
        break;
      case Qt::Key_L:
        m_x = 146;
        m_y = 128;
        changed = true;
        break;
      case Qt::Key_Comma:
        m_x = 128;
        m_y = 110;
        changed = true;
        break;
      case Qt::Key_Period:
        m_x = 145;
        m_y = 111;
        changed = true;
        break;
      case Qt::Key_M:
        m_x = 111;
        m_y = 111;
        changed = true;
        break;
      case Qt::Key_U:
        m_x = 111;
        m_y = 145;
        changed = true;
        break;
      case Qt::Key_O:
        m_x = 145;
        m_y = 145;
        changed = true;
        break;
      case Qt::Key_K:
        m_x = 128;
        m_y = 128;
        changed = true;
        break;
      default:
        break;
    }
  }
  emit ChangedX(m_x);
  emit ChangedY(m_y);
  if (changed)
  {
    update();
  }

}
void StickWidget::handleMouseEvent(QMouseEvent* event)
{
  u16 prev_x = m_x;
  u16 prev_y = m_y;

  if (event->button() == Qt::RightButton)
  {
    m_x = std::round(m_max_x / 2.);
    m_y = std::round(m_max_y / 2.);
  }
  else
  {
    // convert from widget space to value space
    int new_x = (event->pos().x() * m_max_x) / width();
    int new_y = m_max_y - (event->pos().y() * m_max_y) / height();

    m_x = std::max(0, std::min(static_cast<int>(m_max_x), new_x));
    m_y = std::max(0, std::min(static_cast<int>(m_max_y), new_y));
  }

  bool changed = false;
  if (prev_x != m_x)
  {
    emit ChangedX(m_x);
    changed = true;
  }
  if (prev_y != m_y)
  {
    emit ChangedY(m_y);
    changed = true;
  }

  if (changed)
    update();
}


