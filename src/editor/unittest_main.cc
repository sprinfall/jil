//#include "wx/app.h"
#include <iostream>
#include "gtest/gtest.h"

// Because editor library is based on wxWidgets, use wxAppConsole instead of
// the normal main() function.

int main(int argc, char* argv[]) {
  std::cout << "Running unit testing for editor library.\n";

  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
/*
class EditorUtApp : public wxAppConsole {
public:
  virtual int OnRun() {
    std::cout << "Running unit testing for editor library.\n";

    testing::InitGoogleTest(&argc, argv.operator char**());
    return RUN_ALL_TESTS();
  }
};

IMPLEMENT_APP_CONSOLE(EditorUtApp)
*/