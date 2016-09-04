#include "app/pref/global_font_page.h"

#include <algorithm>

#include "wx/button.h"
#include "wx/checkbox.h"
#include "wx/combobox.h"
#include "wx/dc.h"
#include "wx/fontutil.h"
#include "wx/listctrl.h"
#include "wx/log.h"
#include "wx/sizer.h"
#include "wx/statline.h"
#include "wx/stattext.h"

#include "editor/defs.h"
#include "editor/option.h"
#include "editor/util.h"

#include "ui/string_list_ctrl.h"

#include "app/option.h"
#include "app/util.h"
#include "app/pref/common.h"

namespace jil {
namespace pref {

static const wxString kFixedFontNameSuffix = wxT(" *");

////////////////////////////////////////////////////////////////////////////////

class FontEnumerator : public wxFontEnumerator {
public:
  virtual bool OnFacename(const wxString& facename) override {
    if (facename[0] != wxT('@')) {
      facenames->push_back(facename);
    }
    return true;
  }

  std::vector<wxString>* facenames;
};

////////////////////////////////////////////////////////////////////////////////

BEGIN_EVENT_TABLE(Global_FontPage, wxPanel)
EVT_LIST_ITEM_SELECTED(ID_FONT_LIST_CTRL, Global_FontPage::OnFontListSelectionChange)
EVT_LIST_ITEM_DESELECTED(ID_FONT_LIST_CTRL, Global_FontPage::OnFontListSelectionChange)
EVT_COMBOBOX(ID_FONT_NAME_COMBOBOX, Global_FontPage::OnNameComboBox)
EVT_COMBOBOX(ID_FONT_SIZE_COMBOBOX, Global_FontPage::OnSizeComboBox)
EVT_BUTTON(ID_FONT_USE_DEFAULT_BUTTON, Global_FontPage::OnUseDefaultButton)
END_EVENT_TABLE()

Global_FontPage::Global_FontPage(Options* options)
    : options_(options) {
  InitFonts();
}

Global_FontPage::~Global_FontPage() {
  Unbind(wxEVT_THREAD, &Global_FontPage::OnThreadUpdate, this);

  if (GetThread() != NULL && GetThread()->IsRunning()) {
    wxLogWarning(wxT("Wait for the font enumeration thread to complete..."));
    GetThread()->Wait();
  }
}

bool Global_FontPage::Create(wxWindow* parent, wxWindowID id) {
  if (!wxPanel::Create(parent, id)) {
    return false;
  }

  CreateControls();

  Bind(wxEVT_THREAD, &Global_FontPage::OnThreadUpdate, this);

  return true;
}

bool Global_FontPage::TransferDataToWindow() {
  SetFontToWindow(wxNullFont);
  use_default_button_->Enable(false);
  return true;
}

bool Global_FontPage::TransferDataFromWindow() {
  for (int i = 0; i < FONT_COUNT; ++i) {
    options_->fonts[i] = fonts_[i];
  }
  return true;
}

void Global_FontPage::EnumerateFonts() {
  if (!font_names_.empty()) {
    return;
  }

  if (CreateThread(wxTHREAD_JOINABLE) != wxTHREAD_NO_ERROR) {
    wxLogError("Can't create the font enum thread!");
    return;
  }

  if (GetThread()->Run() != wxTHREAD_NO_ERROR) {
    wxLogError("Can't run the font enum thread!");
    return;
  }
}

wxThread::ExitCode Global_FontPage::Entry() {
  FontEnumerator fe;

  // Enumerate all font names.
  fe.facenames = &font_names_;
  fe.EnumerateFacenames(wxFONTENCODING_SYSTEM, false);

  // Sort the font names.
  std::sort(font_names_.begin(), font_names_.end());

  // Enumerate fixed font names.
  std::vector<wxString> fixed_font_names;
  fe.facenames = &fixed_font_names;
  fe.EnumerateFacenames(wxFONTENCODING_SYSTEM, true);

  // Append " *" to the fixed font names.
  std::vector<wxString>::iterator it;
  for (wxString& font_name : fixed_font_names) {
    it = std::lower_bound(font_names_.begin(), font_names_.end(), font_name);
    if (it != font_names_.end()) {
      *it = font_name + kFixedFontNameSuffix;
    }
  }

  wxQueueEvent(GetEventHandler(), new wxThreadEvent());
  return (wxThread::ExitCode)0;
}

void Global_FontPage::OnThreadUpdate(wxThreadEvent& evt) {
  // Performance Notes (Win7 64bit, Jil 32-bit):
  //
  // 1. combo_box->Append(font_names_.size(), &font_names_[0]);
  //
  // 2. for (const wxString& name : font_names_) {
  //      combo_box->Append(name);
  //    }
  //
  // Result (in ms):
  // 1. Debug: 23   Release: 20
  // 2. Debug: 826  Release: 433

  name_combo_box_->Append(font_names_.size(), &font_names_[0]);
}

void Global_FontPage::InitFonts() {
  for (int i = 0; i < FONT_COUNT; ++i) {
    fonts_[i] = options_->fonts[i];
  }
}

void Global_FontPage::CreateControls() {
  wxSizer* top_vsizer = new wxBoxSizer(wxVERTICAL);

  CreateTypeSection(top_vsizer);
  CreateFontSection(top_vsizer);

  use_default_button_ = new wxButton(this, ID_FONT_USE_DEFAULT_BUTTON, _("Use default font"));
  top_vsizer->Add(use_default_button_, wxSizerFlags().Right().Border(wxRIGHT));
  top_vsizer->AddSpacer(10);

  SetSizerAndFit(top_vsizer);
}

void Global_FontPage::CreateTypeSection(wxSizer* top_vsizer) {
  wxStaticText* label = new wxStaticText(this, wxID_ANY, _("Display items:"));

  font_list_ctrl_ = new ui::StringListCtrl;
  font_list_ctrl_->Create(this, ID_FONT_LIST_CTRL);

  wxString tr_labels[FONT_COUNT] = {
    _("Text Editor"),
    _("Tabs"),
    _("Status Bar"),
    _("Find Panel"),
  };

  for (int i = 0; i < FONT_COUNT; ++i) {
    font_list_ctrl_->AppendString(tr_labels[i]);
  }

  font_list_ctrl_->UpdateSizes();

  top_vsizer->Add(label, wxSizerFlags().Left().Border(wxLEFT|wxTOP));
  top_vsizer->Add(font_list_ctrl_, wxSizerFlags().Expand().Border(wxALL));
}

void Global_FontPage::CreateFontSection(wxSizer* top_vsizer) {
  name_combo_box_ = new wxComboBox(this, ID_FONT_NAME_COMBOBOX);

  size_combo_box_ = new wxComboBox(this,
                                   ID_FONT_SIZE_COMBOBOX,
                                   wxEmptyString,
                                   wxDefaultPosition,
                                   kSmallNumTextSize);
  wxArrayString sizes;
  for (int i = kMinFontSize; i <= kMaxFontSize; ++i) {
    sizes.Add(wxString::Format(wxT("%d"), i));
  }
  size_combo_box_->Append(sizes.size(), &sizes[0]);

  wxBoxSizer* hsizer = new wxBoxSizer(wxHORIZONTAL);
  hsizer->Add(name_combo_box_, wxSizerFlags(1));
  hsizer->Add(size_combo_box_, wxSizerFlags().Border(wxLEFT, 10));
  top_vsizer->Add(hsizer, wxSizerFlags().Expand().Border(wxALL));
}

FontType Global_FontPage::GetSelectedFontType() const {
  int index = font_list_ctrl_->selected_index();
  if (index >= 0 && index < FONT_COUNT) {
    return static_cast<FontType>(index);
  }
  return FONT_COUNT;
}

void Global_FontPage::SetFontToWindow(const wxFont& font) {
  if (font.IsOk()) {
    name_combo_box_->SetValue(font.GetFaceName());
    size_combo_box_->SetValue(wxString::Format(wxT("%d"), font.GetPointSize()));

    name_combo_box_->Enable(true);
    size_combo_box_->Enable(true);
  } else {
    name_combo_box_->SetValue(wxEmptyString);
    size_combo_box_->SetValue(wxEmptyString);

    name_combo_box_->Enable(false);
    size_combo_box_->Enable(false);
  }
}

void Global_FontPage::OnFontListSelectionChange(wxListEvent& evt) {
  FontType font_type = GetSelectedFontType();
  if (font_type != FONT_COUNT) {
    SetFontToWindow(fonts_[font_type]);
    use_default_button_->Enable(true);
  } else {
    SetFontToWindow(wxNullFont);
    use_default_button_->Enable(false);
  }
}

void Global_FontPage::OnNameComboBox(wxCommandEvent& evt) {
  FontType font_type = GetSelectedFontType();
  if (font_type == FONT_COUNT) {
    return;
  }

  wxFont& font = fonts_[font_type];

  wxString name = name_combo_box_->GetValue();
  if (name.EndsWith(kFixedFontNameSuffix)) {
    name.RemoveLast(kFixedFontNameSuffix.Length());
  }
  if (!font.SetFaceName(name)) {
    return;
  }
}

void Global_FontPage::OnSizeComboBox(wxCommandEvent& evt) {
  FontType font_type = GetSelectedFontType();
  if (font_type == FONT_COUNT) {
    return;
  }

  wxFont& font = fonts_[font_type];

  long size = GetDefaultFontSize();
  if (!size_combo_box_->GetValue().ToLong(&size)) {
    return;
  }
  if (size < kMinFontSize || size > kMaxFontSize) {
    size = GetDefaultFontSize();
  }
  font.SetPointSize(static_cast<int>(size));
}

void Global_FontPage::OnUseDefaultButton(wxCommandEvent& evt) {
  FontType font_type = GetSelectedFontType();
  if (font_type != FONT_COUNT) {
    fonts_[font_type] = GetDefaultFont(font_type);

    // Update font name and size combo boxes.
    SetFontToWindow(fonts_[font_type]);
  }
}

}  // namespace pref
}  // namespace jil
