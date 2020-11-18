#pragma once
#include <EGL/egl.h>

#include "AndroidApplication.hpp"

namespace My {
class OpenGLESApplication : public AndroidApplication {
   public:
    using AndroidApplication::AndroidApplication;
    virtual int Initialize();
    virtual void Finalize();
    virtual void Tick();

   protected:
    EGLSurface m_Surface;
    EGLContext m_Context;
    EGLDisplay m_Display;
    EGLint m_Width, m_Height;
};
}  // namespace My
