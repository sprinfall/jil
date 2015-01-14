#ifndef JIL_EDITOR_TEXT_WINDOW_H_
#define JIL_EDITOR_TEXT_WINDOW_H_
#pragma once

#include <list>
#include <string>
#include "wx/scrolwin.h"
#include "editor/binding.h"
#include "editor/buffer_listener.h"
#include "editor/compile_config.h"
#include "editor/defs.h"
#include "editor/lex.h"
#include "editor/option.h"
#include "editor/selection.h"
#include "editor/theme.h"

class wxMenu;
class wxMenuItem;
class wxTimer;
class wxTimerEvent;

namespace jil {
namespace editor {

class LineNrArea;
class TextArea;
class TextBuffer;
class FtPlugin;
class TextLine;
class TextExtent;
class WrapHelper;
class Renderer;
class Style;
class StyleValue;
class Options;
class Action;

class TextWindow : public wxScrolledWindow, public BufferListener {
  DECLARE_CLASS(TextWindow)
  DECLARE_EVENT_TABLE()

public:
  // Detailed event types of kTextWindowEvent.
  enum EventType {
    kEncodingEvent = 1,
    kFileFormatEvent,
    kFileNameEvent,
    kModifiedEvent,
    kCaretEvent,
    kLeaderKeyEvent,
    kGetFocusEvent,  // Text area gets focus
  };

  enum ColorId {
    RULER = 0,
    COLOR_COUNT
  };

public:
  explicit TextWindow(TextBuffer* buffer);

  // \param hide Set true to avoid flickering a small window (about 20x20).
  bool Create(wxWindow* parent, wxWindowID id, bool hide = false);

  virtual ~TextWindow();

  //----------------------------------------------------------------------------

  bool allow_text_change() const {
    return allow_text_change_;
  }
  void set_allow_text_change(bool allow_text_change) {
    allow_text_change_ = allow_text_change;
  }

  void set_theme(const SharedTheme& theme) {
    theme_ = theme;
  }

  void set_style(Style* style) {
    style_ = style;
  }

  void set_binding(Binding* binding) {
    binding_ = binding;
  }

  //----------------------------------------------------------------------------
  // Focus

  // Override to consider the text area's focus state.
  virtual bool HasFocus() const override;

  // Override to set focus to text area.
  virtual void SetFocus() override;

  //----------------------------------------------------------------------------
  // Font

  void SetTextFont(const wxFont& font);
  void SetLineNrFont(const wxFont& font);

  //----------------------------------------------------------------------------

  TextBuffer* buffer() const { return buffer_; }

  // Text buffer mediators:
  wxString buffer_file_name() const;
  bool buffer_modified() const;
  bool buffer_new_created() const;

  // Overriddens of BufferListener:
  virtual void OnBufferLineChange(LineChangeType type,
                                  const LineChangeData& data) override;
  virtual void OnBufferChange(ChangeType type) override;

  //----------------------------------------------------------------------------
  // Wrap

  void Wrap(bool wrap);

  //----------------------------------------------------------------------------

  TextArea* text_area() const { return text_area_; }
  LineNrArea* line_nr_area() const { return line_nr_area_; }

  const TextPoint& caret_point() const { return caret_point_; }

  void set_leader_key(Key* key);

  const Selection& selection() const { return selection_; }

  Mode mode() const;

  //----------------------------------------------------------------------------
  // Action

  void Exec(Action* action);

  // Undoable setting file format.
  void SetFileFormat(FileFormat file_format);

  bool CanUndo() const;
  bool CanRedo() const;

  void Undo();
  void Redo();

private:
  // Insert a char at the caret point.
  // If there's any selection, the selected text will be deleted.
  void InsertChar(wchar_t c);

public:
  void NewLineBreak();

  // Break a new line below.
  void NewLineBelow();

  // Break a new line above.
  void NewLineAbove();

private:  // TODO
  // Insert a string at the caret point.
  // If there's any selection, the selected text will be deleted.
  // The caret point will be updated.
  void InsertString(const std::wstring& str);

public:
  void InsertString(const TextPoint& point,
                    const std::wstring& str,
                    bool grouped,
                    bool update_caret);

  void Move(TextUnit text_unit, SeekType seek_type);

  void DeleteText(TextUnit text_unit, SeekType seek_type);

