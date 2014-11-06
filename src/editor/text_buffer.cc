#include "editor/text_buffer.h"
#include <cassert>
#include <utility>
#include "boost/algorithm/string.hpp"
#include "boost/regex.hpp"
#include "uchardet/nscore.h"
#include "uchardet/nsUniversalDetector.h"
#include "wx/log.h"
#include "wx/filesys.h"
#include "base/string_util.h"
#include "editor/action.h"
#include "editor/compile_config.h"
#include "editor/ft_plugin.h"
#include "editor/file_io.h"
#include "editor/indent.h"
#include "editor/lex.h"
#include "editor/util.h"

#define JIL_SPLIT_LINES_WITH_FUNC 0
#define JIL_MARK_UNCLOSED_QUOTE 0

#define JIL_TEST_TIME_SET_TEXT 0
#define JIL_TEST_TIME_SCAN_LEX 0

namespace jil {
namespace editor {

////////////////////////////////////////////////////////////////////////////////

namespace {

const TextPoint kInvalidPoint(-1, 0);

bool IsBracketL(wchar_t c) {
  return c == L'(' ||
         c == L'{' ||
         c == L'[' ||
         c == L'<';
}

bool IsBracketR(wchar_t c) {
  return c == L')' ||
         c == L'}' ||
         c == L']' ||
         c == L'>';
}

Bracket BracketL(wchar_t c) {
  switch (c) {
    case L'(':
      return kParenthesis;
    case L'{':
      return kBrace;
    case L'[':
      return kSquareBracket;
    case L'<':
      return kAngleBracket;
    default:
      return kNoBracket;
  }
}

Bracket BracketR(wchar_t c) {
  switch (c) {
    case L')':
      return kParenthesis;
    case L'}':
      return kBrace;
    case L']':
      return kSquareBracket;
    case L'>':
      return kAngleBracket;
    default:
      return kNoBracket;
  }
}

bool IsBracketPair(wchar_t bracket_l, wchar_t bracket_r) {
  if (bracket_l == L'(') {
    return bracket_r == L')';
  }
  if (bracket_l == L'{') {
    return bracket_r == L'}';
  }
  if (bracket_l == L'[') {
    return bracket_r == L']';
  }
  if (bracket_l == L'<') {
    return bracket_r == L'>';
  }
  if (bracket_l == L'"') {
    return bracket_r == L'"';
  }
  if (bracket_l == L'\'') {
    return bracket_r == L'\'';
  }
  return false;
}

typedef Bracket(*BracketFunc)(wchar_t);

// Helper function for finding unmatched bracket.
static bool CheckBracket(wchar_t c,
                         BracketFunc bf1,
                         BracketFunc bf2,
                         int counter[kBracketCount]) {
  Bracket bracket = bf1(c);
  if (bracket != kNoBracket) {
    if (counter[bracket] == 0) {
      return true;
    } else {
      --counter[bracket];
    }
  } else {
    bracket = bf2(c);
    if (bracket != kNoBracket) {
      ++counter[bracket];
    }
  }
  return false;
}

FileFormat CheckEol(const std::wstring& text, size_t* first_line_size) {
  const size_t text_size = text.size();

  FileFormat eol = FF_NONE;

  size_t i = 0;
  for (; i < text_size; ++i) {
    if (text[i] == CR) {
      if (i + 1 < text_size && text[i + 1] == LF) {
        eol = FF_WIN;
      } else {
        eol = FF_MAC;
      }
      break;
    } else if (text[i] == LF) {
      eol = FF_UNIX;
      break;
    }
  }

  if (first_line_size != NULL && eol != FF_NONE) {
    *first_line_size = i;
  }

  return eol;
}

#if JIL_SPLIT_LINES_WITH_FUNC
/* Usage:
std::wstring text(L"a\nb\n\nc\n");
size_t i = 0;
size_t count = 0;
size_t step = 0;
while (SplitLines(text, i, &count, &step)) {
  text.substr(i, count);
  i += step;
}
*/
bool SplitLines(const std::wstring& text,
                size_t i,
                size_t* count,
                size_t* step) {
  const size_t text_size = text.size();
  if (i >= text_size) {
    return false;
  }

  size_t k = i;
  for (; k < text_size; ++k) {
    if (text[k] == CR || text[k] == LF) {
      break;
    }
  }

  *count = k - i;
  *step = *count + 1;

  // If we break for '\r', the step need to take another '\n' into account.
  if (k + 1 < text_size) {
    if (text[k] == CR && text[k + 1] == LF) {
      ++(*step);
    }
  }
  return true;
}

#endif  // JIL_SPLIT_LINES_WITH_FUNC

}  // namespace

////////////////////////////////////////////////////////////////////////////////

// If wxWidgets pre-defines the conv, reuse it and don't delete it. E.g., reuse
// wxConvISO8859_1 instead of "new wxCSConv(wxFONTENCODING_ISO8859_1)".
static wxMBConv* GetCsConv(const Encoding& encoding,
                           const char** bom,
                           bool* need_delete) {
  *need_delete = true;
  wxMBConv* conv = NULL;

  switch (encoding.value) {
  case wxFONTENCODING_ISO8859_1:
    conv = &wxConvISO8859_1;
    *need_delete = false;
    break;

  case wxFONTENCODING_UTF8:
    conv = &wxConvUTF8;
    *need_delete = false;
    if (encoding.name == "utf-8 bom") {
      *bom = UTF_8_BOM_BYTES;
    }
    break;

  case wxFONTENCODING_UTF16BE:
    conv = new wxMBConvUTF16BE;
    *bom = UTF_16BE_BOM_BYTES;
    break;

  case wxFONTENCODING_UTF16LE:
    conv = new wxMBConvUTF16LE;
    *bom = UTF_16LE_BOM_BYTES;
    break;

  default:
    conv = new wxCSConv(encoding.value);
    break;
  }

  return conv;
}

////////////////////////////////////////////////////////////////////////////////

// Charset detector using uchardet from Mozilla.
// See http://mxr.mozilla.org/mozilla/source/extensions/universalchardet/src/
class CharDetector : public nsUniversalDetector {
 public:
  explicit CharDetector(int lang_filter)
      : nsUniversalDetector(lang_filter) {
  }

  virtual ~CharDetector() {
  }

  const std::string& charset() const { return charset_; }

  bool PureAscii() const { return mInputState == ePureAscii; }

 protected:
  virtual void Report(const char* charset) override {
    charset_ = charset;
    boost::algorithm::to_lower(charset_);
  }

