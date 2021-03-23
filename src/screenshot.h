#ifndef SCREENSHOT_H
#define SCREENSHOT_H

#ifndef SCREENSHOT_API
#define SCREENSHOT_API extern
#endif

#ifdef __cplusplus
extern "C" {
#endif

typedef void *window_t;

SCREENSHOT_API void screenshot_run(int w, int h, const char *title, const char *path);

// Window size hints
#define WINDOW_HINT_NONE 0  // Width and height are default size
#define WINDOW_HINT_MIN 1   // Width and height are minimum bounds
#define WINDOW_HINT_MAX 2   // Width and height are maximum bounds
#define WINDOW_HINT_FIXED 3 // Window size can not be changed by a user

#ifdef __cplusplus
}
#endif

#ifndef SCREENSHOT_HEADER

#if !defined(SCREENSHOT_GTK) && !defined(SCREENSHOT_COCOA) && !defined(SCREENSHOT_EDGE)
#if defined(__linux__)
#define SCREENSHOT_GTK
#elif defined(__APPLE__)
#define SCREENSHOT_COCOA
#elif defined(_WIN32)
#define SCREENSHOT_EDGE
#else
#error "please, specify webview backend"
#endif
#endif

#include <atomic>
#include <functional>
#include <future>
#include <map>
#include <string>
#include <utility>
#include <vector>

#include <cstring>

#if defined(SCREENSHOT_COCOA)

#include <CoreGraphics/CoreGraphics.h>
#include <objc/objc-runtime.h>

#define NSBackingStoreBuffered 2

#define NSWindowStyleMaskResizable 8
#define NSWindowStyleMaskMiniaturizable 4
#define NSWindowStyleMaskTitled 1
#define NSWindowStyleMaskClosable 2

#define NSApplicationActivationPolicyRegular 0

#define WKUserScriptInjectionTimeAtDocumentStart 0

#define NSUTF8StringEncoding 4


// Helpers to avoid too much typing
id operator"" _cls(const char *s, std::size_t) { return (id)objc_getClass(s); }
SEL operator"" _sel(const char *s, std::size_t) { return sel_registerName(s); }
id operator"" _str(const char *s, std::size_t) {
  return ((id(*)(id, SEL, const char *))objc_msgSend)(
      "NSString"_cls, "stringWithUTF8String:"_sel, s);
}

