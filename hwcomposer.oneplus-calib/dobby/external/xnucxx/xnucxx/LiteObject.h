#ifndef LITE_OBJECT_H
#define LITE_OBJECT_H

#include "common_header.h"

class LiteObject {
public:
  virtual ~LiteObject() = default;

  virtual void free();

  virtual void release();
};

#endif
