#ifndef EDITOR_TEXT_BUFFER_H_
#define EDITOR_TEXT_BUFFER_H_

#include <deque>
#include <list>
#include <map>
#include <string>
#include <vector>

#include "wx/datetime.h"
#include "wx/filename.h"
#include "wx/fontenc.h"
#include "wx/string.h"

#include "editor/compile_config.h"
#include "editor/defs.h"
#include "editor/option.h"
#include "editor/text_line.h"
#include "editor/text_listener.h"
#include "editor/text_point.h"
#include "editor/text_range.h"

// Note about wrap:
// Wrap is an operation of the view instead of the model,
// because wrap needs to know the text client width.

namespace editor {

class Action;
class DeleteAction;
class FtPlugin;
class InsertCharAction;
class LinePred;
class Quote;
class TextExtent;

enum Bracket {
  kParenthesis = 0,   // ()
  kBrace,             // {}
  kSquareBracket,     // []
  kAngleBracket,      // <>
  kBracketCount,
  kNoBracket = kBracketCount
};

class QuoteInfo {
public:
  TextPoint start_point;
  TextPoint end_point;
  Coord start_len;
  Coord end_len;
};

// Text buffer is the model of an opened text file.
// A view implemented interface TextListener listens to the change of it.
// A text buffer may have multiple views.
class TextBuffer {
public:
  // Use deque instead of vector for faster line insert & delete operations.
  typedef std::deque<TextLine*> TextLines;

  //----------------------------------------------------------------------------

  TextBuffer(std::size_t id, FtPlugin* ft_plugin,
             const Encoding& file_encoding);

  ~TextBuffer();

  //----------------------------------------------------------------------------
  // File

  // TODO: Add notify parameter?
  bool LoadFile(const wxFileName& fn, int cjk_filters);

  FileError SaveFile();

  //----------------------------------------------------------------------------

  TextPoint point_begin() const {
    return TextPoint(0, 1);
  }

  TextPoint point_end() const {
    return TextPoint(LineLength(LineCount()), LineCount());
  }

  // The whole text range.
  TextRange range() const {
    return TextRange(point_begin(), point_end());
  }

  //----------------------------------------------------------------------------

  std::size_t id() const { return id_; }

  // Example: "E:\doc\example.txt"
  wxString file_path_name() const;
  void set_file_path_name(const wxString& file_path_name);

  // Example: "example.txt"
  wxString file_name() const;

  // Example: "txt"
  wxString file_ext() const;

  // Example: "~/proj/jil/src/editor/"
  wxString file_path(int flags = wxPATH_GET_VOLUME | wxPATH_GET_SEPARATOR,
                     wxPathFormat format = wxPATH_NATIVE) const;

  const wxFileName& file_name_object() const {
    return file_name_object_;
  }

  void set_file_name_object(const wxFileName& file_name_object) {
    file_name_object_ = file_name_object;
  }

  const Encoding& file_encoding() const {
    return file_encoding_;
  }

  void set_file_encoding(const Encoding& file_encoding) {
    file_encoding_ = file_encoding;
  }

  FileFormat file_format() const {
    return file_format_;
  }

  void set_file_format(FileFormat file_format) {
    file_format_ = file_format;
  }

  //----------------------------------------------------------------------------

  FtPlugin* ft_plugin() const {
    return ft_plugin_;
  }

  // Change file type plugin.
  // E.g., when a buffer is saved as another file type.
  void SetFtPlugin(FtPlugin* ft_plugin);

  //----------------------------------------------------------------------------
  // Options

  const Options& options() const {
    return options_;
  }

  const TextOptions& text_options() const {
    return options_.text;
  }
  void set_text_options(const TextOptions& text) {
    options_.text = text;
  }

  const ViewOptions& view_options() const {
    return options_.view;
  }
  void set_view_options(const ViewOptions& view_options) {
    options_.view = view_options;
  }

