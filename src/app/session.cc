#include "app/session.h"
#include <algorithm>
#include <string>
#include "wx/log.h"
#include "app/config.h"
#include "app/splitter.h"
#include "app/util.h"

namespace jil {

// Setting names.
const char* const kBookFrame = "book_frame";
const char* const kFindWindow = "find_window";
const char* const kRect = "rect";
const char* const kMaximized = "maximized";
const char* const kFindStrings = "find_strings";
const char* const kReplaceStrings = "replace_strings";

namespace {

bool GetRect(Setting parent, const char* name, wxRect* rect) {
  Setting rect_setting = parent.Get(name);
  if (!rect_setting) {
    return false;
  }

  if (rect_setting.type() != Setting::kArray || rect_setting.size() != 4) {
    return false;
  }

  rect->x = rect_setting[0].GetInt();
  rect->y = rect_setting[1].GetInt();
  rect->width = rect_setting[2].GetInt();
  rect->height = rect_setting[3].GetInt();

  return true;
}

void SetRect(Setting parent, const char* name, const wxRect& rect) {
  parent.Remove(name);
  Setting rect_setting = parent.Add(name, Setting::kArray);
  rect_setting.Add(NULL, Setting::kInt).SetInt(rect.x);
  rect_setting.Add(NULL, Setting::kInt).SetInt(rect.y);
  rect_setting.Add(NULL, Setting::kInt).SetInt(rect.width);
  rect_setting.Add(NULL, Setting::kInt).SetInt(rect.height);
}

bool GetStringArray(Setting parent,
                    const char* name,
                    std::list<wxString>* strings) {
  Setting setting = parent.Get(name, Setting::kArray);
  if (!setting) {
    return false;
  }

  for (int i = 0; i < setting.size(); ++i) {
    strings->push_back(wxString::FromUTF8(setting[i].GetString()));
  }
  return true;
}

void SetStringArray(Setting parent,
                    const char* name,
                    const std::list<wxString>& strings) {
  parent.Remove(name);

  Setting setting = parent.Add(name, Setting::kArray);

  std::list<wxString>::const_iterator it = strings.begin();
  for (; it != strings.end(); ++it) {
    setting.Add(NULL, Setting::kString).SetString(it->ToUTF8().data());
  }
}

void SetStringArray(Setting parent,
                    const char* name,
                    const std::list<wxString>& strings,
                    size_t limit) {
  parent.Remove(name);

  Setting setting = parent.Add(name, Setting::kArray);

  size_t index = 0;
  std::list<wxString>::const_iterator it = strings.begin();
  for (; it != strings.end(); ++it) {
    setting.Add(NULL, Setting::kString).SetString(it->ToUTF8().data());
    if (++index >= limit) {
      break;
    }
  }
}

}  // namespace

Session::Session()
    : book_frame_maximized_(false)
    , find_history_limit_(10)
    , find_flags_(0)
    , split_root_(NULL) {
}

Session::~Session() {
  if (split_root_ != NULL) {
    delete split_root_;
  }
}

bool Session::Load(const wxString& file) {
  Config config;
  if (!config.Load(file)) {
    return false;
  }

  Setting root_setting = config.Root();

  //----------------------------------------------------------------------------
  // Book frame

  Setting bf_setting = root_setting.Get(kBookFrame, Setting::kGroup);
  if (bf_setting) {
    GetRect(bf_setting, kRect, &book_frame_rect_);
    book_frame_maximized_ = bf_setting.GetBool(kMaximized);
  }

  //----------------------------------------------------------------------------
  // Find window

  Setting fw_setting = root_setting.Get(kFindWindow, Setting::kGroup);
  if (fw_setting) {
    GetRect(fw_setting, kRect, &find_window_rect_);

    GetStringArray(fw_setting, kFindStrings, &find_strings_);
    GetStringArray(fw_setting, kReplaceStrings, &replace_strings_);

    find_flags_ = SetBit(find_flags_,
                         kFind_UseRegex,
                         fw_setting.GetBool("use_regex"));
    find_flags_ = SetBit(find_flags_,
                         kFind_CaseSensitive,
                         fw_setting.GetBool("case_sensitive"));
    find_flags_ = SetBit(find_flags_,
                         kFind_MatchWholeWord,
                         fw_setting.GetBool("match_whole_word"));
  }

  //----------------------------------------------------------------------------
  // Split tree

  Setting split_setting = root_setting.Get("split", Setting::kGroup);
  if (split_setting) {
    split_root_ = RestoreSplitTree(split_setting);
  }

  //----------------------------------------------------------------------------
  // File history

  Setting fs_setting = root_setting.Get("file_history", Setting::kGroup);
  if (fs_setting) {
    GetStringArray(fs_setting, "opened_files", &opened_files_);
    GetStringArray(fs_setting, "recent_files", &recent_files_);
  }

  return true;
}

bool Session::Save(const wxString& file) {
  Config config;

  Setting root_setting = config.Root();

  //----------------------------------------------------------------------------
  // Book frame

  Setting bf_setting = root_setting.Add(kBookFrame, Setting::kGroup);

  if (!book_frame_rect_.IsEmpty()) {
    SetRect(bf_setting, kRect, book_frame_rect_);
  }

  bf_setting.SetBool(kMaximized, book_frame_maximized_);

  //----------------------------------------------------------------------------
  // Find window

  Setting fw_setting = root_setting.Add(kFindWindow, Setting::kGroup);

  if (!find_window_rect_.IsEmpty()) {
    SetRect(fw_setting, kRect, find_window_rect_);
  }

  SetStringArray(fw_setting,
                 kFindStrings,
                 find_strings_,
                 find_history_limit_);

  SetStringArray(fw_setting,
                 kReplaceStrings,
                 replace_strings_,
                 find_history_limit_);

  fw_setting.SetBool("use_regex", GetBit(find_flags_, kFind_UseRegex));
  fw_setting.SetBool("case_sensitive", GetBit(find_flags_, kFind_CaseSensitive));
  fw_setting.SetBool("match_whole_word",
                     GetBit(find_flags_, kFind_MatchWholeWord));

  //----------------------------------------------------------------------------
  // Split tree

  if (split_root_ != NULL) {
    Setting split_setting = root_setting.Add("split", Setting::kGroup);
    SaveSplitTree(split_root_, &split_setting);
  }

  //----------------------------------------------------------------------------
  // File history

  Setting fs_setting = root_setting.Add("file_history", Setting::kGroup);
  SetStringArray(fs_setting, "opened_files", opened_files_);
  SetStringArray(fs_setting, "recent_files", recent_files_);

  // Save to file.
  return config.Save(file);
}

bool Session::AddHistoryString(const wxString& s,
                               std::list<wxString>* strings) {
  std::list<wxString>::iterator it =
      std::find(strings->begin(), strings->end(), s);
  if (it == strings->end()) {
    // Doesn't exist.
    strings->push_front(s);
    return true;
  } else {
    if (it != strings->begin()) {
      // Exist but not in the front, move it to front.
      strings->erase(it);
      strings->push_front(s);
    }
    return false;
  }
}

void Session::SaveSplitTree(SplitNode* n, Setting* setting) {
  SplitLeaf* leaf = n->AsLeaf();
  if (leaf != NULL) {
    setting->SetString("id", leaf->id().c_str());
    return;
  }

  setting->SetString("orientation", n->vertical() ? "vertical" : "horizontal");
  setting->SetFloat("size_ratio", n->size_ratio());

  if (n->node1() == NULL || n->node2() == NULL) {
    wxLogWarning(wxT("Non-leaf split node must have 2 child nodes!"));
    return;
  }

  Setting setting1 = setting->Add("node1", Setting::kGroup);
  SaveSplitTree(n->node1(), &setting1);

  Setting setting2 = setting->Add("node2", Setting::kGroup);
  SaveSplitTree(n->node2(), &setting2);
}

SplitNode* Session::RestoreSplitTree(Setting setting) {
  Setting id_setting = setting.Get("id");
  if (id_setting) {  // Leaf
    SplitLeaf* leaf = new SplitLeaf(NULL);
    leaf->set_id(id_setting.GetString());
    return leaf;
  }

  SplitNode* node = new SplitNode;

  std::string orientation = setting.GetString("orientation");
  if (orientation == "vertical") {
    node->set_orientation(wxVERTICAL);
  } else {
    node->set_orientation(wxHORIZONTAL);
  }

  double size_ratio = setting.GetFloat("size_ratio");
  node->set_size_ratio(size_ratio);

  Setting setting1 = setting.Get("node1", Setting::kGroup);
  Setting setting2 = setting.Get("node2", Setting::kGroup);
  if (setting1 && setting2) {
    node->set_node1(RestoreSplitTree(setting1));
    node->set_node2(RestoreSplitTree(setting2));
  }

  return node;
}

}  // namespace jil
