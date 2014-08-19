#include "app/theme_config.h"
#include <string>
#include <vector>
#include "boost/algorithm/string.hpp"
#include "wx/log.h"
#include "wx/filename.h"
#include "editor/lex.h"
#include "editor/style.h"
#include "editor/text_window.h"
#include "editor/status_bar.h"
#include "editor/color.h"
#include "app/defs.h"
#include "app/config.h"
#include "app/book_ctrl.h"
#include "app/book_frame.h"

namespace jil {

using namespace editor;

namespace {

// Read style value from the setting.
void ReadStyle(Setting setting, StyleValue* style_value) {
  assert(setting);

  Setting fg_setting = setting.Get("fg");
  if (fg_setting) {
    style_value->set_fg(fg_setting.GetColor());
  }

  Setting bg_setting = setting.Get("bg");
  if (bg_setting) {
    style_value->set_bg(bg_setting.GetColor());
  }

  int font = 0;
  std::string font_str = setting.GetString("font");
  if (!font_str.empty()) {
    std::vector<std::string> splitted_font_str;
    boost::split(splitted_font_str,
                 font_str,
                 boost::is_any_of(", "),
                 boost::token_compress_on);

    for (size_t i = 0; i < splitted_font_str.size(); ++i) {
      std::string& f = splitted_font_str[i];
      if (f == "italic") {
        font |= Style::kItalic;
      } else if (f == "bold") {
        font |= Style::kBold;
      } else if (f == "underline") {
        font |= Style::kUnderline;
      } else {
        wxLogError(wxT("Unknown font style: %s!"), f.c_str());
      }
    }

    if (font != 0) {
      style_value->set_font(font);
    }
  }
}

// Read style value from the child setting with the given name.
void ReadStyle(Setting parent, const char* name, StyleValue* style_value) {
  assert(parent);

  style_value->Set();  // Clear

  Setting setting = parent.Get(name, Setting::kGroup);
  if (setting) {
    ReadStyle(setting, style_value);
  }
}

}  // namespace

// TODO: Error handling
bool LoadThemeFile(const wxString& theme_file,
                   SharedTheme& theme,
                   Style* style) {
  if (!wxFileName::FileExists(theme_file)) {
    wxLogError(wxT("Theme file doesn't exist: %s!"), theme_file.c_str());
    return false;
  }

  Config cfg;
  if (!cfg.Load(theme_file)) {
    wxLogError(wxT("Failed to parse theme file: %s!"), theme_file.c_str());
    return false;
  }

  Setting root = cfg.Root();

  //----------------------------------------------------------------------------
  // Window items.

  // Book frame.
  SharedTheme bf_theme(new Theme(BookFrame::COLOR_COUNT, 0));
  Setting bf_setting = root.Get("book_frame", Setting::kGroup);
  if (bf_setting) {
    bf_theme->SetColor(BookFrame::BG, bf_setting.GetColor("bg"));
  }
  theme->SetTheme(THEME_BOOK_FRAME, bf_theme);

  // Text book.
  SharedTheme tb_theme(new Theme(BookCtrl::COLOR_COUNT, BookCtrl::FONT_COUNT));
  Setting tb_setting = root.Get("text_book", Setting::kGroup);
  if (tb_setting) {
    tb_theme->SetColor(BookCtrl::BG,
                       tb_setting.GetColor("bg"));

    tb_theme->SetColor(BookCtrl::TAB_AREA_BG,
                       tb_setting.GetColor("tab_area_bg"));

    tb_theme->SetColor(BookCtrl::TAB_FG,
                       tb_setting.GetColor("tab_fg"));

    tb_theme->SetColor(BookCtrl::ACTIVE_TAB_FG,
                       tb_setting.GetColor("active_tab_fg"));

    tb_theme->SetColor(BookCtrl::TAB_BG,
                       tb_setting.GetColor("tab_bg"));

    tb_theme->SetColor(BookCtrl::ACTIVE_TAB_BG,
                       tb_setting.GetColor("active_tab_bg"));

    tb_theme->SetColor(BookCtrl::TAB_BORDER,
                       tb_setting.GetColor("tab_border"));

    tb_theme->SetColor(BookCtrl::ACTIVE_TAB_BORDER,
                       tb_setting.GetColor("active_tab_border"));

    wxFont tab_font = tb_setting.GetFont("tab_font");
    if (!tab_font.IsOk()) {
      tab_font = wxSystemSettings::GetFont(wxSYS_DEFAULT_GUI_FONT);
    }
    tb_theme->SetFont(BookCtrl::TAB_FONT, tab_font);
  }
  theme->SetTheme(THEME_TEXT_BOOK, tb_theme);

  // Text page.
  SharedTheme tp_theme(new Theme(TextWindow::COLOR_COUNT, 0));
  Setting tp_setting = root.Get("text_page", Setting::kGroup);
  if (tp_setting) {
    tp_theme->SetColor(TextWindow::RULER, tp_setting.GetColor("ruler"));
  }
  theme->SetTheme(THEME_TEXT_PAGE, tp_theme);

  // Status bar.
  SharedTheme sb_theme(
      new Theme(StatusBar::COLOR_COUNT, StatusBar::FONT_COUNT));
  Setting sb_setting = root.Get("status_bar", Setting::kGroup);
  if (sb_setting) {
    sb_theme->SetColor(StatusBar::BORDER, sb_setting.GetColor("border"));
    sb_theme->SetColor(StatusBar::FG, sb_setting.GetColor("fg"));
    sb_theme->SetColor(StatusBar::BG, sb_setting.GetColor("bg"));

    wxFont font = sb_setting.GetFont("font");
    if (!font.IsOk()) {
      font = wxSystemSettings::GetFont(wxSYS_DEFAULT_GUI_FONT);
    }
    sb_theme->SetFont(StatusBar::FONT, font);
  }
  theme->SetTheme(THEME_STATUS_BAR, sb_theme);

  //----------------------------------------------------------------------------
  // Non-lex items.

  StyleValue style_value;

  ReadStyle(root, "normal", &style_value);
  style->Set(Style::kNormal, style_value);

  ReadStyle(root, "visual", &style_value);
  style->Set(Style::kVisual, style_value);

  ReadStyle(root, "number", &style_value);
  style->Set(Style::kNumber, style_value);

  ReadStyle(root, "caret_line", &style_value);
  style->Set(Style::kCaretLine, style_value);

  ReadStyle(root, "space", &style_value);
  style->Set(Style::kSpace, style_value);

  ReadStyle(root, "operator", &style_value);
  style->Set(Style::kOperator, style_value);

  ReadStyle(root, "error", &style_value);
  style->Set(Style::kError, style_value);

  ReadStyle(root, "blank", &style_value);
  style->Set(Style::kBlank, style_value);

  //----------------------------------------------------------------------------
  // Lex items.

  Setting setting;

  style->Set(Lex(kLexNone), style->Get(Style::kNormal)->fg());

  setting = root.Get("comment", Setting::kGroup);
  if (setting) {
    ReadStyle(setting, &style_value);
    style->Set(Lex(kLexComment), style_value);
  }

  setting = root.Get("constant", Setting::kGroup);
  if (setting) {
    ReadStyle(setting, &style_value);
    style->Set(Lex(kLexConstant), style_value);
  }

  setting = root.Get("identifier", Setting::kGroup);
  if (setting) {
    ReadStyle(setting, &style_value);
    style->Set(Lex(kLexIdentifier), style_value);
  }

  setting = root.Get("statement", Setting::kGroup);
  if (setting) {
    ReadStyle(setting, &style_value);
    style->Set(Lex(kLexStatement), style_value);
  }

  setting = root.Get("package", Setting::kGroup);
  if (setting) {
    ReadStyle(setting, &style_value);
    style->Set(Lex(kLexPackage), style_value);
  }

  setting = root.Get("preproc", Setting::kGroup);
  if (setting) {
    ReadStyle(setting, &style_value);
    style->Set(Lex(kLexPreProc), style_value);
  }

  setting = root.Get("type", Setting::kGroup);
  if (setting) {
    ReadStyle(setting, &style_value);
    style->Set(Lex(kLexType), style_value);

    Setting child_setting = setting.Get("qualifier");
    if (child_setting) {
      ReadStyle(child_setting, &style_value);
      style->Set(Lex(kLexType, kLexTypeQualifier), style_value);
    }
  }

  setting = root.Get("special", Setting::kGroup);
  if (setting) {
    ReadStyle(setting, &style_value);
    style->Set(Lex(kLexSpecial), style_value);
  }

  return true;
}

}  // namespace jil