 private:
  std::string charset_;
};

////////////////////////////////////////////////////////////////////////////////

wchar_t TextBuffer::CharIterator::operator*() const {
  TextLine* line = *y_;
  if (x_ >= line->Length()) {
    return LF;
  } else {
    return line->Char(x_);
  }
}

TextBuffer::CharIterator& TextBuffer::CharIterator::operator++() {
  TextLine* line = *y_;
  if (x_ < line->Length()) {
    ++x_;
  } else {
    ++y_;
    x_ = 0;
  }
  return *this;
}

TextBuffer::CharIterator& TextBuffer::CharIterator::operator--() {
  TextLine* line = *y_;
  if (x_ == 0) {
    --y_;
    x_ = (*y_)->Length();
  } else {
    --x_;
  }
  return *this;
}

TextBuffer::CharIterator TextBuffer::CharIterator::operator++(int) {
  CharIterator tmp = *this;
  ++*this;
  return tmp;
}

TextBuffer::CharIterator TextBuffer::CharIterator::operator--(int) {
  CharIterator tmp = *this;
  --*this;
  return tmp;
}

////////////////////////////////////////////////////////////////////////////////

//------------------------------------------------------------------------------

TextBuffer::~TextBuffer() {
  ClearActions();

  ClearContainer(&lines_);
  // Force vector to relinquish its memory (look up "STL swap trick").
  TextLines().swap(lines_);
}

//------------------------------------------------------------------------------

// static
TextBuffer* TextBuffer::Create(FtPlugin* ft_plugin,
                               const Encoding& file_encoding) {
  TextBuffer* buffer = new TextBuffer(ft_plugin);

  buffer->set_file_encoding(file_encoding);

  // A new file has an empty line.
  buffer->DoAppendLine(L"");

  return buffer;
}

// Read the bytes from a file, detect the encoding, then convert
// to std::wstring.
static FileError ReadFile(const wxString& file_path,
                          int cjk_filters,
                          std::wstring* text,
                          Encoding* encoding) {
  std::string bytes;
  int result = ReadBytes(file_path, &bytes);
  if (result != 0) {
    // IO Error or size limit is exceeded.
    return kIOError;
  }

  if (bytes.empty()) {
    return kEmptyError;  // Empty file
  }

  // Detect file encoding.
  CharDetector char_detector(cjk_filters);
  char_detector.HandleData(bytes.c_str(), bytes.size());
  char_detector.DataEnd();

  std::string encoding_name = char_detector.charset();
  if (encoding_name.empty() && !char_detector.PureAscii()) {
    // Failed to detect the file encoding!
    return kEncodingError;
  }

  *encoding = EncodingFromName(encoding_name);

  const char* bom = NULL;
  bool conv_need_delete = true;
  wxMBConv* conv = GetCsConv(*encoding, &bom, &conv_need_delete);
  if (conv == NULL) {
    return kEncodingError;  // Should never be here!
  }

  size_t bom_size = 0;
  if (bom != NULL) {
    if (bom == UTF_8_BOM_BYTES) {
      bom_size = 3;
    } else {
      bom_size = 2;
    }
  }

  // Get the size in wchar_t.
  size_t wlen = conv->ToWChar(NULL,
                              0,
                              bytes.c_str() + bom_size,
                              bytes.size() - bom_size);

  if (wlen == wxCONV_FAILED) {
    if (conv_need_delete) {
      delete conv;
      conv = NULL;
    }
    return kEncodingError;
  }

  if (wlen > 0) {
    text->resize(wlen);
    conv->ToWChar(&(*text)[0],
                  wlen,
                  bytes.c_str() + bom_size,
                  bytes.size() - bom_size);
    if ((*text)[wlen - 1] == L'\0') {
      text->erase(wlen - 1);
    }
  }

  if (conv_need_delete) {
    delete conv;
    conv = NULL;
  }

  return kNoError;
}

// static
TextBuffer* TextBuffer::Create(const wxFileName& file_name_object,
                               FtPlugin* ft_plugin,
                               int cjk_filters,
                               const Encoding& file_encoding) {
  std::wstring text;
  Encoding encoding;

  int error = ReadFile(file_name_object.GetFullPath(),
                       cjk_filters,
                       &text,
                       &encoding);

  if (error == kEmptyError) {
    // The file is empty.
    TextBuffer* buffer = Create(ft_plugin, file_encoding);
    buffer->set_file_name_object(file_name_object);
    return buffer;
  }

  if (error != kNoError) {
    return NULL;
  }

  TextBuffer* buffer = NULL;

  if (text.empty()) {
    // The file has only BOM bytes.
    buffer = Create(ft_plugin, encoding);
  } else {
    buffer = new TextBuffer(ft_plugin);
    buffer->set_file_encoding(encoding);
    buffer->SetText(text);
  }

  buffer->set_file_name_object(file_name_object);

  return buffer;
}

// static
TextBuffer* TextBuffer::Create(const std::wstring& text,
                               FtPlugin* ft_plugin,
                               const Encoding& file_encoding) {
  TextBuffer* buffer = NULL;
  if (text.empty()) {
    // The file is empty with only BOM bytes?
    buffer = Create(ft_plugin, file_encoding);
  } else {
    buffer = new TextBuffer(ft_plugin);
    buffer->set_file_encoding(file_encoding);
    buffer->SetText(text);
  }
  return buffer;
}

//------------------------------------------------------------------------------

FileError TextBuffer::SaveFile() {
  // VC++ needs "this->" if variable has the same name as the function.
  wxString file_path_name = this->file_path_name();
  assert(!file_path_name.empty());

  std::wstring text;
  GetText(&text);

  // Convert back to bytes with the original encoding.

  const char* new_file_encoding = NULL;

  const char* bom = NULL;
  bool conv_need_delete = true;
  wxMBConv* conv = GetCsConv(file_encoding_, &bom, &conv_need_delete);
  if (conv == NULL) {
    // Save in UTF-8 instead.
    conv = &wxConvUTF8;
    conv_need_delete = false;
    bom = NULL;
    new_file_encoding = "uft-8";
  }

  // Get the size in bytes.
  // FIXME: Cannot use std::auto_ptr! Strange!
  size_t count = conv->FromWChar(NULL, 0, text.c_str(), text.size());
  if (count == wxCONV_FAILED) {
    // Can't save in original encoding.
    // Example: Original encoding is ASCII, but non-ASCII characters are added.
    // Save in UTF-8 instead.
    if (conv_need_delete) {
      delete conv;
    }
    conv = &wxConvUTF8;
    conv_need_delete = false;

    // Get the size in bytes using UTF-8.
    count = conv->FromWChar(NULL, 0, text.c_str(), text.size());
    if (count == wxCONV_FAILED) {
      // Still failed!
      if (conv_need_delete) {
        delete conv;
        conv = NULL;
      }
      return kEncodingError;
    }

    bom = NULL;  // UTF-8 without BOM
    new_file_encoding = "uft-8";
  }

  // Allocate buffer for the bytes.
  std::string bytes;
  bytes.resize(count);

  // Convert.
  conv->FromWChar(&bytes[0], count, &text[0], text.size());
  if (bytes[count - 1] == '\0') {
    --count;
  }

  if (conv_need_delete) {
    delete conv;
    conv = NULL;
  }

  // Save.
  int save_result = SaveBytes(file_path_name, bom, bytes);
  if (save_result != 0) {
    return kIOError;
  }

  if (new_file_encoding != NULL) {
    file_encoding_ = EncodingFromName(new_file_encoding);
    Notify(kEncodingChange);
  }

  SaveUndoActions();

  Notify(kModifiedChange);

  return kNoError;
}

//------------------------------------------------------------------------------

TextBuffer::CharIterator TextBuffer::CharBegin() {
  assert(!lines_.empty());
  return CharIterator(lines_.begin(), 0);
}

const TextBuffer::CharIterator TextBuffer::CharBegin() const {
  assert(!lines_.empty());
  return CharIterator(lines_.begin(), 0);
}

TextBuffer::CharIterator TextBuffer::CharEnd() {
  assert(!lines_.empty());
  TextLines::iterator y = lines_.end();
  --y;
  return CharIterator(y, (*y)->Length());
}

const TextBuffer::CharIterator TextBuffer::CharEnd() const {
  assert(!lines_.empty());
  TextLines::const_iterator y = lines_.end();
  --y;
  return CharIterator(y, (*y)->Length());
}

//------------------------------------------------------------------------------

wxString TextBuffer::file_path_name() const {
  return file_name_object_.GetFullPath();
}

void TextBuffer::set_file_path_name(const wxString& file_path_name) {
  file_name_object_ = wxFileName(file_path_name);
}

wxString TextBuffer::file_name() const {
  return file_name_object_.GetFullName();
}

wxString TextBuffer::file_path(int flags, wxPathFormat format) const {
  return file_name_object_.GetPath(flags, format);
}

//------------------------------------------------------------------------------

bool TextBuffer::new_created() const {
  return file_name().empty();
}

bool TextBuffer::modified() const {
  if (saved_undo_actions_.size() != last_saved_undo_count_) {
    return true;
  }
  if (!undo_actions_.empty()) {
    return true;
  }
  if (!recent_ic_actions_.empty()) {
    return true;
  }
  return false;
}

bool TextBuffer::read_only() const {
  return read_only_;
}

bool TextBuffer::deleted() const {
  return deleted_;
}

//------------------------------------------------------------------------------

Coord TextBuffer::LineCount() const {
  return CoordCast(lines_.size());
}

TextLine* TextBuffer::Line(Coord ln) {
  assert(ln > 0 && ln <= LineCount());
  return lines_[ln - 1];
}

const TextLine* TextBuffer::Line(Coord ln) const {
  assert(ln > 0 && ln <= LineCount());
  return lines_[ln - 1];
}

const TextLine* TextBuffer::LineById(size_t id) const {
  TextLines::const_iterator it = lines_.begin();
  for (; it != lines_.end(); ++it) {
    if ((*it)->id() == id) {
      return *it;
    }
  }
  return NULL;
}

Coord TextBuffer::LineNrFromId(size_t id) const {
  TextLines::const_iterator it = lines_.begin();
  for (; it != lines_.end(); ++it) {
    if ((*it)->id() == id) {
      return CoordCast(std::distance(lines_.begin(), it)) + 1;
    }
  }
  return kInvalidCoord;
}

Coord TextBuffer::PrevNonEmptyLine(Coord ln,
                                   bool ignore_spaces,
                                   bool skip_comments) const {
  for (--ln; ln > 0; --ln) {
    const TextLine* line = Line(ln);

    // TODO: Simplify.
    if (ignore_spaces) {
      if (!line->IsEmpty(ignore_spaces)) {
        if (!skip_comments || !line->CommentsOnly()) {
          break;
        }
      }
    } else {
      if (!line->IsEmpty(ignore_spaces)) {
        if (line->SpacesOnly() || !skip_comments || !line->CommentsOnly()) {
          break;
        }
      }
    }
  }

  return ln;
}

Coord TextBuffer::PrevLine(Coord ln, const LinePred& pred) const {
  for (--ln; ln > 0 && !pred.Check(Line(ln)); --ln) {
  }
  return ln;
}

const std::wstring& TextBuffer::LineData(Coord ln) const {
  return Line(ln)->data();
}

wchar_t TextBuffer::Char(const TextPoint& point) const {
  assert(point.y > 0 && point.y <= LineCount());
  return Line(point.y)->Char(point.x);
}

size_t TextBuffer::LineId(Coord ln) const {
  return Line(ln)->id();
}

Coord TextBuffer::LineLength(Coord ln) const {
  return Line(ln)->Length();
}

bool TextBuffer::IsLineEmpty(Coord ln, bool ignore_spaces) const {
  return Line(ln)->IsEmpty(ignore_spaces);
}

bool TextBuffer::AreLinesAllEmpty(const LineRange& line_range,
                                  bool ignore_spaces) const {
  for (Coord ln = line_range.first(); ln <= line_range.last(); ++ln) {
    if (!IsLineEmpty(ln, ignore_spaces)) {
      return false;
    }
  }
  return true;
}

void TextBuffer::GetText(std::wstring* text) const {
  std::wstring eol = GetEol(file_format_);
  *text = lines_[0]->data();
  for (size_t i = 1; i < lines_.size(); ++i) {
    *text += eol;
    *text += lines_[i]->data();
  }
}

void TextBuffer::GetText(const TextRange& range, std::wstring* text) const {
  const TextPoint& point_begin = range.point_begin();
  const TextPoint& point_end = range.point_end();

  if (point_begin.y == point_end.y) {
    *text = Line(point_begin.y)->Sub(point_begin.x,
                                     point_end.x - point_begin.x);
    return;
  }

  // First line.
  *text = Line(point_begin.y)->Sub(point_begin.x, kInvalidCoord);
  *text += LF;

  // Middle lines.
  for (Coord ln = point_begin.y + 1; ln < point_end.y; ++ln) {
    *text += LineData(ln) + LF;
  }

  // Last line.
  if (point_end.x > 0) {
    *text += Line(point_end.y)->Sub(0, point_end.x);
  }
}

void TextBuffer::GetRectText(const TextRange& range, std::wstring* text) const {
  if (range.IsEmpty()) {
    return;
  }

  CharRange char_range = range.GetCharRange();
  if (char_range.IsEmpty()) {
    return;
  }

  LineRange line_range = range.GetLineRange();

  for (Coord ln = line_range.first(); ln < line_range.last(); ++ln) {
    *text += Line(ln)->Sub(char_range) + LF;
  }

  *text += Line(line_range.last())->Sub(char_range);
}

TextPoint TextBuffer::InsertChar(const TextPoint& point, wchar_t c) {
  TextLine* line = Line(point.y);

  if (c != LF) {
    RemoveLineLength(line);
    line->InsertChar(point.x, c);
    AddLineLength(line);

    if (!notify_frozen_) {
      Notify(kLineUpdated, LineChangeData(point.y));
    }

    return TextPoint(point.x + 1, point.y);
  } else {
    TextLine* new_line = NULL;

    if (point.x == line->Length()) {
      new_line = new TextLine(NewLineId(), L"");
    } else {
      RemoveLineLength(line);
      new_line = line->Split(point.x, NewLineId());
      AddLineLength(line);

      if (!notify_frozen_) {
        Notify(kLineUpdated, LineChangeData(point.y));
      }
    }

    DoInsertLine(point.y, new_line);

    if (!notify_frozen_) {
      Notify(kLineAdded, LineChangeData(point.y + 1));
    }

    return TextPoint(0, point.y + 1);
  }
}

void TextBuffer::DeleteChar(const TextPoint& point, wchar_t* c) {
  assert(point.x <= LineLength(point.y));

  if (point.x == LineLength(point.y)) {
    // Delete the line ending.
    if (point.y < LineCount()) {
      TextLine* line = Line(point.y);
      TextLine* next_line = Line(point.y + 1);

      RemoveLineLength(next_line);

      if (next_line->Length() > 0) {
        RemoveLineLength(line);
        line->Append(next_line->data());
        AddLineLength(line);

        if (!notify_frozen_) {
          Notify(kLineUpdated, LineChangeData(point.y));
        }
      }

      lines_.erase(lines_.begin() + point.y);

      if (!notify_frozen_) {
        Notify(kLineDeleted, LineChangeData(point.y + 1));
      }

      if (c != NULL) {
        *c = LF;
      }
    } else {
      // Don't delete the line ending of the last line.
      if (c != NULL) {
        *c = kNilChar;
      }
    }
  } else {
    TextLine* line = Line(point.y);

    RemoveLineLength(line);
    line->DeleteChar(point.x, c);
    AddLineLength(line);

    if (!notify_frozen_) {
      Notify(kLineUpdated, LineChangeData(point.y));
    }
  }
}

TextPoint TextBuffer::InsertString(const TextPoint& point,
                                   const std::wstring& str) {
  TextLine* line = Line(point.y);

  RemoveLineLength(line);
  line->InsertString(point.x, str);
  AddLineLength(line);

  if (!notify_frozen_) {
    Notify(kLineUpdated, LineChangeData(point.y));
  }

  return TextPoint(point.x + str.size(), point.y);
}

void TextBuffer::DeleteString(const TextPoint& point,
                              Coord count,
                              std::wstring* str) {
  TextLine* line = Line(point.y);

  RemoveLineLength(line);
  line->DeleteString(point.x, count, str);
  AddLineLength(line);

  if (!notify_frozen_) {
    Notify(kLineUpdated, LineChangeData(point.y));
  }
}

void TextBuffer::InsertLine(Coord ln, const std::wstring& line_data) {
  assert(ln > 0 && ln <= LineCount() + 1);

  DoInsertLine(ln - 1, new TextLine(NewLineId(), line_data));

  if (!notify_frozen_) {
    Notify(kLineAdded, LineChangeData(ln));
  }
}

TextLine* TextBuffer::AppendLine(const std::wstring& line_data) {
  TextLine* line = new TextLine(NewLineId(), line_data);
  lines_.push_back(line);

  AddLineLength(line);

  if (!notify_frozen_) {
    Notify(kLineAdded, LineChangeData(LineCount()));
  }

  return line;
}

void TextBuffer::DeleteLine(Coord ln, std::wstring* line_data) {
  assert(ln > 0 && ln <= LineCount());

  if (LineCount() == 1) {
    // Only one line left, just clear it.
    TextLine* line = Line(ln);

    RemoveLineLength(line);
    line->Clear(line_data);
    AddLineLength(line);

    if (!notify_frozen_) {
      Notify(kLineUpdated, LineChangeData(ln));
    }
  } else {
    TextLines::iterator it(lines_.begin());
    std::advance(it, ln - 1);

    if (line_data != NULL) {
      *line_data = (*it)->data();
    }

    RemoveLineLength(*it);

    lines_.erase(it);

    if (!notify_frozen_) {
      Notify(kLineDeleted, LineChangeData(ln));
    }
  }
}

TextPoint TextBuffer::InsertText(const TextPoint& point,
                                 const std::wstring& text) {
  // Avoid to notify on every change, notify at last as few as possible.
  FreezeNotify();

  size_t line_count = 0;
  TextPoint insert_point = point;
  size_t p = 0;
  for (size_t i = 0; i < text.size(); ++i) {
    if (text[i] == LF) {
      insert_point = InsertString(insert_point, text.substr(p, i - p));
      insert_point = InsertChar(insert_point, LF);
      p = i + 1;
      ++line_count;
    }
  }

  if (p < text.size()) {
    insert_point = InsertString(insert_point, text.substr(p, text.size() - p));
  }

  ThawNotify();

  // Notify now.
  // Note (2013-04-20): Notify LineAdded first so that wrap infos can be
  // updated before LineUpdated.
  if (line_count > 0) {
    Notify(kLineAdded, LineChangeData(point.y + 1, point.y + line_count));
  }
  Notify(kLineUpdated, LineChangeData(point.y));

  return insert_point;
}

TextPoint TextBuffer::InsertRectText(const TextPoint& point,
                                     const std::wstring& text) {
  FreezeNotify();

  size_t line_count = 0;
  TextPoint insert_point = point;

  size_t p = 0;
  for (size_t i = 0; i < text.size(); ++i) {
    if (text[i] == LF) {
      InsertString(insert_point, text.substr(p, i - p));
      ++insert_point.y;
      p = i + 1;
      ++line_count;
    }
  }

  if (p < text.size()) {
    InsertString(insert_point, text.substr(p, text.size() - p));
  }

  ThawNotify();

  // Notify now.
  if (line_count > 0) {
    Notify(kLineUpdated, LineChangeData(point.y, point.y + line_count));
  }

  return point;
}

void TextBuffer::DeleteText(const TextRange& range, std::wstring* text) {
  if (range.point_begin().y == range.point_end().y) {
    DeleteString(range.point_begin(),
                 range.point_end().x - range.point_begin().x,
                 text);
    return;
  }

  FreezeNotify();

  // First line.
  DeleteString(range.point_begin(), kInvalidCoord, text);
  if (text != NULL) {
    *text += LF;
  }

  // Middle lines.
  for (Coord y = range.point_begin().y + 1; y < range.point_end().y; ++y) {
    if (text != NULL) {
      std::wstring line_data;
      DeleteLine(range.point_begin().y + 1, &line_data);
      *text += line_data + LF;
    } else {
      DeleteLine(range.point_begin().y + 1);
    }
  }

  // Last line.
  if (range.point_end().x > 0) {
    if (text != NULL) {
      std::wstring str;
      DeleteString(TextPoint(0, range.point_begin().y + 1),
                   range.point_end().x,
                   &str);
      *text += str;
    } else {
      DeleteString(TextPoint(0, range.point_begin().y + 1),
                   range.point_end().x);
    }
  }

  // Delete the line ending of first line.
  DeleteChar(TextPoint(LineLength(range.point_begin().y),
                       range.point_begin().y));

  ThawNotify();

  // Avoid to notify on every single change.
  // NOTE (2013-04-20): Notify LineDeleted first so that wrap infos can be
  // updated before LineUpdated.
  Notify(kLineDeleted,
         LineChangeData(range.point_begin().y + 1, range.point_end().y));
  Notify(kLineUpdated, LineChangeData(range.point_begin().y));
}

void TextBuffer::DeleteRectText(const TextRange& range, std::wstring* text) {
  if (range.IsEmpty()) {
    return;
  }

  CharRange char_range = range.GetCharRange();
  if (char_range.IsEmpty()) {
    return;
  }

  FreezeNotify();

  LineRange line_range = range.GetLineRange();

  for (Coord ln = line_range.first(); ln <= line_range.last(); ++ln) {
    TextPoint point(char_range.begin(), ln);

    if (text == NULL) {
      DeleteString(point, char_range.CharCount());
    } else {
      std::wstring str;
      DeleteString(point, char_range.CharCount(), &str);

      if (!text->empty()) {
        *text += LF;
      }
      *text += str;
    }
  }

  ThawNotify();

  Notify(kLineUpdated, LineChangeData(line_range));
}

//------------------------------------------------------------------------------

TextRange TextBuffer::FindString(const std::wstring& str,
                                 const TextRange& range,
                                 bool use_regex,
                                 bool case_sensitive,
                                 bool match_whole_word,
                                 bool reversely) const {
  assert(!str.empty());
  assert(range.point_begin().y <= LineCount());

  if (use_regex) {
    return FindRegexString(str, range, case_sensitive, match_whole_word);
  } else {
    if (reversely) {
      return FindPlainStringReversely(str,
                                      range,
                                      case_sensitive,
                                      match_whole_word);
    } else {
      return FindPlainString(str, range, case_sensitive, match_whole_word);
    }
  }
}

void TextBuffer::FindStringAll(const std::wstring& str,
                               const TextRange& range,
                               bool use_regex,
                               bool case_sensitive,
                               bool match_whole_word,
                               std::list<TextRange>* result_ranges) const {
  assert(!str.empty());
  assert(range.point_begin().y <= LineCount());

  if (!use_regex) {
    FindPlainStringAll(str,
                       range,
                       case_sensitive,
                       match_whole_word,
                       result_ranges);
  } else {
    FindRegexStringAll(str,
                       range,
                       case_sensitive,
                       match_whole_word,
                       result_ranges);
  }
}

//------------------------------------------------------------------------------

void TextBuffer::AttachListener(BufferListener* listener) {
  listeners_.push_back(listener);
}

void TextBuffer::DetachListener(BufferListener* listener) {
  std::remove(listeners_.begin(), listeners_.end(), listener);
}

void TextBuffer::Notify(LineChangeType type, const LineChangeData& data) {
  // Scan lex on change.
  if (ft_plugin_->IsLexAvailable()) {
    if (type == kLineUpdated) {
      ScanLexOnLineUpdated(data);
    } else if (type == kLineAdded) {
      ScanLexOnLineAdded(data);
    } else if (type == kLineDeleted) {
      ScanLexOnLineDeleted(data);
    }
  }

  for (size_t i = 0; i < listeners_.size(); ++i) {
    listeners_[i]->OnBufferLineChange(type, data);
  }
}

void TextBuffer::Notify(ChangeType type) {
  for (size_t i = 0; i < listeners_.size(); ++i) {
    listeners_[i]->OnBufferChange(type);
  }
}

void TextBuffer::FreezeNotify() {
  notify_frozen_ = true;
}

void TextBuffer::ThawNotify() {
  notify_frozen_ = false;
}

//------------------------------------------------------------------------------

Coord TextBuffer::GetIndent(Coord ln) const {
  return Line(ln)->GetIndent(options_.tab_stop);
}

std::wstring TextBuffer::GetIndentStr(Coord ln) const {
  return Line(ln)->GetIndentStr();
}

Coord TextBuffer::GetExpectedIndent(Coord ln) const {
  IndentFunc indent_func = ft_plugin_->indent_func();
  return indent_func(this, ln);
}

//------------------------------------------------------------------------------

// TODO
//TextPoint TextBuffer::MatchBracket(const TextPoint& point) const {
//  const TextLine* line = Line(point.y);
//  Bracket bracket = BracketL(line->Char(point.x));
//  if (bracket != kNoBracket) {  // ( { [ <
//    return UnmatchedBracketR(point, bracket);
//  } else {
//    bracket = BracketR(line->Char(point.x));
//    if (bracket != kNoBracket) {  // ) } ] >
//    }
//  }
//  /*
//  TextPoint open_point = point;
//
//  const std::wstring& line_data = LineData(point.y);
//  Coord x = point.x - 1;
//  for (; x >= 0; --x) {
//    if (line_data[x] == open) {
//      return TextPoint(x, point.y);
//    }
//  }
//
//  for (Coord y = point.y - 1; y > 0; --y) {
//    const std::wstring& line_data = LineData(y);
//    Coord x = LineLength(y) - 1;
//    for (; x >= 0; --x) {
//      if (line_data[x] == open) {
//        return TextPoint(x, y);
//      }
//    }
//  }*/
//
//  return kInvalidPoint;
//}

TextRange TextBuffer::BracketPairOuterRange(const TextPoint& point) const {
  TextPoint point_l = UnmatchedBracketL(point);
  if (!point_l.Valid()) {
    return TextRange();
  }

  wchar_t bracket_l = Char(point_l);

  TextPoint point_r = point;
  while (true) {
    point_r = UnmatchedBracketR(point_r);
    if (!point_r.Valid()) {
      return TextRange();
    }
    if (IsBracketPair(bracket_l, Char(point_r))) {
      break;
    } else {
      ++point_r.x;
    }
  }

  ++point_r.x;
  return TextRange(point_l, point_r);
}

TextRange TextBuffer::BracketPairInnerRange(const TextPoint& point) const {
  TextRange outer_range = BracketPairOuterRange(point);
  if (outer_range.IsEmpty()) {
    return TextRange();
  }

  TextPoint point_begin = outer_range.point_begin();
  TextPoint point_end = outer_range.point_end();

  ++point_begin.x;
  if (point_end.x > 0) {
    --point_end.x;
  } else {
    --point_end.y;
    point_end.x = LineLength(point_end.y);
  }

  return TextRange(point_begin, point_end);
}

bool TextBuffer::IsBracketPairInnerRange(const TextRange& range) const {
  TextPoint point_begin = range.point_begin();
  TextPoint point_end = range.point_end();

  if (point_begin.x > 0) {
    --point_begin.x;
  } else {
    if (point_begin.y > 1) {
      --point_begin.y;
      point_begin.x = LineLength(point_begin.y);
    } else {
      return false;
    }
  }
  if (!IsBracketL(Char(point_begin))) {
    return false;
  }

  // Don't have to increase point last_point().
  return IsBracketPair(Char(point_begin), Char(point_end));
}

TextRange TextBuffer::IncreaseRange(const TextRange& range) const {
  TextPoint point_begin = range.point_begin();
  TextPoint point_end = range.point_end();

  if (point_begin.x > 0) {
    --point_begin.x;
  } else {
    if (point_begin.y > 1) {
      --point_begin.y;
      point_begin.x = LineLength(point_begin.y);
    } else {
      return range;
    }
  }

  const Coord end_line_length = LineLength(point_end.y);
  if (point_end.x < end_line_length) {
    ++point_end.x;
  } else {
    if (point_end.y < LineCount()) {
      ++point_end.y;
    } else {
      return range;
    }
  }

  return TextRange(point_begin, point_end);
}

TextPoint TextBuffer::UnpairedLeftKey(const TextPoint& point,
                                      wchar_t l_key,
                                      wchar_t r_key,
                                      bool single_line) const {
  int counter = 0;

  // Find in the current line.
  Coord y = point.y;
  const TextLine* line = Line(y);

  for (Coord x = point.x - 1; x >= 0; --x) {
    if (line->Char(x) == r_key) {  // Embedded pair.
      ++counter;
    } else if (line->Char(x) == l_key) {
      if (counter == 0) {
        return TextPoint(x, y);
      } else {
        --counter;
      }
    }
  }

  // Find in previous lines.
  if (!single_line) {
    for (--y; y > 0; --y) {
      const TextLine* line = Line(y);
      for (Coord x = line->Length() - 1; x >= 0; --x) {
        if (line->Char(x) == r_key) {  // Embedded pair.
          ++counter;
        } else if (line->Char(x) == l_key) {
          if (counter == 0) {
            return TextPoint(x, y);
          } else {
            --counter;
          }
        }
      }
    }
  }

  return kInvalidPoint;
}

// TODO
TextPoint TextBuffer::UnpairedRightKey(const TextPoint& point,
                                       wchar_t l_key,
                                       wchar_t r_key) const {
  return kInvalidPoint;
}

//----------------------------------------------------------------------------
// Seek

TextPoint TextBuffer::Seek(const TextPoint& point,
                           TextUnit text_unit,
                           SeekType seek_type) {
  switch (text_unit) {
  case kChar:
    if (seek_type == kPrev) {
      return SeekPrevChar(point);
    } else if (seek_type == kNext) {
      return SeekNextChar(point);
    }

    break;

  case kWord:
    if (seek_type == kBegin) {
      return SeekWordBegin(point);
    } else if (seek_type == kEnd) {
      return SeekWordEnd(point);
    } else if (seek_type == kPrev) {
      return SeekPrevWord(point);
    } else if (seek_type == kNext) {
      return SeekNextWord(point);
    }

    break;

  case kLine:
    if (seek_type == kBegin) {
      return SeekLineBegin(point);
    } else if (seek_type == kEnd) {
      return SeekLineEnd(point);
    } else if (seek_type == kPrev) {
      return SeekPrevLine(point);
    } else if (seek_type == kNext) {
      return SeekNextLine(point);
    }

    break;

  case kBuffer:
    if (seek_type == kBegin) {
      return point_begin();
    } else if (seek_type == kEnd) {
      return point_end();
    }
    break;
  }

  return point;
}

//----------------------------------------------------------------------------
// Action operations, undo/redo.

static bool MergeDeleteActions(DeleteAction* delete_action,
                               Action* prev_action) {
  if (prev_action == NULL) {
    return false;
  }

  DeleteAction* prev_delete_action = dynamic_cast<DeleteAction*>(prev_action);
  if (prev_delete_action == NULL) {
    return false;
  }

  if (!prev_delete_action->SameKind(*delete_action)) {
    return false;
  }

  if (prev_delete_action->CaretPointAfterExec() !=
      delete_action->caret_point()) {
    return false;  // Not continuous.
  }

  // Check time interval.
  wxTimeSpan max_interval = wxTimeSpan::Seconds(1);
  wxDateTime prev_timestamp = prev_delete_action->timestamp();
  prev_timestamp = prev_timestamp.Add(max_interval);
  if (prev_timestamp.IsEarlierThan(delete_action->timestamp())) {
    return false;
  }

  // Merge them.
  return prev_delete_action->Merge(delete_action);
}

Action* TextBuffer::AddAction(Action* action) {
  bool modified_backup = modified();

  MergeInsertCharActions();

  action->Exec();

  if (!action->effective()) {
    // The action doesn't change anything.
    delete action;
    return NULL;
  }

  ClearContainer(&redo_actions_);

  DeleteAction* delete_action = dynamic_cast<DeleteAction*>(action);
  if (delete_action != NULL) {
    if (MergeDeleteActions(delete_action, TopUndoAction())) {
      delete delete_action;
      return TopUndoAction();
    }
  }

  PushUndoAction(action);

  if (modified() != modified_backup) {
    Notify(kModifiedChange);
  }

  return action;
}

void TextBuffer::AddInsertCharAction(InsertCharAction* insert_char_action) {
  bool modified_backup = modified();

  ClearContainer(&redo_actions_);

  if (insert_char_action->c() == LF ||
      (!recent_ic_actions_.empty() &&
       recent_ic_actions_.back()->dir() != insert_char_action->dir())) {
    MergeInsertCharActions();
    PushUndoAction(insert_char_action);
    insert_char_action->Exec();
  } else {
    bool modified_backup = modified();

    if (!recent_ic_actions_.empty()) {
      InsertCharAction* prev_insert_char_action = recent_ic_actions_.back();

      TextPoint prev_point = prev_insert_char_action->point();
      TextPoint point = insert_char_action->point();

      if (prev_point.y != point.y || prev_point.x + 1 != point.x) {
        MergeInsertCharActions();
      }
    }

    insert_char_action->Exec();
    recent_ic_actions_.push_back(insert_char_action);
  }

  if (modified() != modified_backup) {
    Notify(kModifiedChange);
  }
}

Action* TextBuffer::Undo() {
  bool modified_backup = modified();

  Action* action = PopUndoAction();
  if (action != NULL) {
    action->Undo();
    PushRedoAction(action);
  }

  if (modified() != modified_backup) {
    Notify(kModifiedChange);
  }

  return action;
}

Action* TextBuffer::Redo() {
  if (!CanRedo()) {
    return NULL;
  }

  bool modified_backup = modified();

  Action* action = redo_actions_.front();
  redo_actions_.pop_front();
  PushUndoAction(action);
  action->Exec();

  if (modified() != modified_backup) {
    Notify(kModifiedChange);
  }

  return action;
}

bool TextBuffer::CanUndo() const {
  return !undo_actions_.empty() ||
         !saved_undo_actions_.empty() ||
         !recent_ic_actions_.empty();
}

bool TextBuffer::CanRedo() const {
  return !redo_actions_.empty();
}

//------------------------------------------------------------------------------

Coord TextBuffer::GetMaxLineLength() const {
  if (!long_line_length_map_.empty()) {
    return CoordCast((--long_line_length_map_.end())->first);
  }
  size_t i = kLongLineBoundary;
  while (i > 0 && line_length_table_[i] == 0) {
    --i;
  }
  return CoordCast(i);
}

//------------------------------------------------------------------------------

TextBuffer::TextBuffer(FtPlugin* ft_plugin)
    : ft_plugin_(ft_plugin)
    , options_(ft_plugin->options())
    , file_format_(FF_DEFAULT)
    , read_only_(false)
    , deleted_(false)
    , line_id_(0)
    , notify_frozen_(false)
    , last_saved_undo_count_(0) {
  ClearLineLength();
}

// NOTE: Don't have to notify since there shouldn't be any listeners when
// SetText() is called.
void TextBuffer::SetText(const std::wstring& text) {
#if JIL_TEST_TIME_SET_TEXT
  TimeIt time_it(wxT("SetText"));
  time_it.Start();
#endif  // JIL_TEST_TIME_SET_TEXT

  lines_.clear();
  ClearLineLength();

  size_t i = 0;
  size_t first_line_size = 0;
  FileFormat ff = CheckEol(text, &first_line_size);
  if (ff != FF_NONE) {
    // The file has more than one line.
    // Add the first line here, add the left lines later.
    DoAppendLine(&text[0], first_line_size);  // Avoid copy via substr()
    i += first_line_size;  // Skip the first line.
    i += ff == FF_WIN ? 2 : 1;  // Skip the EOL

    // Overwrite the default file format.
    file_format_ = ff;
  }

#if JIL_SPLIT_LINES_WITH_FUNC
  size_t count = 0;
  size_t step = 0;
  while (SplitLines(text, i, &count, &step)) {
    DoAppendLine(&text[i], count);
    i += step;
  }

#else
  // A little faster.
  if (i < text.size()) {
    const wchar_t* p = &text[i];

    while (*p != kNilChar) {
      const wchar_t* k = p;

      // Find next EOL.
      for (; *k != kNilChar; ++k) {
        if (*k == LF) {
          DoAppendLine(p, k - p);
          break;
        }
        if (*k == CR) {
          DoAppendLine(p, k - p);
          if (*(k + 1) == LF) {
            ++k;
          }
          break;
        }
      }

      if (*k == kNilChar) {
        // No next EOL.
        DoAppendLine(p, k - p);
        break;
      }

      p = k + 1;
    }
  }
#endif  // JIL_SPLIT_LINES_WITH_FUNC

  // If text ends with EOL, append an extra empty line.
  wchar_t last_char = text[text.size() - 1];
  if (last_char == LF || last_char == CR) {
    DoAppendLine(L"");
  }

#if JIL_TEST_TIME_SET_TEXT
  time_it.End();
  DoAppendLine(time_it.result_msg().ToStdWstring());
#endif  // JIL_TEST_TIME_SET_TEXT

  if (ft_plugin_->IsLexAvailable()) {
#if JIL_TEST_TIME_SCAN_LEX
    TimeIt time_it(wxT("Scan lex on text load."));
    time_it.Start();
#endif  // JIL_TEST_TIME_SCAN_LEX

    // Scan lex for the whole buffer.
    ScanLex();

#if JIL_TEST_TIME_SCAN_LEX
    time_it.End();
    DoAppendLine(time_it.result_msg().ToStdWstring());
#endif  // JIL_TEST_TIME_SCAN_LEX
  }
}

//------------------------------------------------------------------------------
// Find

static void SetFindRegex(const std::wstring& str,
                         bool case_sensitive,
                         bool match_whole_word,
                         boost::wregex* e) {
  int flag = case_sensitive ? 0 : boost::regex_constants::icase;

  if (match_whole_word) {
    const std::wstring kBoundary = L"\\b";

    std::wstring adjusted_str = str;

    if (!boost::starts_with(str, kBoundary)) {
      adjusted_str = kBoundary + adjusted_str;
    }

    if (!boost::ends_with(str, kBoundary)) {
      adjusted_str += kBoundary;
    }

    e->set_expression(adjusted_str, flag);
  } else {
    e->set_expression(str, flag);
  }
}

TextRange TextBuffer::FindPlainString(const std::wstring& str,
                                      const TextRange& range,
                                      bool case_sensitive,
                                      bool match_whole_word) const {
  TextRange result_range;

  TextLines::const_iterator it = lines_.begin();
  std::advance(it, range.point_begin().y - 1);

  // The range contains only one line.
  if (range.LineCount() == 1) {
    FindLineString(it,
                   range.point_begin().x,
                   range.point_end().x,
                   str,
                   case_sensitive,
                   false,
                   match_whole_word,
                   &result_range);
    return result_range;
  }

  // Find in the first line of the range.
  if (FindLineString(it,
                     range.point_begin().x,
                     kInvalidCoord,
                     str,
                     case_sensitive,
                     false,
                     match_whole_word,
                     &result_range)) {
    return result_range;
  }

  TextLines::const_iterator end_it = lines_.begin();
  std::advance(end_it, range.point_end().y - 1);

  // Find in the middle lines of the range.
  for (++it; it != end_it; ++it) {
    if (FindLineString(it,
                       0,
                       kInvalidCoord,
                       str,
                       case_sensitive,
                       false,
                       match_whole_word,
                       &result_range)) {
      return result_range;
    }
  }

  // Find in the last line of the range.
  FindLineString(it,
                 0,
                 range.point_end().x,
                 str,
                 case_sensitive,
                 false,
                 match_whole_word,
                 &result_range);
  return result_range;
}

TextRange TextBuffer::FindPlainStringReversely(const std::wstring& str,
                                               const TextRange& range,
                                               bool case_sensitive,
                                               bool match_whole_word) const {
  TextRange result_range;

  TextLines::const_iterator it = lines_.begin();
  std::advance(it, range.point_end().y - 1);

  // The range contains only one line.
  if (range.LineCount() == 1) {
    FindLineString(it,
                   range.point_begin().x,
                   range.point_end().x,
                   str,
                   case_sensitive,
                   true,
                   match_whole_word,
                   &result_range);
    return result_range;
  }

  // Find in the last line of the range.
  if (FindLineString(it,
                     0,
                     range.point_end().x,
                     str,
                     case_sensitive,
                     true,
                     match_whole_word,
                     &result_range)) {
    return result_range;
  }

  TextLines::const_iterator begin_it = lines_.begin();
  std::advance(begin_it, range.point_begin().y - 1);

  // Find in the middle lines of the range.
  for (--it; it != begin_it; --it) {
    if (FindLineString(it,
                       0,
                       kInvalidCoord,
                       str,
                       case_sensitive,
                       true,
                       match_whole_word,
                       &result_range)) {
      return result_range;
    }
  }

  // Find in the first line of the range.
  FindLineString(it,
                 range.point_begin().x,
                 kInvalidCoord,
                 str,
                 case_sensitive,
                 true,
                 match_whole_word,
                 &result_range);

  return result_range;
}

TextRange TextBuffer::FindRegexString(const std::wstring& str,
                                      const TextRange& range,
                                      bool case_sensitive,
                                      bool match_whole_word) const {
  boost::wregex e;
  SetFindRegex(str, case_sensitive, match_whole_word, &e);

  boost::match_results<CharIterator> m;
  boost::match_flag_type flags = boost::match_default | boost::match_partial;

  CharIterator c_begin(CharIteratorFromPoint(range.point_begin()));
  CharIterator c_end(CharEnd());

  try {
    if (boost::regex_search(c_begin, c_end, m, e, flags)) {
      return TextRange(PointFromCharIterator(m[0].first),
                       PointFromCharIterator(m[0].second));
    }
  } catch (std::exception&) {  // Don't use boost::regex_error!
    // Invalid regular expression!
  }

  return TextRange();
}

void TextBuffer::FindPlainStringAll(const std::wstring& str,
                                    const TextRange& range,
                                    bool case_sensitive,
                                    bool match_whole_word,
                                    std::list<TextRange>* result_ranges) const {
  TextLines::const_iterator it = lines_.begin();
  std::advance(it, range.point_begin().y - 1);

  // The range contains only one line.
  if (range.LineCount() == 1) {
    FindLineStringAll(it,
                      range.point_begin().x,
                      range.point_end().x,
                      str,
                      case_sensitive,
                      match_whole_word,
                      result_ranges);
    return;
  }

  // Find in the first line of the range.
  FindLineStringAll(it,
                    range.point_begin().x,
                    kInvalidCoord,
                    str,
                    case_sensitive,
                    match_whole_word,
                    result_ranges);

  TextLines::const_iterator end_it = lines_.begin();
  std::advance(end_it, range.point_end().y - 1);

  // Find in the middle lines of the range.
  for (++it; it != end_it; ++it) {
    FindLineStringAll(it,
                      0,
                      kInvalidCoord,
                      str,
                      case_sensitive,
                      match_whole_word,
                      result_ranges);
  }

  // Find in the last line of the range.
  FindLineStringAll(it,
                    0,
                    range.point_end().x,
                    str,
                    case_sensitive,
                    match_whole_word,
                    result_ranges);
}

void TextBuffer::FindRegexStringAll(const std::wstring& str,
                                    const TextRange& range,
                                    bool case_sensitive,
                                    bool match_whole_word,
                                    std::list<TextRange>* result_ranges) const {
  boost::wregex e;
  SetFindRegex(str, case_sensitive, match_whole_word, &e);

  boost::match_results<CharIterator> m;
  boost::match_flag_type flags = boost::match_default | boost::match_partial;

  CharIterator c_begin = CharIteratorFromPoint(range.point_begin());
  CharIterator c_end = CharEnd();

  try {
    while (true) {
      if (!boost::regex_search(c_begin, c_end, m, e, flags)) {
        break;
      }

      TextPoint point_begin = PointFromCharIterator(m[0].first);
      TextPoint point_end = PointFromCharIterator(m[0].second);

      result_ranges->push_back(TextRange(point_begin, point_end));

      c_begin = m[0].second;
    }
  } catch (std::exception&) {  // Don't use boost::regex_error!
    // Invalid regular expression!
  }
}

bool TextBuffer::FindLineString(TextLines::const_iterator line_it,
                                Coord x_begin,
                                Coord x_end,
                                const std::wstring& str,
                                bool case_sensitive,
                                bool reversely,
                                bool match_whole_word,
                                TextRange* result_range) const {
  const TextLine* line = *line_it;

  if (x_end == kInvalidCoord) {
    x_end = line->Length();
  }

  CmpFunc cmp = case_sensitive ? wcsncmp : wcsnicmp;

  Coord pos = kInvalidCoord;

  if (!reversely) {
    pos = StringFind(line->data(), x_begin, x_end, str, match_whole_word, cmp);
  } else {
    pos = StringFindReversely(line->data(),
                              x_begin,
                              x_end,
                              str,
                              match_whole_word,
                              cmp);
  }

  if (pos == kInvalidCoord) {
    return false;
  }

  Coord x = pos;
  Coord y = CoordCast(std::distance(lines_.begin(), line_it)) + 1;
  result_range->Set(TextPoint(x, y), TextPoint(x + str.size(), y));

  return true;
}

void TextBuffer::FindLineStringAll(TextLines::const_iterator line_it,
                                   Coord x_begin,
                                   Coord x_end,
                                   const std::wstring& str,
                                   bool case_sensitive,
                                   bool match_whole_word,
                                   std::list<TextRange>* result_ranges) const {
  const TextLine* line = *line_it;

  if (x_end == kInvalidCoord) {
    x_end = line->Length();
  }

  CmpFunc cmp = case_sensitive ? wcsncmp : wcsnicmp;

  while (x_begin < x_end) {
    Coord pos = StringFind(line->data(),
                           x_begin,
                           x_end,
                           str,
                           match_whole_word,
                           cmp);
    if (pos == kInvalidCoord) {
      break;
    }

    Coord str_size = CoordCast(str.size());

    Coord x = pos;
    Coord y = CoordCast(std::distance(lines_.begin(), line_it)) + 1;
    result_ranges->push_back(TextRange(TextPoint(x, y),
                                       TextPoint(x + str_size, y)));

    x_begin = x + str_size;
  }
}

Coord TextBuffer::StringFind(const std::wstring& str,
                             Coord begin,
                             Coord end,
                             const std::wstring& sub,
                             bool match_whole_word,
                             CmpFunc cmp) const {
  Coord sub_size = CoordCast(sub.size());
  if (sub_size == 0 || sub_size > (end - begin)) {
    return kInvalidCoord;
  }

  Coord j = end - sub_size + 1;
  for (Coord i = begin; i < j; ++i) {
    if (cmp(&str[i], &sub[0], sub_size) == 0) {
      if (!match_whole_word) {
        return i;
      }

      if (!ft_plugin_->IsDelimiter(sub[0])) {
        if (i > 0 && !ft_plugin_->IsDelimiter(str[i - 1])) {
          continue;
        }
      }

      if (!ft_plugin_->IsDelimiter(sub[sub.size() - 1])) {
        Coord k = i + sub_size;
        if (k < CoordCast(str.size()) && !ft_plugin_->IsDelimiter(str[k])) {
          continue;
        }
      }

      return i;
    }
  }

  return kInvalidCoord;
}

Coord TextBuffer::StringFindReversely(const std::wstring& str,
                                      Coord begin,
                                      Coord end,
                                      const std::wstring& sub,
                                      bool match_whole_word,
                                      CmpFunc cmp) const {
  Coord sub_size = CoordCast(sub.size());
  if (sub_size == 0 || sub_size > (end - begin)) {
    return kInvalidCoord;
  }

  for (Coord i = end - sub_size; i >= begin; --i) {
    if (cmp(&str[i], &sub[0], sub_size) == 0) {
      if (!match_whole_word) {
        return i;
      }

      if (!ft_plugin_->IsDelimiter(sub[0])) {
        if (i > 0 && !ft_plugin_->IsDelimiter(str[i - 1])) {
          continue;
        }
      }

      if (!ft_plugin_->IsDelimiter(sub[sub.size() - 1])) {
        Coord k = i + sub_size;
        if (k < CoordCast(str.size()) && !ft_plugin_->IsDelimiter(str[k])) {
          continue;
        }
      }

      return i;
    }
  }

  return kInvalidCoord;
}

//------------------------------------------------------------------------------

TextPoint TextBuffer::UnmatchedBracketL(const TextPoint& point) const {
  int counter[kBracketCount] = { 0 };

  const std::wstring& line_data = LineData(point.y);
  Coord x = point.x - 1;
  for (; x >= 0; --x) {
    if (CheckBracket(line_data[x], BracketL, BracketR, counter)) {
      return TextPoint(x, point.y);
    }
  }

  for (Coord y = point.y - 1; y > 0; --y) {
    const std::wstring& line_data = LineData(y);
    Coord x = LineLength(y) - 1;
    for (; x >= 0; --x) {
      if (CheckBracket(line_data[x], BracketL, BracketR, counter)) {
        return TextPoint(x, y);
      }
    }
  }

  return kInvalidPoint;
}

TextPoint TextBuffer::UnmatchedBracketR(const TextPoint& point) const {
  int counter[kBracketCount] = { 0 };

  const std::wstring& line_data = LineData(point.y);
  const Coord line_length = LineLength(point.y);
  Coord x = point.x;
  for (; x < line_length; ++x) {
    if (CheckBracket(line_data[x], BracketR, BracketL, counter)) {
      return TextPoint(x, point.y);
    }
  }

  const Coord line_count = LineCount();
  for (Coord y = point.y + 1; y <= line_count; ++y) {
    const std::wstring& line_data = LineData(y);
    const Coord line_length = LineLength(y);
    Coord x = 0;
    for (; x < line_length; ++x) {
      if (CheckBracket(line_data[x], BracketR, BracketL, counter)) {
        return TextPoint(x, y);
      }
    }
  }

  return kInvalidPoint;
}

//------------------------------------------------------------------------------

TextPoint TextBuffer::SeekPrevChar(const TextPoint& point) {
  if (point.x > 0) {
    Coord line_length = LineLength(point.y);
    if (point.x > line_length) {
      // Skip virtual spaces.
      return TextPoint(line_length, point.y);
    } else {
      return TextPoint(point.x - 1, point.y);
    }
  } else {
    if (point.y > 1) {
      return TextPoint(LineLength(point.y - 1), point.y - 1);
    } else {
      return point;
    }
  }
}

TextPoint TextBuffer::SeekNextChar(const TextPoint& point) {
  if (point.x < LineLength(point.y)) {
    return TextPoint(point.x + 1, point.y);
  } else {
    if (point.y < LineCount()) {
      return TextPoint(0, point.y + 1);
    } else {
      return point;
    }
  }
}

TextPoint TextBuffer::SeekPrevWord(const TextPoint& point) {
  assert(point.y <= LineCount());

  const Coord line_length = LineLength(point.y);

  if (point.x > line_length) {
    // Skip virtual spaces.
    return TextPoint(line_length, point.y);
  }

  TextPoint begin_point;

  if (point.x == 0) {
    if (point.y == 1) {  // No previous line.
      begin_point = point;
    } else {
      begin_point.y = point.y - 1;
      begin_point.x = LineLength(begin_point.y);
      if (begin_point.x > 0) {  // Previous line is not empty.
        begin_point.x = WordBegin(
            TextPoint(begin_point.x - 1, begin_point.y), true);
      }
    }
  } else {  // point.x > 0
    begin_point.y = point.y;
    size_t i = WordBegin(TextPoint(point.x - 1, point.y), true);
    begin_point.x = i;
  }

  return begin_point;
}

TextPoint TextBuffer::SeekNextWord(const TextPoint& point) {
  assert(point.y <= LineCount());

  const Coord line_length = LineLength(point.y);
  assert(point.x <= line_length);

  TextPoint end_point;

  if (point.x == line_length) {
    if (point.y == LineCount()) {  // No next line.
      end_point = point;
    } else {
      end_point.y = point.y + 1;
      end_point.x = Line(end_point.y)->FirstNonSpaceChar();
    }
  } else {  // point.x < line_length
    size_t i = WordEnd(point, true);
    end_point.Set(i, point.y);
  }

  return end_point;
}

TextPoint TextBuffer::SeekWordBegin(const TextPoint& point) {
  const TextLine* line = Line(point.y);
  if (line->Length() == 0) {
    return point;
  }

  assert(point.x <= line->Length());

  if (point.x == 0) {
    return point;
  }

  if (point.x == line->Length()) {
    return TextPoint(WordBegin(TextPoint(point.x - 1, point.y), false),
                     point.y);
  } else {
    return TextPoint(WordBegin(point, false), point.y);
  }
}

TextPoint TextBuffer::SeekWordEnd(const TextPoint& point) {
  const TextLine* line = Line(point.y);
  if (line->Length() == 0) {
    return point;
  }

  assert(point.x <= line->Length());

  if (point.x == line->Length()) {
    return point;
  }

  return TextPoint(WordEnd(point, false), point.y);
}

TextPoint TextBuffer::SeekLineBegin(const TextPoint& point) {
  return TextPoint(0, point.y);
}

TextPoint TextBuffer::SeekLineEnd(const TextPoint& point) {
  return TextPoint(LineLength(point.y), point.y);
}

TextPoint TextBuffer::SeekPrevLine(const TextPoint& point) {
  if (point.y <= 1) {
    return point;
  }
  return TextPoint(0, point.y - 1);
}

TextPoint TextBuffer::SeekNextLine(const TextPoint& point) {
  if (point.y >= LineCount()) {
    return point;
  }
  return TextPoint(0, point.y + 1);
}

Coord TextBuffer::WordBegin(const TextPoint& point, bool include_space) {
  TextLine* line = Line(point.y);
  assert(point.x < line->Length());

  Coord x = point.x;

  wchar_t c = line->Char(x);

  if (IsSpace(c)) {
    // Skip spaces before.
    for (; x > 0 && IsSpace(line->Char(x - 1)); --x) {}
    if (include_space && x > 0) {
      x = WordBegin(TextPoint(x - 1, point.y), include_space);
    }
  } else {
    if (ft_plugin_->IsOperator(c)) {
      // Skip operators before.
      for (; x > 0 && ft_plugin_->IsOperator(line->Char(x - 1)); --x) {}
    } else {
      for (; x > 0; --x) {
        if (ft_plugin_->IsDelimiter(line->Char(x - 1))) {
          break;
        }
      }
    }
  }

  return x;
}

Coord TextBuffer::WordEnd(const TextPoint& point, bool include_space) {
  TextLine* line = Line(point.y);
  assert(point.x < line->Length());

  Coord x = point.x;
  wchar_t c = line->Char(x);
  const Coord length = line->Length();

  if (IsSpace(c)) {  // This word will be space(s).
    // Skip spaces.
    for (++x; x < length && IsSpace(line->Char(x)); ++x) {
    }
  } else if (ft_plugin_->IsOperator(c)) {  // This word will be operator(s).
    // Skip operators.
    for (++x; x < length && ft_plugin_->IsOperator(line->Char(x)); ++x) {
    }
    if (include_space) {
      // Skip the spaces after this operator.
      for (; x < length && IsSpace(line->Char(x)); ++x) {
      }
    }
  } else {
    // Find the end of this word.
    for (++x; x < length; ++x) {
      if (IsSpace(line->Char(x))) {
        if (include_space) {
          for (++x; x < length && IsSpace(line->Char(x)); ++x) {
          }
        }
        break;
      } else if (ft_plugin_->IsDelimiter(line->Char(x))) {
        break;
      }
    }
  }

  return x;
}

void TextBuffer::SplitWords(const std::wstring& line_data,
                            bool include_space,
                            std::vector<std::wstring>* words) {
  size_t p = 0;

  for (size_t i = 0; i < line_data.size();) {
    p = i;

    if (IsSpace(line_data[i])) {  // This word will be spaces.
      while (i < line_data.size() && IsSpace(line_data[i])) {
        ++i;
      }
    } else if (ft_plugin_->IsOperator(line_data[i])) {
      // This word will be operator(s).
      while (i < line_data.size() && ft_plugin_->IsOperator(line_data[i])) {
        ++i;
      }
      if (include_space) {
        // Skip the white spaces after this operator.
        while (i < line_data.size() && IsSpace(line_data[i])) {
          ++i;
        }
      }
    } else {
      // Find a space or operator indicating the end of this word.
      for (++i; i < line_data.size(); ++i) {
        if (IsSpace(line_data[i])) {
          if (include_space) {
            while (i < line_data.size() && IsSpace(line_data[i])) {
              ++i;
            }
          }
          break;
        } else if (ft_plugin_->IsDelimiter(line_data[i])) {
          break;
        }
      }
    }

    words->push_back(line_data.substr(p, i - p));
  }
}

//------------------------------------------------------------------------------

Action* TextBuffer::TopUndoAction() {
  MergeInsertCharActions();

  if (!undo_actions_.empty()) {
    return undo_actions_.front();
  }
  if (!saved_undo_actions_.empty()) {
    return saved_undo_actions_.front();
  }
  return NULL;
}

Action* TextBuffer::PopUndoAction() {
  MergeInsertCharActions();

  if (!undo_actions_.empty()) {
    Action* action = undo_actions_.front();
    undo_actions_.pop_front();
    return action;
  }

  if (!saved_undo_actions_.empty()) {
    Action* action = saved_undo_actions_.front();
    saved_undo_actions_.pop_front();
    return action;
  }

  return NULL;
}

void TextBuffer::PushUndoAction(Action* action) {
  if (action->saved()) {
    assert(undo_actions_.empty());
    assert(recent_ic_actions_.empty());
    saved_undo_actions_.push_front(action);
  } else {
    undo_actions_.push_front(action);
  }
}

void TextBuffer::PushRedoAction(Action* action) {
  redo_actions_.push_front(action);
}

void TextBuffer::ClearActions() {
  ClearContainer(&recent_ic_actions_);
  ClearContainer(&saved_undo_actions_);
  ClearContainer(&undo_actions_);
  ClearContainer(&redo_actions_);
}

void TextBuffer::SaveUndoActions() {
  MergeInsertCharActions();

  std::list<Action*>::const_reverse_iterator it = undo_actions_.rbegin();
  for (; it != undo_actions_.rend(); ++it) {
    (*it)->set_saved(true);
    saved_undo_actions_.push_front(*it);
  }
  undo_actions_.clear();

  last_saved_undo_count_ = saved_undo_actions_.size();
}

// Merge inserted chars by word.
void TextBuffer::MergeInsertCharActions() {
  if (recent_ic_actions_.empty()) {
    return;
  }

  std::wstring str;
  str.resize(recent_ic_actions_.size());
  for (size_t i = 0; i < recent_ic_actions_.size(); ++i) {
    str[i] = recent_ic_actions_[i]->c();
  }

  std::vector<std::wstring> words;
  SplitWords(str, true, &words);

  // p: Insert char action index.
  for (size_t i = 0, p = 0; i < words.size(); ++i) {
    TextPoint point = recent_ic_actions_[p]->point();

    Action* action = NULL;
    if (words[i].size() == 1) {
      action = new InsertCharAction(this, point, words[i][0]);
    } else {  // > 1
      action = new InsertStringAction(this, point, words[i]);
    }

    action->set_grouped(recent_ic_actions_[p]->grouped());

    // NOTE: Don't call PushUndoAction() to avoid notify.
    undo_actions_.push_front(action);

    p += words[i].size();
  }

  ClearContainer(&recent_ic_actions_);
}

//------------------------------------------------------------------------------
// Lex

// Check if the char at the given index is an unescaped \.
// Examples:
// "\\"[1] -> false
// "a\"[1] -> true
static bool IsUnescapedBackSlash(const std::wstring& str, size_t i) {
  // Count '\' backward. It's unescaped if the number is odd.
  size_t count = 0;

  while (str[i] == L'\\') {
    ++count;
    if (i == 0) {
      break;
    }
    --i;
  }

  return (count % 2 == 1);
}

void TextBuffer::ScanLex(TextLine* line, Quote*& quote) {
  line->ClearLexElements();
  line->ClearQuoteInfos();

  const std::wstring& line_data = line->data();

  // quote != NULL: this line continues the last quote.
  size_t quote_off = quote != NULL ? 0 : kNpos;

  LexElement le;  // Current lex element.
  LexElement prev_le;  // Previous lex element.

  size_t i = 0;

  if (quote != NULL && !quote->multi_line() && quote->NoEnd()) {
    // Single line quote ending with EOL, don't have to step forward by char.
    // NOTE: If nested quote is allowed, this doesn't work.
    i = line_data.size();
  }

  for (; i < line_data.size();) {
    // Skip spaces. (Don't keep spaces as lex element to save memory.)
    if (IsSpace(line_data[i])) {
      for (++i; i < line_data.size() && IsSpace(line_data[i]); ++i) {}
      continue;
    }

    // Reset current lex element.
    le.Reset();

    if (quote != NULL) {  // Check quote end.
      size_t quote_i = i;
      if (!quote->NoEnd() && SubStringEquals(line_data, i, quote->end())) {
        quote_i += quote->end().length();

        if (i == 0 || !IsUnescapedBackSlash(line_data, i - 1)) {  // Quote ends.
          line->AddQuoteInfo(quote, quote_off, i - quote_off, Quote::kBody);
          line->AddQuoteInfo(quote, i, quote_i - i, Quote::kEnd);

          i = quote_i;
          le.Set(quote_off, i - quote_off, quote->lex());
          line->AddLexElement(le);

          quote = NULL;
        } else {  // Quote continues.
          ++i;
        }
      } else {  // Quote continues.
        ++i;
      }
    } else {  // Check quote start.
      size_t quote_i = ft_plugin_->MatchQuote(line_data, i, &quote);

      if (quote_i > i) {  // Quote starts.
        quote_off = i;
        line->AddQuoteInfo(quote, i, quote_i - i, Quote::kStart);

        if (!quote->multi_line() && quote->NoEnd()) {
          // Single line quote ends with EOL.
          // Don't have to step forward by char. Break to go to the line end.
          // NOTE: If nested quote is allowed, this doesn't work.
          break;
        }

        i = quote_i;
        prev_le.Reset();

      } else {  // No quote starts, check other lex types.
        le.Set(i, 0, Lex(kLexNone));

        // Match regex.
        // Suppose "regex" always starts from the first-non-space char.
        // This might be an ISSUE for some file types.
        size_t regex_i = ft_plugin_->MatchRegex(line_data, i, &le.lex);
        if (regex_i > i) {
          le.len = regex_i - i;
          prev_le = le;
          line->AddLexElement(le);
          i = regex_i;
          continue;
        }

        if (ft_plugin_->IsOperator(line_data[i])) {
          if (prev_le.len > 0 && prev_le.lex.IsEmpty()) {
            // Look back to check the lex of previous word.
            // FIXME: Currently, only a single operator can be suffix.
            if (ft_plugin_->MatchSuffix(line_data, i, 1, &prev_le.lex)) {
              line->AddLexElement(prev_le);
            }
          }

          prev_le.Set(i, 1, Lex(kLexSpecial));  // Any non kLexNone is OK.

          // Skip operator. (ISSUE: See comments for "match regex".)
          // NOTE: If there are more operators after this operator, don't skip
          // them here. Because they might be the quote start.
          ++i;

          continue;
        }

        // Get the word.
        for (++i; i < line_data.size(); ++i) {
          if (ft_plugin_->IsDelimiter(line_data[i])) {
            break;
          }
        }

        // Update the word length.
        le.len = i - le.off;

        // Match anyof.
        if (ft_plugin_->MatchAnyof(line_data, le.off, le.len, &le.lex)) {
          line->AddLexElement(le);
          prev_le = le;
          continue;
        }

        if (prev_le.len > 0) {
          // Look back to check if the previous word is a lex prefix.
          // FIXME: If the previous word is operators, only one single
          // operator is supported.
          if (ft_plugin_->MatchPrefix(line_data,
                                      prev_le.off,
                                      prev_le.len,
                                      &le.lex)) {
            line->AddLexElement(le);
            prev_le = le;
            continue;
          }
        }

        // Save the previous lex element.
        prev_le = le;
      }
    }
  }

  // Deal with unclosed quote and EOL-closed quote.
  if (quote == NULL) {
    return;
  }

  assert(quote_off != kNpos);

  if (line_data.size() > quote_off) {
    line->AddLexElement(quote_off, line_data.size() - quote_off, quote->lex());
  }

  if (quote->multi_line()) {
    // Quote continues to next line.
    // Note the line might be empty. Add quote info even the line is empty.
    size_t count = line_data.size() - quote_off;
    line->AddQuoteInfo(quote, quote_off, count, Quote::kBody);
    return;
  }

  // Single line quote.
  line->AddQuoteInfo(quote,
                     quote_off,
                     line_data.size() - quote_off,
                     Quote::kBody);

  if (quote->NoEnd()) {  // Quote ends with EOL.
    if (!quote->escape_eol() ||
        !(!line_data.empty() &&
          IsUnescapedBackSlash(line_data, line_data.size() - 1))) {
      // Quote ends.
      line->AddQuoteInfo(quote, line_data.size(), 0, Quote::kEnd);
      quote = NULL;
    }  // else: EOL is escaped, quote continues.

  } else {  // Quote has an end marker.
    if (line_data.empty() ||
        !quote->escape_eol() ||
        !IsUnescapedBackSlash(line_data, line_data.size() - 1)) {
      // Quote is invalid with no end marker found. Just end it.
      line->AddQuoteInfo(quote, line_data.size(), 0, Quote::kEnd);
      quote = NULL;
    }  // else: Quote continues.
  }
}

void TextBuffer::ScanLex() {
  Quote* quote = NULL;
  ScanLex(1, quote);
}

void TextBuffer::ScanLex(Coord start_ln, Quote* quote) {
  assert(start_ln <= LineCount());

#if JIL_MARK_UNCLOSED_QUOTE
  TextLine* quote_start_line = NULL;

  TextLines::iterator it = lines_.begin() + (start_ln - 1);
  for (; it < lines_.end(); ++it) {
    if (quote == NULL) {
      ScanLex(*it, quote);
      if (quote != NULL) {
        quote_start_line = *it;
      }
    } else {
      ScanLex(**it, quote);
    }
  }

  // Deal with unclosed quote.
  if (quote != NULL) {
    assert(quote_start_line != NULL);
    quote_start_line->AddMark(0, quote_start_line->Length());
  }

#else
  TextLines::iterator it = lines_.begin() + (start_ln - 1);
  for (; it < lines_.end(); ++it) {
    ScanLex(*it, quote);
  }
#endif  // JIL_MARK_UNCLOSED_QUOTE
}

Coord TextBuffer::ScanLexTillQuoteEnd(Coord start_ln, Quote* quote) {
  Coord line_count = LineCount();
  for (Coord ln = start_ln; ln <= line_count; ++ln) {
    ScanLex(Line(ln), quote);
    // Stop scanning lex after quote ends.
    if (quote == NULL) {
      return ln;
    }
  }
  return line_count;
}

void TextBuffer::ScanLexOnLineUpdated(const LineRange& line_range) {
  wxLogDebug(wxT("TextBuffer::ScanLexOnLineUpdated(%s)"),
             line_range.ToString().c_str());

  // Check previous unended quote.
  Quote* quote = NULL;
  if (line_range.first() > 1) {
    quote = Line(line_range.first() - 1)->UnendedQuote(true);
  }

  wxLogDebug("  - Scan lex from line %d to %d",
             line_range.first(),
             line_range.last());

  for (Coord ln = line_range.first(); ln < line_range.last(); ++ln) {
    ScanLex(Line(ln), quote);
  }

  TextLine* last_line = Line(line_range.last());
  Quote* old_quote = last_line->UnendedQuote(true);
  // Scan last line.
  ScanLex(last_line, quote);

  if (quote != old_quote) {
    // Find the line which ends the old quote.
    Coord old_quote_end_ln = line_range.last();
    if (old_quote != NULL) {
      Coord line_count = LineCount();
      old_quote_end_ln = line_count;
      for (Coord ln = line_range.last() + 1; ln <= line_count; ++ln) {
        if (Line(ln)->EndQuote(old_quote)) {
          old_quote_end_ln = ln;
          break;
        }
      }
    }

    // Last line quote changes.
    wxLogDebug("  - Last line quote changes: %d", line_range.last());
    Coord stop_ln = ScanLexTillQuoteEnd(line_range.last() + 1, quote);

    if (stop_ln < old_quote_end_ln) {
      ScanLex(stop_ln + 1, quote);
    }

    wxLogDebug("  - Scan lex from line %d to %d",
               line_range.last() + 1,
               stop_ln);
  }
}

void TextBuffer::ScanLexOnLineAdded(const LineRange& line_range) {
  wxLogDebug(wxT("TextBuffer::ScanLexOnLineAdded(%s)"),
             line_range.ToString().c_str());

  // Check previous unended quote.
  Quote* quote = NULL;
  Quote* quote_before = NULL;
  if (line_range.first() > 1) {
    quote = Line(line_range.first() - 1)->UnendedQuote(true);
    quote_before = quote;
  }

  wxLogDebug("  - Scan lex from line %d to %d",
             line_range.first(),
             line_range.last());

  for (Coord ln = line_range.first(); ln <= line_range.last(); ++ln) {
    ScanLex(Line(ln), quote);
  }

  if (quote != quote_before) {
    // Last line quote changes.
    wxLogDebug("  - Last line quote changes: %d", line_range.last());
    Coord stop_ln = ScanLexTillQuoteEnd(line_range.last() + 1, quote);

    wxLogDebug("  - Scan lex from line %d to %d",
               line_range.last() + 1,
               stop_ln);
  }
}

void TextBuffer::ScanLexOnLineDeleted(const LineRange& line_range) {
  wxLogDebug(wxT("TextBuffer::ScanLexOnLineDeleted(%s)"),
             line_range.ToString().c_str());

  Quote* quote = NULL;
  if (line_range.first() > 1) {
    quote = Line(line_range.first() - 1)->UnendedQuote(true);
  }

  if (line_range.first() <= LineCount()) {
    Quote* next_quote = NULL;

    TextLine* next_line = Line(line_range.first());
    if (!next_line->quote_infos().empty()) {
      const QuoteInfo& qi = next_line->quote_infos().front();
      if (qi.part != Quote::kStart) {
        next_quote = qi.quote;
      }
    }

    if (next_quote != quote) {
      Coord stop_ln = ScanLexTillQuoteEnd(line_range.first(), quote);
      wxLogDebug("  - Scan lex from line %d to %d",
                 line_range.first(),
                 stop_ln);
    }
  }
}

//------------------------------------------------------------------------------

void TextBuffer::AddLineLength(TextLine* line) {
  const Coord length = line->Length();

  if (length > kLongLineBoundary) {
    ++long_line_length_map_[length];
  } else {
    ++line_length_table_[length];
  }
}

void TextBuffer::RemoveLineLength(TextLine* line) {
  const Coord length = line->Length();

  if (length > kLongLineBoundary) {
    LongLineLengthMap::iterator it = long_line_length_map_.find(length);
    if (it != long_line_length_map_.end()) {
      --it->second;

      if (it->second <= 0) {  // No line with this length any more.
        long_line_length_map_.erase(it);
      }
    }
  } else {
    if (line_length_table_[length] > 0) {
      --line_length_table_[length];
    }
  }
}

void TextBuffer::ClearLineLength() {
  for (size_t i = 0; i <= kLongLineBoundary; ++i) {
    line_length_table_[i] = 0;
  }
  long_line_length_map_.clear();
}

}  // namespace editor
}  // namespace jil
