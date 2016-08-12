#include "app/theme_config.h"
#include <string>
#include <vector>
#include "boost/algorithm/string.hpp"
#include "wx/dir.h"
#include "wx/filename.h"
#include "wx/log.h"
#include "ui/color.h"
#include "editor/lex.h"
#include "editor/style.h"
#include "editor/text_window.h"
#include "app/navigation_dialog.h"
#include "app/defs.h"
#include "app/config.h"
#include "app/book_ctrl.h"
#include "app/book_frame.h"
#include "app/find_panel.h"
#include "app/status_bar.h"

namespace jil {

using namespace editor;

// Read style value from the setting.
static void ReadStyle(Setting setting, StyleValue* style_value) {
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
static void ReadStyle(Setting parent,
                      const char* name,
                      StyleValue* style_value) {
  assert(parent);

  style_value->Set();  // Clear

  Setting setting = parent.Get(name, Setting::kGroup);
  if (setting) {
    ReadStyle(setting, style_value);
  }
}

static SharedTheme GetButtonTheme(Setting button_setting) {
  assert(button_setting);

  const char* kButtonParts[] = {
    "fg", "bg1", "bg2", "bg3", "border_outer", "border_inner"
  };
  const char* kButtonStates[] = {
    "normal", "hover", "pressed", "pressed_hover", "disabled"
  };

  SharedTheme button_theme(new Theme(ui::ButtonStyle::PARTS));

  for (int part = 0; part < ui::ButtonStyle::PARTS; ++part) {
    Setting fg_setting = button_setting.Get(kButtonParts[part], Setting::kGroup);
    if (fg_setting) {
      SharedTheme part_theme(new Theme(0, ui::ButtonStyle::STATES, 0));
      for (int state = 0; state < ui::ButtonStyle::STATES; ++state) {
        part_theme->SetColor(state, fg_setting.GetColor(kButtonStates[state]));
      }
      button_theme->SetTheme(part, part_theme);
    }
  }

  return button_theme;
}

static wxBitmap GetImage(const wxString& dir, const wxString& name) {
  return wxBitmap(dir + name + wxT(".png"), wxBITMAP_TYPE_PNG);
}

// TODO: Error handling
// TODO: Rename theme_file
bool LoadThemeFile(const wxString& theme_file, SharedTheme& theme, Style* style) {
  if (!wxDir::Exists(theme_file)) {
    wxLogError(wxT("The theme folder '%s' doesn't exist!"), theme_file);
    return false;
  }

  wxString image_dir = theme_file + wxFILE_SEP_PATH + wxT("image") + wxFILE_SEP_PATH;

  wxString color_file = theme_file + wxFILE_SEP_PATH + wxT("color.cfg");

  Config cfg;
  if (!cfg.Load(color_file)) {
    wxLogError(wxT("Failed to parse theme/color file: %s!"), theme_file);
    return false;
  }

  Setting root = cfg.Root();

  //----------------------------------------------------------------------------
  // Window items

  // Book frame
  SharedTheme bf_theme = theme->GetTheme(THEME_BOOK_FRAME);
  if (!bf_theme) {
    bf_theme.reset(new Theme(0, BookFrame::COLORS, 0));
    theme->SetTheme(THEME_BOOK_FRAME, bf_theme);
  }

  Setting bf_setting = root.Get("book_frame", Setting::kGroup);
  if (bf_setting) {
    bf_theme->SetColor(BookFrame::COLOR_BG, bf_setting.GetColor("bg"));
  }

  // Text book
  SharedTheme tb_theme = theme->GetTheme(THEME_TEXT_BOOK);
  if (!tb_theme) {
    tb_theme.reset(new Theme(0, BookCtrl::COLORS, 0));
    theme->SetTheme(THEME_TEXT_BOOK, tb_theme);
  }

  Setting tb_setting = root.Get("text_book", Setting::kGroup);
  if (tb_setting) {
    tb_theme->SetColor(BookCtrl::COLOR_BG, tb_setting.GetColor("bg"));
    tb_theme->SetColor(BookCtrl::COLOR_TAB_AREA_BG, tb_setting.GetColor("tab_area_bg"));
    tb_theme->SetColor(BookCtrl::COLOR_TAB_FG, tb_setting.GetColor("tab_fg"));
    tb_theme->SetColor(BookCtrl::COLOR_ACTIVE_TAB_FG, tb_setting.GetColor("active_tab_fg"));
    tb_theme->SetColor(BookCtrl::COLOR_TAB_BG, tb_setting.GetColor("tab_bg"));
    tb_theme->SetColor(BookCtrl::COLOR_ACTIVE_TAB_BG, tb_setting.GetColor("active_tab_bg"));
    tb_theme->SetColor(BookCtrl::COLOR_TAB_BORDER, tb_setting.GetColor("tab_border"));
    tb_theme->SetColor(BookCtrl::COLOR_ACTIVE_TAB_BORDER, tb_setting.GetColor("active_tab_border"));
  }

  // Text page
  SharedTheme tp_theme = theme->GetTheme(THEME_TEXT_PAGE);
  if (!tp_theme) {
    tp_theme.reset(new Theme(0, TextWindow::COLORS, 0));
    theme->SetTheme(THEME_TEXT_PAGE, tp_theme);
  }

  Setting tp_setting = root.Get("text_page", Setting::kGroup);
  if (tp_setting) {
    tp_theme->SetColor(TextWindow::COLOR_RULER, tp_setting.GetColor("ruler"));
  }

  // Find panel
  SharedTheme fp_theme(new Theme(FindPanel::THEMES, FindPanel::COLORS, FindPanel::IMAGES));
  Setting fp_setting = root.Get("find_panel", Setting::kGroup);
  if (fp_setting) {
    fp_theme->SetColor(FindPanel::COLOR_FG, fp_setting.GetColor("fg"));
    fp_theme->SetColor(FindPanel::COLOR_BG_TOP, fp_setting.GetColor("bg_top"));
    fp_theme->SetColor(FindPanel::COLOR_BG_BOTTOM, fp_setting.GetColor("bg_bottom"));
    fp_theme->SetColor(FindPanel::COLOR_BORDER_OUTER, fp_setting.GetColor("border_outer"));
    fp_theme->SetColor(FindPanel::COLOR_BORDER_INNER, fp_setting.GetColor("border_inner"));

    Setting button_setting = fp_setting.Get("button", Setting::kGroup);
    if (button_setting) {
      fp_theme->SetTheme(FindPanel::THEME_BUTTON, GetButtonTheme(button_setting));
    }
  }
  // Load images.
  fp_theme->SetImage(FindPanel::IMAGE_HISTORY, GetImage(image_dir, wxT("fp_history")));
#if JIL_BMP_BUTTON_FIND_OPTIONS
  fp_theme->SetImage(FindPanel::IMAGE_LOCATION, GetImage(image_dir, wxT("fp_location")));
  fp_theme->SetImage(FindPanel::IMAGE_CASE_SENSITIVE, GetImage(image_dir, wxT("fp_case_sensitive")));
  fp_theme->SetImage(FindPanel::IMAGE_MATCH_WORD, GetImage(image_dir, wxT("fp_match_word")));
  fp_theme->SetImage(FindPanel::IMAGE_USE_REGEX, GetImage(image_dir, wxT("fp_use_regex")));
#endif  // JIL_BMP_BUTTON_FIND_OPTIONS
  fp_theme->SetImage(FindPanel::IMAGE_FOLDERS, GetImage(image_dir, wxT("fp_folders")));
  fp_theme->SetImage(FindPanel::IMAGE_ADD_FOLDER, GetImage(image_dir, wxT("fp_add_folder")));

  theme->SetTheme(THEME_FIND_PANEL, fp_theme);

  // Status bar
  SharedTheme sb_theme = theme->GetTheme(THEME_STATUS_BAR);
  if (!sb_theme) {
    sb_theme.reset(new Theme(0, StatusBar::COLORS, 0));
    theme->SetTheme(THEME_STATUS_BAR, sb_theme);
  }

  Setting sb_setting = root.Get("status_bar", Setting::kGroup);
  if (sb_setting) {
    sb_theme->SetColor(StatusBar::COLOR_FG, sb_setting.GetColor("fg"));
    sb_theme->SetColor(StatusBar::COLOR_BORDER_OUTER, sb_setting.GetColor("border_outer"));
    sb_theme->SetColor(StatusBar::COLOR_BORDER_INNER, sb_setting.GetColor("border_inner"));
    sb_theme->SetColor(StatusBar::COLOR_BG_TOP, sb_setting.GetColor("bg_top"));
    sb_theme->SetColor(StatusBar::COLOR_BG_BOTTOM, sb_setting.GetColor("bg_bottom"));
    sb_theme->SetColor(StatusBar::COLOR_SEPARATOR, sb_setting.GetColor("separator"));
  }

  // Navigation dialog
  SharedTheme nd_theme = theme->GetTheme(THEME_NAVIGATION_DIALOG);
  if (!nd_theme) {
    nd_theme.reset(new Theme(0, NavigationDialog::COLORS, 0));
    theme->SetTheme(THEME_NAVIGATION_DIALOG, nd_theme);
  }

  Setting nd_setting = root.Get("navigation_dialog", Setting::kGroup);
  if (nd_setting) {
    nd_theme->SetColor(NavigationDialog::COLOR_BG, nd_setting.GetColor("bg"));
    nd_theme->SetColor(NavigationDialog::COLOR_FG, nd_setting.GetColor("fg"));
    nd_theme->SetColor(NavigationDialog::COLOR_SELECT_FG, nd_setting.GetColor("select_fg"));
    nd_theme->SetColor(NavigationDialog::COLOR_SELECT_BG, nd_setting.GetColor("select_bg"));
    nd_theme->SetColor(NavigationDialog::COLOR_SELECT_BORDER, nd_setting.GetColor("select_border"));
  }

  //----------------------------------------------------------------------------
  // Non-lex items.

  StyleValue style_value;

  ReadStyle(root, "normal", &style_value);
  style->Set(Style::kNormal, style_value);

  ReadStyle(root, "visual", &style_value);
  style->Set(Style::kVisual, style_value);

  ReadStyle(root, "matching", &style_value);
  style->Set(Style::kMatching, style_value);

  ReadStyle(root, "number", &style_value);
  style->Set(Style::kNumber, style_value);

  ReadStyle(root, "caret_line", &style_value);
  style->Set(Style::kCaretLine, style_value);

  ReadStyle(root, "space", &style_value);
  style->Set(Style::kSpace, style_value);

  ReadStyle(root, "delimiter", &style_value);
  style->Set(Style::kDelimiter, style_value);

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
    }
    style->Set(Lex(kLexType, kLexTypeQualifier), style_value);
  }

  setting = root.Get("url", Setting::kGroup);
  if (setting) {
    ReadStyle(setting, &style_value);
    style->Set(Lex(kLexUrl), style_value);
  }

  setting = root.Get("error", Setting::kGroup);
  if (setting) {
    ReadStyle(setting, &style_value);
    style->Set(Lex(kLexError), style_value);
  }

  return true;
}

}  // namespace jil
