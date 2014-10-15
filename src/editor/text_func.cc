#include "editor/text_func.h"
#include "wx/clipbrd.h"
#include "wx/dataobj.h"
#include "wx/log.h"
#include "pugixml/pugixml.hpp"
#include "editor/util.h"
#include "editor/text_buffer.h"
#include "editor/text_window.h"
#include "editor/action.h"

namespace jil {
namespace editor {

void Move(TextWindow* tw, TextUnit text_unit, SeekType seek_type) {
  tw->Move(text_unit, seek_type);
}

void Delete(TextWindow* tw, TextUnit text_unit, SeekType seek_type) {
  tw->DeleteText(text_unit, seek_type);
}

void Scroll(TextWindow* tw, TextUnit text_unit, SeekType seek_type) {
  tw->ScrollText(text_unit, seek_type);
}

void Select(TextWindow* tw, TextUnit text_unit, SeekType seek_type) {
  tw->SelectText(text_unit, seek_type);
}

void Undo(TextWindow* tw) {
  tw->Undo();
}

void Redo(TextWindow* tw) {
  if (tw->CanRedo()) {
    tw->Redo();
  }
}

////////////////////////////////////////////////////////////////////////////////

// Clipboard

enum ClipboardFormat {
  kClipboard_Normal = 0,
  kClipboard_Line,
  kClipboard_Rect,
};

static const wxDataFormat kLineTextDataFormat("jil_line_textdataformat");
static const wxDataFormat kRectTextDataFormat("jil_rect_textdataformat");

static std::wstring GetClipboardText(ClipboardFormat* clipboard_format) {
  std::wstring text;
  *clipboard_format = kClipboard_Normal;

  if (wxTheClipboard->Open()) {
    if (wxTheClipboard->IsSupported(wxDF_TEXT)) {
      wxTextDataObject data;
      wxTheClipboard->GetData(data);
      text = data.GetText().ToStdWstring();

      if (wxTheClipboard->IsSupported(kLineTextDataFormat)) {
        *clipboard_format = kClipboard_Line;
      } else if (wxTheClipboard->IsSupported(kRectTextDataFormat)) {
        *clipboard_format = kClipboard_Rect;
      }
    }

    wxTheClipboard->Close();
  }

  return text;
}

static bool SetClipboardText(const std::wstring& text,
                             ClipboardFormat clipboard_format) {
  if (!wxTheClipboard->Open()) {
    return false;
  }

  wxTextDataObject* text_data = new wxTextDataObject(wxString(text));

  if (clipboard_format == kClipboard_Normal) {
    wxTheClipboard->SetData(text_data);
  } else {
    wxDataObjectComposite* composite_data = new wxDataObjectComposite;
    composite_data->Add(text_data);

    if (clipboard_format == kClipboard_Line)  {
      composite_data->Add(new wxDataObjectSimple(kLineTextDataFormat));
    } else {  // kClipboard_Rect
      composite_data->Add(new wxDataObjectSimple(kRectTextDataFormat));
    }

    wxTheClipboard->SetData(composite_data);
  }

  wxTheClipboard->Close();
  return true;
}

////////////////////////////////////////////////////////////////////////////////

void Cut(TextWindow* tw) {
  TextBuffer* buffer = tw->buffer();

  std::wstring text;
  ClipboardFormat clipboard_format = kClipboard_Normal;

  const Selection& selection = tw->selection();

  if (!selection.IsEmpty()) {
    if (selection.rect) {
      if (!selection.IsRectEmpty()) {
        clipboard_format = kClipboard_Rect;
        buffer->GetRectText(selection.range, &text);
      }
    } else {
      buffer->GetText(selection.range, &text);
    }

    Delete(tw, kSelected, kWhole);
  } else {
    // Cut the current line.
    TextPoint caret_point = tw->caret_point();
    text = buffer->LineData(caret_point.y);
    clipboard_format = kClipboard_Line;

    if (caret_point.y < buffer->LineCount()) {
      Delete(tw, kLine, kWhole);
      text += GetEol(FF_DEFAULT);
    } else {  // Last line
      // Just clear it.
      if (!text.empty()) {
        Delete(tw, kLine, kWhole);
      }
    }
  }

  if (!text.empty()) {
    SetClipboardText(text, clipboard_format);
  }
}

void Copy(TextWindow* tw) {
  TextBuffer* buffer = tw->buffer();

  std::wstring text;
  ClipboardFormat clipboard_format = kClipboard_Normal;

  const Selection& selection = tw->selection();

  if (!selection.IsEmpty()) {
    if (selection.rect) {
      if (!selection.IsRectEmpty()) {
        clipboard_format = kClipboard_Rect;
        buffer->GetRectText(selection.range, &text);
      }
    } else {
      buffer->GetText(selection.range, &text);
    }
  } else {
    // Copy the current line.
    TextPoint caret_point = tw->caret_point();
    text = buffer->LineData(caret_point.y);
    clipboard_format = kClipboard_Line;

    if (caret_point.y < buffer->LineCount()) {
      text += GetEol(FF_DEFAULT);
    }
  }

  if (!text.empty()) {
    SetClipboardText(text, clipboard_format);
  }
}

void Paste(TextWindow* tw) {
  ClipboardFormat clipboard_format = kClipboard_Normal;
  std::wstring text = GetClipboardText(&clipboard_format);
  if (text.empty()) {
    return;
  }

  tw->Exec(new GroupAction(tw->buffer()));

  const Selection& selection = tw->selection();
  if (!selection.IsEmpty()) {
    DeleteRangeAction* dra = new DeleteRangeAction(tw->buffer(),
                                                   selection.range,
                                                   selection.dir,
                                                   selection.rect,
                                                   true);
    dra->set_caret_point(tw->caret_point());
    dra->set_update_caret(true);
    tw->Exec(dra);
  }

  TextPoint point = tw->caret_point();

  if (clipboard_format == kClipboard_Line) {
    point.x = 0;
  }

  InsertTextAction* ita = new InsertTextAction(tw->buffer(), point, text);
  ita->set_caret_point(tw->caret_point());
  ita->set_update_caret(true);

  if (clipboard_format == kClipboard_Line) {
    ita->set_use_delta(false, true);
  } else if (clipboard_format == kClipboard_Rect) {
    ita->set_use_delta(false, false);
  } else {
    ita->set_use_delta(true, true);
  }

  tw->Exec(ita);

  tw->Exec(new GroupAction(tw->buffer()));
}

void AutoIndent(TextWindow* tw) {
  TextRange text_range = tw->selection().range;
  if (text_range.IsEmpty()) {
    text_range.Set(tw->caret_point(), tw->caret_point());
  }

  if (tw->buffer()->AreLinesAllEmpty(text_range.GetLineRange(), true)) {
    // Don't have to create indent action if all lines are empty.
    return;
  }

  const bool selected = !tw->selection().IsEmpty();
  AutoIndentAction* iia = new AutoIndentAction(tw->buffer(),
                                               text_range,
                                               tw->selection().dir,
                                               tw->selection().rect,
                                               selected);
  iia->set_caret_point(tw->caret_point());
  iia->set_update_caret(true);
  tw->Exec(iia);
}

// If there's selection, indent the selected lines. Otherwise indent the
// current line.
void IncreaseIndent(TextWindow* tw) {
  TextRange text_range = tw->selection().range;
  if (text_range.IsEmpty()) {
    text_range.Set(tw->caret_point(), tw->caret_point());
  }

  if (tw->buffer()->AreLinesAllEmpty(text_range.GetLineRange(), true)) {
    // Don't have to create indent action if all lines are empty.
    return;
  }

  const Selection& selection = tw->selection();
  bool selected = !selection.IsEmpty();
  IncreaseIndentAction* iia = new IncreaseIndentAction(tw->buffer(),
                                                       text_range,
                                                       selection.dir,
                                                       selection.rect,
                                                       selected);
  iia->set_caret_point(tw->caret_point());
  iia->set_update_caret(true);
  tw->Exec(iia);
}

void DecreaseIndent(TextWindow* tw) {
  TextRange text_range = tw->selection().range;
  if (text_range.IsEmpty()) {
    text_range.Set(tw->caret_point(), tw->caret_point());
  }

  if (tw->buffer()->AreLinesAllEmpty(text_range.GetLineRange(), true)) {
    // Don't have to create indent action if all lines are empty.
    return;
  }

  const Selection& selection = tw->selection();
  const bool selected = !selection.IsEmpty();
  DecreaseIndentAction* dia = new DecreaseIndentAction(tw->buffer(),
                                                       text_range,
                                                       selection.dir,
                                                       selection.rect,
                                                       selected);
  dia->set_caret_point(tw->caret_point());
  dia->set_update_caret(true);
  tw->Exec(dia);
}

struct XmlStringWriter : public pugi::xml_writer {
  std::wstring result;

