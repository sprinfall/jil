#ifndef JIL_COMPILE_CONFIG_H_
#define JIL_COMPILE_CONFIG_H_
#pragma once

// Allow only one instance of Jil.
#define JIL_SINGLE_INSTANCE 1

// Allow multiple main windows (frames).
// Sublime Text supports multi-window.
// Multi-window is not very useful since we have tabs. It also makes
// the application complex.
#define JIL_MULTIPLE_WINDOW 0

#ifdef __WXMSW__
// Use GDI+ directly instead of wxGraphicsContext to render book ctrl
// tab foreground.
// wxGraphicsContext doesn't support gradiant text foreground.
#  define JIL_BOOK_USE_GDIPLUS 1
#else
#  define JIL_BOOK_USE_GDIPLUS 0
#endif  // __WXMSW__

// ISSUE: Native tooltip wraps the long text. But a tooltip displaying
// file path shouldn't be wrapped.
// TODO: Check non-windows platforms.
#define JIL_BOOK_NATIVE_TOOLTIP 0

#define JIL_BOOK_TAB_ROUND_CORNER 0

#endif  // JIL_COMPILE_CONFIG_H_