  int tab_stop() const {
    return options_.text.tab_stop;
  }
  void set_tab_stop(int tab_stop) {
    options_.text.tab_stop = tab_stop;
  }

  bool expand_tab() const {
    return options_.text.expand_tab;
  }
  void set_expand_tab(bool expand_tab) {
    options_.text.expand_tab = expand_tab;
  }

  // Set tab options and optionally notify kTabOptionsChange.
  void SetTabOptions(const TabOptions& tab_options, bool notify);

  // Guess tab options from existing lines.
  bool GuessTabOptions(TabOptions* tab_options) const;

  // Get extra indent option.
  OptionValue GetIndentOption(const std::string& key) const;

  // Only used by unit test.
  void SetIndentOption(const std::string& key, const OptionValue& value);

  //----------------------------------------------------------------------------
  // States

  // New buffer. No file path name.
  bool new_created() const;

  // Buffer's modified, but not saved yet.
  bool modified() const;

  // Read-only. Could be edited, but saving the change is not allowed.
  bool read_only() const;

  // The file is deleted in disk (after it's opened in Jil).
  bool deleted() const;

  //----------------------------------------------------------------------------

  // Return true if the buffer is empty.
  bool IsEmpty() const;

  Coord LineCount() const;

  TextLine* Line(Coord ln) const;

  TextLine* LastLine() const;

  const TextLine* LineById(std::size_t id) const;

  // Return kInvCoord if the line with the given ID doesn't exist.
  Coord LineNrFromId(std::size_t id) const;

  // Return the previous non-empty line.
  // \param skip_comment Skip comment lines.
  Coord PrevNonEmptyLine(Coord ln, bool skip_comment) const;

  const std::wstring& LineData(Coord ln) const;

  // Return the char at the given point.
  wchar_t Char(const TextPoint& point) const;

  std::size_t LineId(Coord ln) const;

  Coord LineLength(Coord ln) const;

  bool IsLineEmpty(Coord ln, bool ignore_spaces) const;

  bool AreLinesAllEmpty(const LineRange& line_range, bool ignore_spaces) const;

  // Get the lex at the given point.
  Lex GetLex(const TextPoint& point) const;

  void GetText(std::wstring* text) const;
  void GetText(const TextRange& range, std::wstring* text) const;
  void GetRectText(const TextRange& range, std::wstring* text) const;

  //----------------------------------------------------------------------------
  // Buffer change.

  void SetText(const std::wstring& text, bool notify = false);

  void ClearText(bool notify);

  TextPoint InsertChar(const TextPoint& point, wchar_t c);

  void DeleteChar(const TextPoint& point, wchar_t* c = NULL);

  TextPoint InsertString(const TextPoint& point, const std::wstring& str);

  void DeleteString(const TextPoint& point, Coord count,
                    std::wstring* str = NULL);

  // The new line will be with the given line number.
  void InsertLine(Coord ln, const std::wstring& line_data = L"");

  // Return the appended line.
  TextLine* AppendLine(const std::wstring& line_data);

  void DeleteLine(Coord ln, std::wstring* line_data = NULL);

  TextPoint InsertText(const TextPoint& point, const std::wstring& text);
  TextPoint InsertRectText(const TextPoint& point, const std::wstring& text);

  // Delete the text in the given range.
  void DeleteText(const TextRange& range, std::wstring* text = NULL);

  void DeleteRectText(const TextRange& range, std::wstring* text = NULL);

  void HandleLineChange(LineChangeType type, Coord first_ln, Coord last_ln = 0);

  //----------------------------------------------------------------------------
  // Undo-able buffer change (with action(s) created).

  void UndoableInsertString(const TextPoint& point,
                            const std::wstring& str,
                            bool grouped);

  void UndoableDeleteRange(const TextRange& range,
                           TextDir dir,
                           bool rect,
                           bool grouped);

