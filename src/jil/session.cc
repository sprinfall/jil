#include "jil/session.h"
#include <algorithm>
#include <string>
#include "wx/log.h"
#include "jil/config.h"
#include "jil/splitter.h"
#include "jil/util.h"

#define BOOK_FRAME        "book_frame"
#define FIND_PANEL        "find_panel"
#define RECT              "rect"
#define SIZE              "size"
#define MAXIMIZED         "maximized"

#define SPLIT             "split"
#define ID                "id"
#define ORIENTATION       "orientation"
#define VERTICAL          "vertical"
#define HORIZONTAL        "horizontal"
#define SIZE_RATIO        "size_ratio"
#define NODE1             "node1"
#define NODE2             "node2"

#define LOCATION          "location"
#define USE_REGEX         "use_regex"
#define CASE_SENSITIVE    "case_sensitive"
#define MATCH_WORD        "match_word"
#define FIND_STRINGS      "find_strings"
#define REPLACE_STRINGS   "replace_strings"

#define OPENED_FILES      "opened_files"
#define FILE_PATH         "file_path"
#define STACK_INDEX       "stack_index"
#define RECENT_FILES      "recent_files"

namespace jil {

static bool GetSize(Setting parent, const char* name, wxSize* size) {
  Setting size_setting = parent.Get(name);
  if (!size_setting) {
    return false;
  }

  if (size_setting.type() != Setting::kArray || size_setting.size() != 2) {
    return false;
  }

  size->x = size_setting[0].GetInt();
  size->y = size_setting[1].GetInt();

  return true;
}

static void SetSize(Setting parent, const char* name, const wxSize& size) {
  parent.Remove(name);
  Setting size_setting = parent.Add(name, Setting::kArray);
  size_setting.Add(NULL, Setting::kInt).SetInt(size.x);
  size_setting.Add(NULL, Setting::kInt).SetInt(size.y);
}

static bool GetRect(Setting parent, const char* name, wxRect* rect) {
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

static void SetRect(Setting parent, const char* name, const wxRect& rect) {
  parent.Remove(name);
  Setting rect_setting = parent.Add(name, Setting::kArray);
  rect_setting.Add(NULL, Setting::kInt).SetInt(rect.x);
  rect_setting.Add(NULL, Setting::kInt).SetInt(rect.y);
  rect_setting.Add(NULL, Setting::kInt).SetInt(rect.width);
  rect_setting.Add(NULL, Setting::kInt).SetInt(rect.height);
}

static bool GetStringArray(Setting parent, const char* name, std::list<wxString>* strings) {
  Setting setting = parent.Get(name, Setting::kArray);
  if (!setting) {
    return false;
  }

  int size = setting.size();
  for (int i = 0; i < size; ++i) {
    strings->push_back(wxString::FromUTF8(setting[i].GetString()));
  }

  return true;
}

static void SetStringArray(Setting parent, const char* name, const std::list<wxString>& strings) {
  parent.Remove(name);
  Setting setting = parent.Add(name, Setting::kArray);
  for (const wxString& str : strings) {
    setting.Add(NULL, Setting::kString).SetString(str.ToUTF8().data());
  }
}

////////////////////////////////////////////////////////////////////////////////

Session::Session()
    : book_frame_maximized_(false)
    , find_history_limit_(kFindHistoryLimit)
    , find_flags_(0)
    , find_location_(kCurrentPage)
    , split_root_(NULL) {
}

Session::~Session() {
  if (split_root_ != NULL) {
    delete split_root_;
  }
}

bool Session::Load(const wxString& cfg_file) {
  Config config;
  if (!config.Load(cfg_file)) {
    return false;
  }

  Setting root_setting = config.Root();

  //----------------------------------------------------------------------------
  // Book frame

  Setting bf_setting = root_setting.Get(BOOK_FRAME, Setting::kGroup);
  if (bf_setting) {
#if defined (__WXGTK__)
    GetSize(bf_setting, SIZE, &book_frame_size_);
#else
    GetRect(bf_setting, RECT, &book_frame_rect_);
#endif  // __WXGTK__

    book_frame_maximized_ = bf_setting.GetBool(MAXIMIZED);
  }

  //----------------------------------------------------------------------------
  // Find panel

  Setting fp_setting = root_setting.Get(FIND_PANEL, Setting::kGroup);
  if (fp_setting) {
    GetStringArray(fp_setting, FIND_STRINGS, &find_strings_);
    GetStringArray(fp_setting, REPLACE_STRINGS, &replace_strings_);

    find_flags_ = SetBit(find_flags_, kFind_UseRegex, fp_setting.GetBool(USE_REGEX));
    find_flags_ = SetBit(find_flags_, kFind_CaseSensitive, fp_setting.GetBool(CASE_SENSITIVE));
    find_flags_ = SetBit(find_flags_, kFind_MatchWord, fp_setting.GetBool(MATCH_WORD));

    int location = fp_setting.GetInt(LOCATION);
    if (location >= 0 && location < kFindLocations) {
      find_location_ = static_cast<FindLocation>(location);
    }
  }

  //----------------------------------------------------------------------------
  // Split tree

  Setting split_setting = root_setting.Get(SPLIT, Setting::kGroup);
  if (split_setting) {
    split_root_ = RestoreSplitTree(split_setting);
  }

  //----------------------------------------------------------------------------
  // File history

  Setting of_setting = root_setting.Get(OPENED_FILES, Setting::kList);
  if (of_setting) {
    int size = of_setting.size();
    for (int i = 0; i < size; ++i) {
      Setting setting = of_setting[i];
      if (setting.type() == Setting::kGroup) {
        OpenedFile opened_file;
        opened_file.file_path = wxString::FromUTF8(setting.GetString(FILE_PATH));
        opened_file.stack_index = setting.GetInt(STACK_INDEX);
        opened_files_.push_back(opened_file);
      }
    }
  }

  //----------------------------------------------------------------------------
  // Recent files

  GetStringArray(root_setting, RECENT_FILES, &recent_files_);

  return true;
}

bool Session::Save(const wxString& cfg_file) {
  Config config;

  Setting root_setting = config.Root();

  //----------------------------------------------------------------------------
  // Book frame

  Setting bf_setting = root_setting.Add(BOOK_FRAME, Setting::kGroup);

#if defined (__WXGTK__)
  if (book_frame_size_ != wxDefaultSize) {
    SetSize(bf_setting, SIZE, book_frame_size_);
  }
#else
  if (!book_frame_rect_.IsEmpty()) {
    SetRect(bf_setting, RECT, book_frame_rect_);
  }
#endif  // __WXGTK__

  bf_setting.SetBool(MAXIMIZED, book_frame_maximized_);

  //----------------------------------------------------------------------------
  // Find panel

  Setting fp_setting = root_setting.Add(FIND_PANEL, Setting::kGroup);

  SetStringArray(fp_setting, FIND_STRINGS, find_strings_);
  SetStringArray(fp_setting, REPLACE_STRINGS, replace_strings_);

  fp_setting.SetInt(LOCATION, find_location_);

  fp_setting.SetBool(USE_REGEX, GetBit(find_flags_, kFind_UseRegex));
  fp_setting.SetBool(CASE_SENSITIVE, GetBit(find_flags_, kFind_CaseSensitive));
  fp_setting.SetBool(MATCH_WORD, GetBit(find_flags_, kFind_MatchWord));

  //----------------------------------------------------------------------------
  // Split tree

  if (split_root_ != NULL) {
    Setting split_setting = root_setting.Add(SPLIT, Setting::kGroup);
    SaveSplitTree(split_root_, &split_setting);
  }

  //----------------------------------------------------------------------------
  // Opened files

  Setting of_setting = root_setting.Add(OPENED_FILES, Setting::kList);

  for (OpenedFile& opened_file : opened_files_) {
    Setting setting = of_setting.Add(NULL, Setting::kGroup);
    setting.SetString(FILE_PATH, opened_file.file_path.ToUTF8().data());
    setting.SetInt(STACK_INDEX, opened_file.stack_index);
  }

  //----------------------------------------------------------------------------
  // Recent files

  SetStringArray(root_setting, RECENT_FILES, recent_files_);

  // Save to file.
  return config.Save(cfg_file);
}

bool Session::AddHistoryString(std::list<wxString>& strings, const wxString& s,
                               std::size_t limit) {
  auto it = std::find(strings.begin(), strings.end(), s);

  if (it == strings.end()) {
    // Doesn't exist.
    strings.push_front(s);
    if (strings.size() > limit) {
      strings.pop_back();
    }
    return true;
  } else {
    if (it != strings.begin()) {
      // Exist but not in the front, move it to front.
      strings.erase(it);
      strings.push_front(s);
    }
    return false;
  }
}

void Session::SaveSplitTree(SplitNode* n, Setting* setting) {
  SplitLeaf* leaf = n->AsLeaf();
  if (leaf != NULL) {
    setting->SetString(ID, leaf->id().c_str());
    return;
  }

  setting->SetString(ORIENTATION, n->vertical() ? VERTICAL : HORIZONTAL);
  setting->SetFloat(SIZE_RATIO, n->size_ratio());

  if (n->node1() == NULL || n->node2() == NULL) {
    wxLogWarning(wxT("Non-leaf split node must have 2 child nodes!"));
    return;
  }

  Setting setting1 = setting->Add(NODE1, Setting::kGroup);
  SaveSplitTree(n->node1(), &setting1);

  Setting setting2 = setting->Add(NODE2, Setting::kGroup);
  SaveSplitTree(n->node2(), &setting2);
}

SplitNode* Session::RestoreSplitTree(Setting setting) {
  Setting id_setting = setting.Get(ID);
  if (id_setting) {  // Leaf
    SplitLeaf* leaf = new SplitLeaf(NULL);
    leaf->set_id(id_setting.GetString());
    return leaf;
  }

  SplitNode* node = new SplitNode;

  std::string orientation = setting.GetString(ORIENTATION);
  if (orientation == VERTICAL) {
    node->set_orientation(wxVERTICAL);
  } else {
    node->set_orientation(wxHORIZONTAL);
  }

  double size_ratio = setting.GetFloat(SIZE_RATIO);
  node->set_size_ratio(size_ratio);

  Setting setting1 = setting.Get(NODE1, Setting::kGroup);
  Setting setting2 = setting.Get(NODE2, Setting::kGroup);
  if (setting1 && setting2) {
    node->set_node1(RestoreSplitTree(setting1));
    node->set_node2(RestoreSplitTree(setting2));
  }

  return node;
}

}  // namespace jil