class cocoa_window {
public:
  cocoa_window() {
    // Application
    id app = ((id(*)(id, SEL))objc_msgSend)("NSApplication"_cls,
                                            "sharedApplication"_sel);
    ((void (*)(id, SEL, long))objc_msgSend)(
        app, "setActivationPolicy:"_sel, NSApplicationActivationPolicyRegular);

    // Main window
    m_window = ((id(*)(id, SEL))objc_msgSend)("NSWindow"_cls, "alloc"_sel);
    m_window =
        ((id(*)(id, SEL, CGRect, int, unsigned long, int))objc_msgSend)(
            m_window, "initWithContentRect:styleMask:backing:defer:"_sel,
            CGRectMake(0, 0, 0, 0), 0, NSBackingStoreBuffered, 0);

    ((void (*)(id, SEL, id))objc_msgSend)(m_window, "makeKeyAndOrderFront:"_sel,
                                          nullptr);
  }
  ~cocoa_window() { close(); }
  void terminate() {
    close();
    ((void (*)(id, SEL, id))objc_msgSend)("NSApp"_cls, "terminate:"_sel,
                                          nullptr);
  }
  void run() {
    id app = ((id(*)(id, SEL))objc_msgSend)("NSApplication"_cls,
                                            "sharedApplication"_sel);
    ((void (*)(id, SEL))objc_msgSend)(app, "run"_sel);
  }
  void set_contentview(int width, int height) {
    // ContentView
    id c_view =
        ((id(*)(id, SEL, CGRect))objc_msgSend)(
            ((id(*)(id, SEL))objc_msgSend)("NSView"_cls, "alloc"_sel),
            "initWithFrame:"_sel,
            CGRectMake(0, 0, width, height));
    ((void (*)(id, SEL, CGSize))objc_msgSend)(m_window, "setContentSize:"_sel,
        CGSizeMake(width, height));
    // Add SubView
    id button =
        ((id(*)(id, SEL, CGRect))objc_msgSend)(
            ((id(*)(id, SEL))objc_msgSend)("NSButton"_cls, "alloc"_sel),
            "initWithFrame:"_sel,
            CGRectMake(50, 50, 100, 30));

    ((void (*)(id, SEL, id))objc_msgSend)(c_view, "addSubview:"_sel, button);
    ((void (*)(id, SEL, id))objc_msgSend)(m_window, "setContentView:"_sel, c_view);

  }
  void set_color() {
    ((void (*)(id, SEL, id))objc_msgSend)(
        m_window, "setBackgroundColor:"_sel,
        ((id(*)(id, SEL))objc_msgSend)(
            "NSColor"_cls, "redColor"_sel));
  }
  void set_title(const std::string title) {
    ((void (*)(id, SEL, id))objc_msgSend)(
        m_window, "setTitle:"_sel,
        ((id(*)(id, SEL, const char *))objc_msgSend)(
            "NSString"_cls, "stringWithUTF8String:"_sel, title.c_str()));
  }
  void set_size(int width, int height, int hints) {
    auto style = NSWindowStyleMaskTitled | NSWindowStyleMaskClosable |
                 NSWindowStyleMaskMiniaturizable;
    if (hints != WINDOW_HINT_FIXED) {
      style = style | NSWindowStyleMaskResizable;
    }
    ((void (*)(id, SEL, unsigned long))objc_msgSend)(
        m_window, "setStyleMask:"_sel, style);

    if (hints == WINDOW_HINT_MIN) {
      ((void (*)(id, SEL, CGSize))objc_msgSend)(
          m_window, "setContentMinSize:"_sel, CGSizeMake(width, height));
    } else if (hints == WINDOW_HINT_MAX) {
      ((void (*)(id, SEL, CGSize))objc_msgSend)(
          m_window, "setContentMaxSize:"_sel, CGSizeMake(width, height));
    } else {
      ((void (*)(id, SEL, CGRect, BOOL, BOOL))objc_msgSend)(
          m_window, "setFrame:display:animate:"_sel,
          CGRectMake(0, 0, width, height), 1, 0);
    }
    ((void (*)(id, SEL))objc_msgSend)(m_window, "center"_sel);
  }
  void save_screenshot(const std::string path) {
    id content_view = ((id(*)(id, SEL))objc_msgSend)(m_window, "contentView"_sel);
    CGRect content_view_frame = ((CGRect(*)(id, SEL))objc_msgSend_stret)(content_view, "frame"_sel);
    id img_data = ((id(*)(id, SEL, CGRect))objc_msgSend)(content_view, "dataWithPDFInsideRect:"_sel,
        CGRectMake(0, 0, content_view_frame.size.width, content_view_frame.size.height));
    id img = ((id(*)(id, SEL, id))objc_msgSend)(
        ((id(*)(id, SEL))objc_msgSend)("NSImage"_cls, "alloc"_sel),
        "initWithData:"_sel, img_data);
    id data = ((id(*)(id, SEL))objc_msgSend)(img, "TIFFRepresentation"_sel);
    id result_file = ((id(*)(id, SEL, const char *))objc_msgSend)(
        "NSString"_cls, "stringWithUTF8String:"_sel, path.c_str());
    ((void (*)(id, SEL, id, BOOL))objc_msgSend)(
        data, "writeToFile:atomically:"_sel, result_file, 1);
  }
  void log(const std::string path) {
    id log_file = ((id(*)(id, SEL, const char *))objc_msgSend)(
        "NSString"_cls, "stringWithUTF8String:"_sel, path.c_str());
    id log_data = ((id(*)(id, SEL, unsigned long))objc_msgSend)(
        "Log Write Test"_str, "dataUsingEncoding:"_sel, NSUTF8StringEncoding);
    ((void (*)(id, SEL, id, BOOL))objc_msgSend)(
        log_data, "writeToFile:atomically:"_sel, log_file, 1);
  }

private:
  void close() { ((void (*)(id, SEL))objc_msgSend)(m_window, "close"_sel); }
  id m_window;
};
#elif defined(SCREENSHOT_EDGE) /* SCREENSHOT_COCOA END */
// TODO
#elif defined(SCREENSHOT_GTK) /* SCREENSHOT_EDGE END */
// TODO
#endif /* SCREENSHOT_GTK END */

class window: public cocoa_window {
public:
  window()
      : cocoa_window() {}

}; // class window

SCREENSHOT_API void screenshot_run(int w, int h, const char *title, const char *path) {
  window_t ww = new window();
  // static_cast<window *>(ww)->log(path);
  static_cast<window *>(ww)->set_title(title);
  static_cast<window *>(ww)->set_size(w, h, WINDOW_HINT_NONE);
  static_cast<window *>(ww)->set_contentview(w, h);
  static_cast<window *>(ww)->save_screenshot(path);
  static_cast<window *>(ww)->run();
}

#endif /* SCREENSHOT_API */
#endif /* SCREENSHOT_H */