  void UndoableReplace(const TextRange& range,
                       const std::wstring& replace_str,
                       bool grouped);

  //----------------------------------------------------------------------------
  // Find & Replace

  // \param reversely Currently, only for non-regex find.
  TextRange FindString(const std::wstring& str,
                       const TextRange& range,
                       bool use_regex,
                       bool case_sensitive,
                       bool match_word,
                       bool reversely) const;

  void FindStringAll(const std::wstring& str,
                     const TextRange& range,
                     bool use_regex,
                     bool case_sensitive,
                     bool match_word,
                     std::list<TextRange>* result_ranges) const;

  void ReplaceStringAll(const std::wstring& str,
                        const std::wstring& replace_str,
                        const TextRange& range,
                        bool use_regex,
                        bool case_sensitive,
                        bool match_word);

  //----------------------------------------------------------------------------
  // Listener

  void AttachListener(TextListener* listener);
  void DetachListener(TextListener* listener);

  std::size_t ListenerCount() const {
    return listeners_.size();
  }

  void Notify(LineChangeType type, const LineChangeData& data);

  void Notify(LineChangeType type, const LineRange& line_range) {
    Notify(type, LineChangeData(line_range));
  }

  void Notify(ChangeType type);

  void FreezeNotify();
  void ThawNotify();

  //----------------------------------------------------------------------------
  // Indent

  // Get the indent as spaces of the given line.
  Coord GetIndent(Coord ln) const;

  // Get the indent as spaces of the previous non-empty line.
  Coord GetPrevLineIndent(Coord ln, bool skip_comment) const;

  // Get the original indent string of the given line.
  std::wstring GetIndentStr(Coord ln) const;

  // Get the length of the original indent string of the given line.
  Coord GetIndentStrLength(Coord ln) const;

  // Get the expected indent (as spaces) of the given line.
  // The indent function defined in the Lua script of this file type will be
  // invoked. If no indent function is defined, the indent of the previous
  // line will be returned.
  Coord GetExpectedIndent(Coord ln) const;

  //----------------------------------------------------------------------------
  // Bracket and key pairs: (), {}, [], <>, begin/end, etc.

  // The char at the given point should be one of (){}[]<>.
  // If the char at the given point is:
  // - (, find ) forward
  // - ], find [ backward
  // - etc.
  // Return invalid point if no match is found or the char at the given point
  // is not a bracket.
  // TODO
  // TextPoint MatchBracket(const TextPoint& point) const;

  // Get the outer bracket pair range around the given point.
  // Example:
  //      (          )
  //           p
  //      |          | (range)
  TextRange BracketPairOuterRange(const TextPoint& point) const;

  // Get the inner bracket pair range around the given point.
  // Example:
  //      (          )
  //           p
  //       |        | (range)
  TextRange BracketPairInnerRange(const TextPoint& point) const;

  // Check if the given range is a bracket pair inner range.
  bool IsBracketPairInnerRange(const TextRange& range) const;

  // Increase the given range by a char.
  // Example:
  // [(2,2), (8,2)) is normally increased to [(1,2), (9,2)).
  //   (      )
  //  (        )   increased
  // Return empty text range if failed to increase.
  TextRange IncreaseRange(const TextRange& range) const;

  // Find the first unpaired left key before the given point.
  // \param single_line Find only in the current line.
  // Examples:
  //   UnpairedLeftKey(point, L'{', L'}')
  //   UnpairedLeftKey(point, L'(', L')')
  // NOTE: The keys inside strings or comments are not taken into account.
  TextPoint UnpairedLeftKey(const TextPoint& point,
                            wchar_t l_key,
                            wchar_t r_key,
                            bool single_line = false) const;

  // Lua wrapper for UnpairedLeftKey.
  TextPoint Lua_UnpairedLeftKey(const TextPoint& point,
                                char l_key,
                                char r_key,
                                bool single_line) const;