  virtual void write(const void* data, size_t size) override {
    result += std::wstring(static_cast<const wchar_t*>(data),
                           size / sizeof(wchar_t));
  }
};

void Format(TextWindow* tw) {
  TextBuffer* buffer = tw->buffer();

  /*
  std::wstring text;
  buffer->GetText(&text);

  pugi::xml_document xdoc;
  pugi::xml_parse_result result =
      xdoc.load_buffer(text.c_str(),
                       text.size() * sizeof(wchar_t),
                       pugi::parse_full,
                       pugi::encoding_wchar);

  if (!result) {
    wxLogError(wxT("Failed to parse the XML!"));
    return;
  }

  XmlStringWriter writer;
  xdoc.print(writer, "  ", pugi::format_default, pugi::encoding_wchar);

  Coord ln = buffer->LineCount();
  TextRange range(TextPoint(0, 1), TextPoint(buffer->LineLength(ln), ln));
  DeleteRangeAction* dra = new DeleteRangeAction(buffer,
                                                 range,
                                                 kForward,
                                                 false);
  dra->set_grouped(true);
  tw->ClearSelection();
  tw->Exec(dra);

  InsertTextAction* ita = new InsertTextAction(buffer,
                                               tw->caret_point(),
                                               writer.result);
  ita->set_grouped(true);
  tw->Exec(ita);
  */
}

void NewLineBreak(TextWindow* tw) {
  tw->NewLineBreak();
}

void NewLineBelow(TextWindow* tw) {
  tw->NewLineBelow();
}

void NewLineAbove(TextWindow* tw) {
  tw->NewLineAbove();
}

}  // namespace editor
}  // namespace jil