  void DeleteRange(const TextRange& range,
                   TextDir dir,
                   bool rect,
                   bool grouped,
                   bool selected,
                   bool update_caret);

  // Delete the current selection.
  void DeleteSelection(bool grouped, bool update_caret);

  void SelectText(TextUnit text_unit, SeekType seek_type);

  void ScrollText(TextUnit text_unit, SeekType seek_type);

  // Scroll if necessary to make sure the point is inside the client area.
  void ScrollToPoint(const TextPoint& point);

  //----------------------------------------------------------------------------
  // Scroll

  // wxWindow::ScrollLines() is very slow.
  // This function is implemented with wxWindow::Scroll() which is more
  // straightforward.
  void ScrollLinesDirectly(int lines);

  void Goto(Coord ln);

  //----------------------------------------------------------------------------

  // The number of lines a page contains.
  Coord GetPageSize() const;

  // The number of lines a half page contains.
  Coord GetHalfPageSize() const {
    return GetPageSize() / 2;
  }

  //----------------------------------------------------------------------------
  // Indent

  void AutoIndent(Coord ln);

  //----------------------------------------------------------------------------
  // Set new caret point and update caret position.
  // \param point The new caret point.
  // \param line_step Caret point is changed by line step action.
  // \param scroll Scroll to the caret point if it's not in the client area.
  // \vspace Allow virtual space.
  void UpdateCaretPoint(const TextPoint& point,
                        bool line_step,
                        bool scroll,
                        bool vspace);

  //----------------------------------------------------------------------------
  // Selection

  // NOTE: Caret point won't be updated.
  void SetSelection(const TextRange& range, TextDir dir, bool rect);

  void ClearSelection(bool refresh = true);

protected:
  //----------------------------------------------------------------------------
  // Mediators.

  FtPlugin* ft_plugin() const;

  // TODO
  const Options& options() const;

  //----------------------------------------------------------------------------
  // Wrap

  // Return the wrap helper (create it if NULL).
  WrapHelper* wrap_helper() const;

  //----------------------------------------------------------------------------
  // Handlers for buffer and buffer line changes.

  void HandleLineUpdated(const LineChangeData& data);
  void HandleLineAdded(const LineChangeData& data);
  void HandleLineDeleted(const LineChangeData& data);

  //----------------------------------------------------------------------------
  // System event handlers.

  void OnSize(wxSizeEvent& evt);

  void OnSetFocus(wxFocusEvent& evt);

  //----------------------------------------------------------------------------
  // Action

  // Restore or clear the selection, update caret point, etc.
  void UpdateAfterExec(Action* action);
  void UpdateAfterUndo(Action* action);

  void InsertChar(const TextPoint& point,
                  wchar_t c,
                  TextDir dir,
                  bool grouped);

  //----------------------------------------------------------------------------
  // Delegated event handlers from TextArea.

  friend class TextArea;

  void OnTextSize(wxSizeEvent& evt);

  // Return true if the event is handled.
  bool OnTextMouse(wxMouseEvent& evt);

  void OnTextPaint(Renderer& renderer);

  void HandleTextPaint(Renderer& renderer);
  void HandleWrappedTextPaint(Renderer& renderer);

  void DrawTextLine(Coord ln, Renderer& renderer, int x, int& y);
  void DrawWrappedTextLine(Coord ln,
                           Renderer& renderer,
                           int x,
                           int& y);

  // NOTE: Tabs and different text styles are handled here.
  void DrawTextLine(Renderer& renderer,
                    const TextLine* line,
                    int x,
                    int y);

  void DrawTextLinePiece(Renderer& renderer,
                         const std::wstring& line_data,
                         Coord i,
                         Coord j,
                         Lex lex,
                         int& x,
                         int y,
                         Coord& chars);

  void DrawTextWord(Renderer& renderer,
                    const std::wstring& line_data,
                    Coord off,
                    Coord len,
                    Lex lex,
                    int& x,
                    int y,
                    Coord& chars);

  void SetRendererStyle(Renderer& renderer,
                        const StyleValue* style_value);

  void HandleTextLeftDown(wxMouseEvent& evt);
  void HandleTextLeftDown_NoAccel();
  void HandleTextLeftDown_Ctrl();
  void HandleTextLeftDown_CtrlAlt();
  void HandleTextLeftDown_CtrlShift();
  void HandleTextLeftDown_CtrlAltShift();
  void HandleTextLeftDown_Alt();
  void HandleTextLeftDown_Shift();
  void HandleTextLeftDown_AltShift();