  // Find the first unpaired right key after the given point.
  // Examples:
  //   UnpairedRightKey(point, L'{', L'}')
  //   UnpairedRightKey(point, L'(', L')')
  TextPoint UnpairedRightKey(const TextPoint& point, wchar_t l_key,
                             wchar_t r_key) const;

  //----------------------------------------------------------------------------
  // Seek

  TextPoint Seek(const TextPoint& point, TextUnit text_unit,
                 SeekType seek_type);

  //----------------------------------------------------------------------------
  // Action operations, undo/redo.

  // Return the added action or the previous action to which this action
  // is merged.
  Action* AddAction(Action* action);

  void AddGroupAction();

  // Special handling for insert char action.
  // Insert char actions added with this function might be merged later.
  void AddInsertCharAction(InsertCharAction* insert_char_action);

  // Return the undo action or NULL.
  Action* Undo();
  // Return the redo action or NULL.
  Action* Redo();

  bool CanUndo() const;
  bool CanRedo() const;

  //----------------------------------------------------------------------------
  // Lex

  void set_scan_lex(bool scan_lex) {
    scan_lex_ = scan_lex;
  }

  // Scan lex for the whole buffer.
  void ScanLex();

  bool GetQuoteInfo(const TextPoint& point, QuoteInfo* quote_info) const;

  //----------------------------------------------------------------------------
  // Line length table.

  Coord GetMaxLineLength() const;

private:
  //----------------------------------------------------------------------------

  void Init();

  //----------------------------------------------------------------------------
  // Options

  bool GuessTabOptionsWithoutFunc(TabOptions* tab_options) const;

  bool GetTabOptions(const TextLine* line,
                     const TextLine* prev_line,
                     TabOptions* tab_options) const;

  //----------------------------------------------------------------------------
  // Notify-free line operations.

  void DoInsertLine(Coord ln, TextLine* line) {
    lines_.insert(lines_.begin() + ln, line);
    AddLineLength(line);
  }

  void DoAppendLine(const std::wstring& line_data) {
    DoAppendLine(new TextLine(NewLineId(), line_data));
  }

  void DoAppendLine(const wchar_t* data, std::size_t length) {
    DoAppendLine(new TextLine(NewLineId(), data, length));
  }

  void DoAppendLine(const wchar_t* data) {
    DoAppendLine(new TextLine(NewLineId(), data));
  }

  void DoAppendLine(TextLine* line) {
    lines_.push_back(line);
    AddLineLength(line);
  }

  //----------------------------------------------------------------------------

  std::size_t NewLineId() {
    return ++line_id_;
  }

  //----------------------------------------------------------------------------
  // Find & Replace

  // Find a plain string in the given range.
  TextRange FindPlainString(const std::wstring& str,
                            const TextRange& range,
                            bool case_sensitive,
                            bool match_word) const;

  TextRange FindPlainStringReversely(const std::wstring& str,
                                     const TextRange& range,
                                     bool case_sensitive,
                                     bool match_word) const;

  // Find a regex string in the given range.
  TextRange FindRegexString(const std::wstring& str,
                            const TextRange& range,
                            bool case_sensitive,
                            bool reversely) const;

  TextRange FindRegex(const std::wregex& re, const TextRange& range) const;

  TextRange FindRegexReversely(const std::wregex& re,
                               const TextRange& range) const;

  // Find all occurrences of a plain string in the given range.
  void FindPlainStringAll(const std::wstring& str,
                          const TextRange& range,
                          bool case_sensitive,
                          bool match_word,
                          std::list<TextRange>* result_ranges) const;

  void FindRegexStringAll(const std::wstring& str,
                          const TextRange& range,
                          bool case_sensitive,
                          std::list<TextRange>* result_ranges) const;

  void FindRegexAll(const std::wregex& re,
                    const TextRange& range,
                    std::list<TextRange>* result_ranges) const;

