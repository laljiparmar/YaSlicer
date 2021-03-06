#pragma once

#include "GlContext.h"

#include <EGL/egl.h>

class GlContextANGLE : public IGlContext
{
public:
	GlContextANGLE(uint32_t width, uint32_t height, uint32_t samples);
	~GlContextANGLE();
private:

	uint32_t GetSurfaceWidth() const override;
	uint32_t GetSurfaceHeight() const override;

	void SwapBuffers() override;
	std::vector<uint8_t> GetRaster() override;
	std::vector<uint8_t> GetRasterGLES();
	void SetRaster(const std::vector<uint8_t>& raster, uint32_t width, uint32_t height) override;

	void CreateTextureFBO(GLFramebuffer& fbo, GLTexture& texture) override;
	void Resolve(const GLFramebuffer& fboTo) override;
	void ResetFBO() override;

	void Blit(const GLFramebuffer& fboFrom, const GLFramebuffer& fboTo);
	void Blit(GLuint fboFrom, GLuint fboTo);

	void CreateMultisampledFBO(uint32_t width, uint32_t height, uint32_t samples);
	void CreateTextureFBO(uint32_t width, uint32_t height, GLFramebuffer& fbo, GLTexture& texture);
	

	struct GLData
	{
		GLData() : display(EGL_NO_DISPLAY), context(EGL_NO_CONTEXT), surface(EGL_NO_SURFACE) {}
		~GLData();

		EGLDisplay display;
		EGLContext context;
		EGLSurface surface;

		GLRenderbuffer renderBuffer;
		GLRenderbuffer renderBufferDepth;
		GLFramebuffer fbo;

		GLTexture renderTexture;
		GLFramebuffer textureFBO;
	};

	GLData gl_;
	uint32_t width_;
	uint32_t height_;

	std::unique_ptr<RasterSetter> rasterSetter_;
};