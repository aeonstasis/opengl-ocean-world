#ifndef JPEGIO_H
#define JPEGIO_H

#include "image.h"
#include <memory>
#include <string>

bool SaveJPEG(const std::string &filename, int image_width, int image_height,
              const unsigned char *pixels);
std::unique_ptr<Image> LoadJPEG(const std::string &file_name);

#endif