  // Find regex all matching results in the given line.
  void FindRegexAll(const TextLine* line,
                    Coord ln,
                    Coord x_begin,
                    Coord x_end,
                    const std::wregex& re,
                    std::list<TextRange>* result_ranges) const;

  // Find a plain string inside the given line.
  bool FindLineString(TextLines::const_iterator line_it,
                      Coord x_begin,
                      Coord x_end,
                      const std::wstring& str,
                      bool case_sensitive,
                      bool reversely,
                      bool match_word,
                      TextRange* result_range) const;

  // Find all occurrences of a plain string inside the given line.
  void FindLineStringAll(TextLines::const_iterator line_it,
                         Coord x_begin,
                         Coord x_end,
                         const std::wstring& str,
                         bool case_sensitive,
                         bool match_word,
                         std::list<TextRange>* result_ranges) const;

  void ReplacePlainStringAll(const std::wstring& str,
                             const std::wstring& replace_str,
                             const TextRange& range,
                             bool case_sensitive,
                             bool match_word);

  void ReplaceRegexStringAll(const std::wstring& str,
                             const std::wstring& replace_str,
                             const TextRange& range,
                             bool case_sensitive);

  typedef int(*CmpFunc)(const wchar_t*, const wchar_t*, std::size_t);

  // STL and Boost.Algorithm both suck!
  // Find the sub in the range [begin, end) of str.
  // Return kInvCoord if not found.
  Coord StringFind(const std::wstring& str,
                   Coord begin,
                   Coord end,
                   const std::wstring& sub,
                   bool match_word,
                   CmpFunc cmp) const;

  // Find the sub reversely in the range [begin, end) of str.
  // Return kInvCoord if not found.
  Coord StringFindReversely(const std::wstring& str,
                            Coord begin,
                            Coord end,
                            const std::wstring& sub,
                            bool match_word,
                            CmpFunc cmp) const;

  //----------------------------------------------------------------------------
  // Bracket and key pairs: (), {}, [], <>, begin/end, etc.

  // TODO: Rename unmatched to unpaired.

  // Find the first unmatched left bracket before the given point.
  // Example:
  //      (    (   )
  //      * (found)     p
  TextPoint UnmatchedBracketL(const TextPoint& point) const;

  // Find the first unmatched right bracket after the given point.
  // Example:
  //          (   )     )
  //      p             * (found)
  TextPoint UnmatchedBracketR(const TextPoint& point) const;

  //----------------------------------------------------------------------------
  // Seek

  TextPoint SeekPrevChar(const TextPoint& point);
  TextPoint SeekNextChar(const TextPoint& point);

  TextPoint SeekPrevWord(const TextPoint& point);
  TextPoint SeekNextWord(const TextPoint& point);

  TextPoint SeekWordBegin(const TextPoint& point);
  TextPoint SeekWordEnd(const TextPoint& point);

  TextPoint SeekLineBegin(const TextPoint& point);
  TextPoint SeekLineEnd(const TextPoint& point);

  TextPoint SeekPrevLine(const TextPoint& point);
  TextPoint SeekNextLine(const TextPoint& point);

  // \param include_space The trailing spaces belong to the word before it.
  Coord WordBegin(const TextPoint& point, bool include_space);
  Coord WordEnd(const TextPoint& point, bool include_space);

  void SplitWords(const std::wstring& line_data, bool include_space,
                  std::vector<std::wstring>* words);

  //----------------------------------------------------------------------------
  // Undo / Redo

  Action* TopUndoAction();
  Action* PopUndoAction();

  void PushUndoAction(Action* action);
  void PushRedoAction(Action* action);

  void ClearActions();

  // Move current undo actions to "saved undo action list".
  void SaveUndoActions();

  // Merge recent insert char actions to insert string actions to make
  // undo more practical.
  void MergeInsertCharActions();

  bool MergeDeleteActions(const wxDateTime& now, DeleteAction* delete_action,
                          Action* prev_action);

