#pragma once

class TextureToRender {
public:
  TextureToRender();
  ~TextureToRender();
  TextureToRender(TextureToRender &&);
  TextureToRender &operator=(TextureToRender &&);
  void create(int width, int height);
  void bind();
  void unbind();
  const unsigned int *getTexture() const { return &tex_; }

private:
  int w_, h_;
  unsigned int fb_ = -1;
  unsigned int tex_ = -1;
  unsigned int dep_ = -1;
};