  void HandleTextMotion(wxMouseEvent& evt);
  void SelectByDragging();

  void StartScrollTimer();
  void StopScrollTimer();
  void OnScrollTimer(wxTimerEvent& evt);

  void HandleTextLeftUp(wxMouseEvent& evt);
  void HandleTextRightDown(wxMouseEvent& evt);
  virtual void HandleTextRightUp(wxMouseEvent& evt);
  virtual void HandleTextLeftDClick(wxMouseEvent& evt);

  // Return true if the event is handled.
  bool HandleTextMouseWheel(wxMouseEvent& evt);

  void ScrollHorizontally(wxMouseEvent& evt);

  void OnTextMouseCaptureLost(wxMouseCaptureLostEvent& evt);

  // Return true if the event is handled.
  bool OnTextKeyDown(wxKeyEvent& evt);

  void OnTextChar(wxKeyEvent& evt);

  void OnTextSetFocus(wxFocusEvent& evt);

  //----------------------------------------------------------------------------
  // Delegated event handlers from LineNrArea.

  friend class LineNrArea;

  void OnLineNrPaint(wxDC& dc);
  void HandleLineNrPaint(wxDC& dc);
  void HandleWrappedLineNrPaint(wxDC& dc);

  void OnLineNrSize(wxSizeEvent& evt);
  // Return true if the event is handled.
  bool OnLineNrMouse(wxMouseEvent& evt);

  void HandleLineNrLeftDown(wxMouseEvent& evt);
  void HandleLineNrMotion(wxMouseEvent& evt);
  void HandleLineNrLeftUp(wxMouseEvent& evt);

  //----------------------------------------------------------------------------

  // NOTE: The line number might be > line count.
  Coord LineFromScrolledY(int scrolled_y) const;

  // Calculate the caret point according to the text area client position.
  TextPoint CalcCaretPoint(const wxPoint& pos, bool vspace);

  // Get the client rect of text or line nr area from the given line range.
  wxRect ClientRectFromLineRange(wxWindow* area,
                                 const LineRange& line_range) const;

  wxRect ClientRectAfterLine(wxWindow* area, Coord ln, bool included) const;

  //----------------------------------------------------------------------------
  // Refresh

  // If the line is included, it will be refreshed together with
  // the lines after it.
  void RefreshTextAfterLine(Coord ln, bool included, bool update = false);
  void RefreshLineNrAfterLine(Coord ln, bool included, bool update = false);

  // Refresh a line.
  // Sometimes, it's necessary to update immediately to avoid
  // combined paint event.
  void RefreshTextByLine(Coord ln, bool update = false);

  // Refresh a range of lines.
  void RefreshTextByLineRange(const LineRange& line_range,
                              bool update = false);

  // Refresh the line number area of a line.
  void RefreshLineNrByLine(Coord ln, bool update = false);

  // Refresh the line number area of a range of lines.
  void RefreshLineNrByLineRange(const LineRange& line_range,
                                bool update = false);

  //----------------------------------------------------------------------------

  void HandleTextFontChange();
  void HandleLineNrFontChange();

  // Return true if areas are resized.
  bool HandleTextChange();

  void UpdateCharSize();
  void UpdateTextSize();
  void UpdateLineNrWidth();

  void UpdateVirtualSize();

  // Layout line number area, text area, etc.
  void LayoutAreas();

  //----------------------------------------------------------------------------
  // Caret

  // Set caret position according to current caret point.
  void UpdateCaretPosition();

  //----------------------------------------------------------------------------
  // Selection

  // Select from one point to another.
  // The caret point will be updated to the point_to.
  // And point_from might > point_to.
  // \param vspace Allow virtual space or not when update the caret point.
  void SetSelection(const TextPoint& point_from,
                    const TextPoint& point_to,
                    bool vspace);

  // Extend the current selection to the given point.
  // The caret point will be updated to the point_to.
  // \param vspace Allow virtual space or not when update the caret point.
  void ExtendSelection(const TextPoint& point_to, bool vspace);

  void ExtendSelectionByWord(const TextPoint& point_to);
  void ExtendSelectionByLine(Coord ln_to);

