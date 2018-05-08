#include "texture_to_render.h"
#include <GL/glew.h>
#include <debuggl.h>
#include <iostream>

TextureToRender::TextureToRender() {}

TextureToRender::~TextureToRender() {
  if (fb_ == static_cast<unsigned int>(-1)) {
    return;
  }
  unbind();
  glDeleteFramebuffers(1, &fb_);
  glDeleteTextures(1, &tex_);
  glDeleteRenderbuffers(1, &dep_);
}

TextureToRender::TextureToRender(TextureToRender &&other) {
  *this = std::move(other);
}

TextureToRender &TextureToRender::operator=(TextureToRender &&other) {
  if (this != &other) {
    // Free existing resources
    if (fb_ != static_cast<unsigned int>(-1)) {
      unbind();
      glDeleteFramebuffers(1, &fb_);
      glDeleteTextures(1, &tex_);
      glDeleteRenderbuffers(1, &dep_);
    }

    // Steal other's resources
    w_ = other.w_;
    h_ = other.h_;
    fb_ = other.fb_;
    tex_ = other.tex_;
    dep_ = other.dep_;

    // Ensure other's destructor doesn't free anything
    other.fb_ = -1;
  }
  return *this;
}

void TextureToRender::create(int width, int height) {
  w_ = width;
  h_ = height;
  // Create the framebuffer object backed by a texture
  glGenFramebuffers(1, &fb_);
  glGenTextures(1, &tex_);
  glGenRenderbuffers(1, &dep_);
  bind();
  // Give empty image to OpenGL
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA,
               GL_UNSIGNED_BYTE, 0);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

  // Configure framebuffer to use texture
  glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, tex_, 0);
  GLenum draw_buffers[1] = {GL_COLOR_ATTACHMENT0};
  glDrawBuffers(1, draw_buffers);

  // Attach renderbuffer
  glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, width, height);
  glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
                            GL_RENDERBUFFER, dep_);

  // Check framebuffer creation success
  if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
    std::cerr << "Failed to create framebuffer object as render target"
              << std::endl;
  }
  unbind();
}

void TextureToRender::bind() {
  // Bind the framebuffer object to GL_FRAMEBUFFER
  glBindFramebuffer(GL_FRAMEBUFFER, fb_);
  glBindTexture(GL_TEXTURE_2D, tex_);
  glBindRenderbuffer(GL_RENDERBUFFER, dep_);
}

void TextureToRender::unbind() {
  // Unbind current framebuffer object from the render target
  glBindFramebuffer(GL_FRAMEBUFFER, 0);
  glBindTexture(GL_TEXTURE_2D, 0);
  glBindRenderbuffer(GL_RENDERBUFFER, 0);
}
