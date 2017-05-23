#ifndef __RENDER_WINDOW_H__
#define __RENDER_WINDOW_H__

#include "types.h"

#include <memory>
class Input;
class Window {
 public:
  Window();
  ~Window();
  static bool init(uint32 width, uint32 height);

  static uint32 width();
  static uint32 height();

  static void makeFullScreen();
  static void makeWindowed();

  static bool processEvents();
  static void swap();
  static void destroy();

  static void drawSimpleTriangle();

  friend class Input;
  struct Data;
 private:
  static std::unique_ptr<Data> data_;
};

class Input {
 public:
  Input();
  ~Input();

  static bool isKeyDown(Key key);
  static bool isKeyUp(Key key);
};

#endif // __RENDER_WINDOW_H__