  void SelectByWord(const TextPoint& point_from, const TextPoint& point_to);
  // Select by line.
  void SelectByLine(Coord ln_from, Coord ln_to);

  //----------------------------------------------------------------------------

  // Get the width of the sub-line (substr(off1, off2 - off1)).
  int GetLineWidth(const TextLine* line,
                   Coord off1,
                   Coord off2 = kInvalidCoord) const;
  int GetLineWidth(Coord ln, Coord off1, Coord off2 = kInvalidCoord) const;

  // The char index might > line length if vspace is true.
  Coord GetCharIndex(Coord ln, int client_x, bool vspace) const;

  Coord GetWrappedCharIndex(Coord ln,
                            Coord wrapped_sub_ln,
                            int client_x,
                            bool vspace) const;

  int GetUnscrolledX(int scrolled_x) const;
  int GetUnscrolledY(int scrolled_y) const;

  wxPoint GetUnscrolledPos(const wxPoint& pos) const;

  int GetScrolledX(int unscrolled_x) const;
  int GetScrolledY(int unscrolled_y) const;

  // Get the range of lines currently displayed in the client area.
  // NOTE: The line number might be > line count.
  LineRange GetClientLineRange() const;

  //----------------------------------------------------------------------------

  void PostEvent(EventType type);

  wxMenuItem* AppendMenuItem(wxMenu* menu,
                             int id,
                             const wxString& label,
                             wxItemKind kind = wxITEM_NORMAL);
protected:
  TextBuffer* buffer_;

  // Some text window cannot be changed, e.g., find result.
  bool allow_text_change_;

  // Reference to the options in text buffer.
  Options& options_;

  Style* style_;
  SharedTheme theme_;

  Binding* binding_;

  // Sub-windows and the GUI related properties.
  LineNrArea* line_nr_area_;
  TextArea* text_area_;

  int line_nr_width_;

  // Char width is not very useful now, especially when the text font is not
  // mono-space or there are CJK characters. But just keep it.
  int char_width_;
  int char_height_;
  int line_height_;
  int text_width_;
  int text_height_;

  mutable WrapHelper* wrap_helper_;

  TextExtent* text_extent_;

  // Caret coordinates.
  // There's always a char at the right of the caret. If the caret is at the
  // end of a line, the char is the line ending. For a given line with N chars,
  // caret_point_.x is in the range [0, N].
  // For a text with N lines, caret_point_.y is in the range [1, N].
  TextPoint caret_point_;

  // Keep the max caret x while moving to previous/next line.
  // Example:
  // Given 3 lines of text, | indicates the caret:
  // *****************|*******
  // *********
  // *****************************
  // Move to next line:
  // *************************
  // ********|
  // *****************************
  // Move to next line:
  // *************************
  // *********
  // *****************|***********
  // Without this max caret x kept, the second "move to next line" would get:
  // *************************
  // *********
  // ********|********************
  Coord max_caret_x_;

  // A reference to an external Key object.
  // It would be better to keep a local copy. But currently, it's difficult to
  // update it (the copy) correctly.
  Key* leader_key_;

  // Last text area mouse down point.
  TextPoint down_point_;
  // Last text area mouse move position.
  wxPoint move_position_;

  // Modifier keys pressed on mouse left down.
  int down_modifiers_;
  // Whether the mouse is dragged or not.
  bool dragged_;

  // For scrolling when mouse is dragged out of window.
  // Currently horizontal scroll is not supported.
  wxTimer* scroll_timer_;

  enum ScrollDir {
    kScrollUp,
    kScrollDown,
  };
  // Scroll direction.
  ScrollDir scroll_dir_;

  // Current text selection.
  Selection selection_;
};

////////////////////////////////////////////////////////////////////////////////

// Text window event.

BEGIN_DECLARE_EVENT_TYPES()
// Check GetInt(), which returns TextWindow::EventType, for the details.
DECLARE_EVENT_TYPE(kTextWindowEvent, 0)
END_DECLARE_EVENT_TYPES()

}  // namespace editor
}  // namespace jil

#define EVT_TEXT_WINDOW(id, func)\
  DECLARE_EVENT_TABLE_ENTRY(jil::editor::kTextWindowEvent, id, -1, \
  wxCommandEventHandler(func), (wxObject*)NULL),

#endif  // JIL_EDITOR_TEXT_WINDOW_H_
