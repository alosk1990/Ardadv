// Copyright (C) 2012 Mark R. Stevens
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.

#ifndef ardadv_accviewer_CentralWidget_h
#define ardadv_accviewer_CentralWidget_h

#include <QtGui>
#include <QtOpenGL>

namespace ardadv
{
  namespace accviewer
  {

    //! @class CentralWidget
    //!
    //! @brief The opengl rendering surface to draw the frame
    //!
    class CentralWidget : public QGLWidget
    {
      Q_OBJECT

    public:

      //! @brief Constructor
      //!
      //! @param[in] parent the parent widget
      //!
      CentralWidget(QWidget *parent = 0);

      //! @brief Provide a size hint
      //!
      //! @return the size hint
      //!
      inline virtual QSize sizeHint() const
      {
        return QSize(640, 480);
      }

      //! @brief Provide a size hint
      //!
      //! @return the size hint
      //!
      inline virtual QSize minimumSizeHint() const
      {
        return QSize(640, 480);
      }

    protected:

      //! @brief Update the image texture
      //!
      void updateTexture();

      //! @brief Initialize opengl for rendering
      //!
      virtual void initializeGL();

      //! @brief Render the screen
      //!
      virtual void paintGL();

    private:

      //! @brief The current image being rendered
      //!
      QImage   qImage;

      //! @brief The image in gl format
      //!
      QImage   glImage;

      //! @brief The image texture id
      //!
      GLuint mTextureId;

    };
  }
}

#endif