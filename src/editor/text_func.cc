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

static const wxDataFormat kCustomTextDataFormat("jilcustomtextdataformat");

static std::wstring GetClipboardText(bool* by_line) {
  std::wstring text;
  *by_line = false;

  if (wxTheClipboard->Open()) {
    if (wxTheClipboard->IsSupported(wxDF_TEXT)) {
      wxTextDataObject data;
      wxTheClipboard->GetData(data);
      text = data.GetText().ToStdWstring();

      if (wxTheClipboard->IsSupported(kCustomTextDataFormat)) {
        *by_line = true;
      }
    }

    wxTheClipboard->Close();
  }

  return text;
}

static bool SetClipboardText(const std::wstring& text, bool by_line) {
  if (!wxTheClipboard->Open()) {
    return false;
  }

  wxTextDataObject* text_data = new wxTextDataObject(wxString(text));

  if (!by_line) {
    wxTheClipboard->SetData(text_data);
  } else {
    wxDataObjectComposite* composite_data = new wxDataObjectComposite;
    composite_data->Add(text_data);
    composite_data->Add(new wxDataObjectSimple(kCustomTextDataFormat));
    wxTheClipboard->SetData(composite_data);
  }

  wxTheClipboard->Close();
  return true;
}

////////////////////////////////////////////////////////////////////////////////

void Cut(TextWindow* tw) {
  TextBuffer* buffer = tw->buffer();

  std::wstring text;
  bool by_line = false;

  if (!tw->selection().IsEmpty()) {
    buffer->GetText(tw->selection().range, &text);
    Delete(tw, kSelected, kWhole);
  } else {
    // Cut the current line.
    TextPoint caret_point = tw->caret_point();
    text = buffer->LineData(caret_point.y);
    by_line = true;

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
    SetClipboardText(text, by_line);
  }
}

void Copy(TextWindow* tw) {
  TextBuffer* buffer = tw->buffer();

  std::wstring text;
  bool by_line = false;

  if (!tw->selection().IsEmpty()) {
    buffer->GetText(tw->selection().range, &text);
  } else {
    // Copy the current line.
    TextPoint caret_point = tw->caret_point();
    text = buffer->LineData(caret_point.y);
    by_line = true;

    if (caret_point.y < buffer->LineCount()) {
      text += GetEol(FF_DEFAULT);
    }
  }

  if (!text.empty()) {
    SetClipboardText(text, by_line);
  }
}

void Paste(TextWindow* tw) {
  bool by_line = false;
  std::wstring text = GetClipboardText(&by_line);
  if (text.empty()) {
    return;
  }

  if (tw->selection().IsEmpty()) {
    TextPoint point = tw->caret_point();
    if (by_line) {
      point.x = 0;
    }
    InsertTextAction* ita = new InsertTextAction(tw->buffer(), point, text);
    ita->set_caret_point(tw->caret_point());
    ita->set_update_caret(true);
    tw->Exec(ita);
  } else {
    DeleteRangeAction* dra = new DeleteRangeAction(tw->buffer(),
                                                   tw->selection().range,
                                                   tw->selection().dir,
                                                   true);
    dra->set_caret_point(tw->caret_point());
    dra->set_update_caret(true);
    dra->set_grouped(true);
    tw->Exec(dra);

    InsertTextAction* ita = new InsertTextAction(tw->buffer(),
                                                 tw->caret_point(),
                                                 text);
    ita->set_caret_point(tw->caret_point());
    ita->set_update_caret(true);
    ita->set_grouped(true);
    tw->Exec(ita);
  }
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

  const bool selected = !tw->selection().IsEmpty();
  IncreaseIndentAction* iia = new IncreaseIndentAction(tw->buffer(),
                                                       text_range,
                                                       tw->selection().dir,
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

  const bool selected = !tw->selection().IsEmpty();
  DecreaseIndentAction* dia = new DecreaseIndentAction(tw->buffer(),
                                                       text_range,
                                                       tw->selection().dir,
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