  //----------------------------------------------------------------------------
  // Lex

  // Scan lex for the given line.
  // \param quote As input, it's the previous unclosed quote; as output, it's
  //   the unclosed quote after scan lex for this line.
  void ScanLex(TextLine* line, Quote*& quote);  // NOLINT

  // Scan lex from a given line with the optional previous unclosed quote.
  void ScanLex(Coord start_ln, Quote* quote);

  // Scan lex from the given start line till the quote ends.
  // Return the stop line.
  Coord ScanLexTillQuoteEnd(Coord start_ln, Quote* quote);

  // Scan lex on changes.
  void ScanLexOnLineUpdated(const LineRange& line_range);
  void ScanLexOnLineAdded(const LineRange& line_range);
  void ScanLexOnLineDeleted(const LineRange& line_range);

  // Clear lex for the whole buffer.
  // TODO
  void ClearLex();

  //----------------------------------------------------------------------------
  // Line length table

  void AddLineLength(TextLine* line);
  void RemoveLineLength(TextLine* line);
  void ClearLineLength();

  //----------------------------------------------------------------------------

  // Get line iterator by line number.
  TextLines::const_iterator GetLineIter(Coord ln) const;

private:
  // Unique ID of this text buffer.
  // You can't use file full path to identify text buffers uniquely because a
  // new created text buffer doesn't have file name before it's saved.
  std::size_t id_;

  wxFileName file_name_object_;

  // Shared file type specific data.
  FtPlugin* ft_plugin_;

  // Options specific to this buffer.
  // There might be some difference from the ones in the ft plugin.
  Options options_;

  // Encoding of the file.
  // The buffer itself always uses wchar_t as a character.
  Encoding file_encoding_;

  FileFormat file_format_;

  // Buffer states:
  bool read_only_;
  bool deleted_;

  // Text buffer consists of text lines.
  TextLines lines_;

  std::size_t line_id_;

  // A text buffer has one or more listeners.
  // Different listeners may want to listen to different changes of the buffer.
  // But in current implementation, a listener always listens to all changes.
  typedef std::vector<TextListener*> Listeners;
  Listeners listeners_;

  // The listeners won't be notified on line changes if it's frozen.
  // Currently this flag only applies to line changes.
  bool notify_frozen_;

  // Scan lex or not on line changes.
  // Default: true
  bool scan_lex_;

  // Undo/redo.

  std::list<Action*> undo_actions_;
  std::list<Action*> redo_actions_;

  // When buffer is saved, the undo actions are moved to this list.
  std::list<Action*> saved_undo_actions_;

  std::size_t last_saved_undo_count_;

  // Continuously inserted chars should be undone by word. Keep the recent
  // insert char actions to merge to insert string actions later.
  std::vector<InsertCharAction*> recent_ic_actions_;

  // The timestamp of the previous delete action.
  // Used to merge continuous the same kind of delete actions.
  wxDateTime prev_delete_action_time_;

  // Line length information.
  // For quick access of the longest line.

  enum {
    kLongLineBoundary = 256,
  };

  // Index is the line length, value is the count of lines with this length.
  // Example:
  // Suppose the buffer has the following five lines:
  // 1 xxx
  // 2 xx
  // 3
  // 4 xxxx
  // 5 xx
  // And the line length table will be:
  // Index  Value
  //   0      1     (line 3)
  //   1      0     (no line with size 1)
  //   2      2     (line 2 & 5)
  //   3      1     (line 1)
  //   4      1     (line 4)
  int line_length_table_[kLongLineBoundary + 1];

  // If the line is longer than kLongLineBoundary (e.g., 256), the line length
  // information is kept in a map instead of the above vector.
  typedef std::map<int, int> LongLineLengthMap;
  LongLineLengthMap long_line_length_map_;
};

}  // namespace editor

#endif  // EDITOR_TEXT_BUFFER_H_
