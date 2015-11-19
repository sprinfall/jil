#ifndef INDENT_TEST_BASE_H_
#define INDENT_TEST_BASE_H_

#include "gtest/gtest.h"

#include "wx/string.h"

// Include here for convenience.

extern "C" {
#include "lualib.h"
}
#include "editor/lua_proxy.h"

#include "editor/ft_plugin.h"
#include "editor/text_buffer.h"
#include "editor/text_line.h"
#include "editor/util.h"

class IndentTestBase : public testing::Test {
protected:
  static void LoadLua();
  static void SetTextOptions(bool expand_tab, int tab_stop);
  static void LoadIndentFunc(const wxString& ft_id);

  // Call this in SetUpTestCase() of sub-class.
  static void StaticSetUp(const jil::editor::FileType& ft);

  // Call this in TearDownTestCase() of sub-class.
  static void StaticTearDown();

  virtual void SetUp();
  virtual void TearDown();

  int GetExpectedIndent(int ln);

  static lua_State* lua_state;
  static jil::editor::FtPlugin* ft_plugin;

  jil::editor::TextBuffer* buffer_;
};

#define ASSERT_LINE(ln)\
  EXPECT_EQ(buffer_->GetIndent(ln), GetExpectedIndent(ln));

#endif  // INDENT_TEST_BASE_H_
