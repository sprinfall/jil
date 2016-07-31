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

// Use bitmap toggle buttons for options in find panel.
// If disabled, a text label "Options" with floating menu will be used.
#define JIL_BMP_BUTTON_FIND_OPTIONS 1

#endif  // JIL_COMPILE_CONFIG_H_
