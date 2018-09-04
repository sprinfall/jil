#include "jil/pref/editor_comment_page.h"

#include "wx/checkbox.h"
#include "wx/sizer.h"
#include "wx/stattext.h"

#include "ui/static_box.h"

#include "editor/option.h"

#include "jil/defs.h"
#include "jil/pref/common.h"

namespace jil {
namespace pref {

bool Editor_CommentPage::Create(wxWindow* parent, wxWindowID id) {
  if (!wxPanel::Create(parent, id)) {
    return false;
  }

  CreateControls();

  return true;
}

bool Editor_CommentPage::TransferDataToWindow() {
  add_space_check_box_->SetValue(options_->text.comment_add_space);
  respect_indent_check_box_->SetValue(options_->text.comment_respect_indent);

  return true;
}

bool Editor_CommentPage::TransferDataFromWindow() {
  options_->text.comment_add_space = add_space_check_box_->GetValue();
  options_->text.comment_respect_indent = respect_indent_check_box_->GetValue();

  return true;
}

void Editor_CommentPage::CreateControls() {
  wxSizer* top_vsizer = new wxBoxSizer(wxVERTICAL);

  ui::StaticBox* box = new ui::StaticBox(this, wxEmptyString);
  wxSizer* box_vsizer = new wxBoxSizer(wxVERTICAL);

  add_space_check_box_ = new wxCheckBox(box, wxID_ANY, _("Add space"));
  respect_indent_check_box_ = new wxCheckBox(box, wxID_ANY,
                                             _("Respect line indent"));

  box_vsizer->Add(add_space_check_box_, wxSizerFlags().Border(wxTOP));
  box_vsizer->Add(respect_indent_check_box_,
                  wxSizerFlags().Expand().Border(wxTOP));

  box->SetBodySizer(box_vsizer);
  top_vsizer->Add(box, wxSizerFlags().Expand().Border(wxALL));

  SetSizerAndFit(top_vsizer);
}

}  // namespace pref
}  // namespace jil
