#include "jil/lex_config.h"
#include <string>
#include <vector>
#include "boost/algorithm/string.hpp"
#include "wx/log.h"
#include "wx/filename.h"
#include "editor/lex.h"
#include "editor/ft_plugin.h"
#include "jil/config.h"

using namespace editor;

namespace jil {

namespace {

LexConstant ParseLexConstant(const std::string& minor_str) {
  if (minor_str.empty()) {
    return kLexConstantOther;
  }

  if (minor_str == "char") {
    return kLexConstantChar;
  } else if (minor_str == "string") {
    return kLexConstantString;
  } else if (minor_str == "number") {
    return kLexConstantNumber;
  } else if (minor_str == "bool") {
    return kLexConstantBool;
  } else if (minor_str == "null") {
    return kLexConstantNull;
  } else {
    return kLexConstantOther;
  }
}

LexIdentifier ParseLexIdentifier(const std::string& minor_str) {
  if (minor_str.empty()) {
    return kLexIdentifierOther;
  }

  if (minor_str == "function") {
    return kLexIdentifierFunction;
  } else if (minor_str == "class") {
    return kLexIdentifierClass;
  } else {
    return kLexIdentifierOther;
  }
}

LexStatement ParseLexStatement(const std::string& minor_str) {
  if (minor_str.empty()) {
    return kLexStatementOther;
  }

  if (minor_str == "conditional") {
    return kLexStatementConditional;
  } else if (minor_str == "repeat") {
    return kLexStatementRepeat;
  } else if (minor_str == "operator") {
    return kLexStatementOperator;
  } else if (minor_str == "exception") {
    return kLexStatementException;
  } else {
    return kLexStatementOther;
  }
}

LexType ParseLexType(const std::string& minor_str) {
  if (minor_str.empty()) {
    return kLexTypeOther;
  }

  if (minor_str == "qualifier") {
    return kLexTypeQualifier;
  } else if (minor_str == "struct") {
    return kLexTypeStruct;
  } else {
    return kLexTypeOther;
  }
}

Lex ParseLex(const char* lex_str) {
  std::string major_str = lex_str;
  std::string minor_str;

  size_t dot_index = major_str.find('.', 0);
  if (dot_index != std::string::npos) {
    minor_str = major_str.substr(dot_index + 1);
    major_str = major_str.substr(0, dot_index);
  }

  if (major_str == "comment") {
    return Lex(kLexComment);
  } else if (major_str == "constant") {
    return Lex(kLexConstant, ParseLexConstant(minor_str));
  } else if (major_str == "identifier") {
    return Lex(kLexIdentifier, ParseLexIdentifier(minor_str));
  } else if (major_str == "statement") {
    return Lex(kLexStatement, ParseLexStatement(minor_str));
  } else if (major_str == "package") {
    return Lex(kLexPackage);
  } else if (major_str == "preproc") {
    return Lex(kLexPreProc);
  } else if (major_str == "type") {
    return Lex(kLexType, ParseLexType(minor_str));
  } else if (major_str == "url") {
    return Lex(kLexUrl);
  } else if (major_str == "error") {
    return Lex(kLexError);
  } else {
    return Lex(kLexNone);
  }
}

bool ParseQuoteSetting(const Setting& quote_setting,
                       std::wstring* start,
                       std::wstring* end) {
  if (quote_setting.type() != Setting::kArray) {
    return false;
  }

  if (quote_setting.size() > 0) {
    const char* start_cstr = quote_setting[0].GetString();
    start->insert(start->end(), start_cstr, start_cstr + strlen(start_cstr));
    if (quote_setting.size() > 1) {
      const char* end_cstr = quote_setting[1].GetString();
      end->insert(end->end(), end_cstr, end_cstr + strlen(end_cstr));
    }
  }

  if (start->empty()) {
    return false;
  }

  return true;
}

}  // namespace

bool LoadLexFile(const wxString& lex_cfg_file, FtPlugin* ft_plugin) {
  if (!wxFileName::FileExists(lex_cfg_file)) {
    wxLogInfo(wxT("Lex file for [%s] doesn't exist."), ft_plugin->id().c_str());
    return false;
  }

  Config lex_cfg;
  if (!lex_cfg.Load(lex_cfg_file)) {
    wxLogError(wxT("Failed to parse lex file for [%s]!"), ft_plugin->id().c_str());
    return false;
  }

  Setting root_setting = lex_cfg.Root();

  bool ignore_case = root_setting.GetBool("ignore_case");
  ft_plugin->SetIgnoreCase(ignore_case);

  Setting lex_list_setting = root_setting.Get("lex", Setting::kList, false);
  if (!lex_list_setting) {
    wxLogError(wxT("Cannot get lex list!"));
    return false;
  }

  const int count = lex_list_setting.size();

  for (int i = 0; i < count; ++i) {
    Setting lex_setting = lex_list_setting.Get(i);

    Lex lex = ParseLex(lex_setting.GetString("id"));
    if (lex.IsEmpty()) {
      wxLogError(wxT("Lex is none!"));
      continue;
    }

    Setting quote_setting = lex_setting.Get("quote");
    if (quote_setting) {
      // Optional name.
      std::string name = lex_setting.GetString("name");

      std::wstring start;
      std::wstring end;
      if (!ParseQuoteSetting(quote_setting, &start, &end)) {
        continue;
      }

      int flags = 0;
      if (lex_setting.GetBool("multi_line")) {
        flags |= kQuoteMultiLine;
      }
      if (lex_setting.GetBool("escape_eol")) {
        flags |= kQuoteEscapeEol;
      }

      if (lex_setting.GetBool("use_regex")) {
        RegexQuote* regex_quote = new RegexQuote(lex, start, end, flags);
        regex_quote->set_name(name);
        ft_plugin->AddRegexQuote(regex_quote);
      } else {
        Quote* quote = new Quote(lex, start, end, flags);
        quote->set_name(name);
        ft_plugin->AddQuote(quote);
      }

      continue;
    }

    Setting anyof_setting = lex_setting.Get("anyof");
    if (anyof_setting) {
      const char* str = anyof_setting.GetString();
      // Assume that the string is pure ascii.
      std::wstring anyof_str(str, str + strlen(str));
      ft_plugin->AddAnyof(lex, anyof_str);
      continue;
    }

    Setting regex_setting = lex_setting.Get("regex");
    if (regex_setting) {
      const char* str = regex_setting.GetString();
      std::wstring regex_str(str, str + strlen(str));
      ft_plugin->AddRegex(lex, regex_str);
      continue;
    }

    Setting prefix_setting = lex_setting.Get("prefix");
    if (prefix_setting) {
      const char* str = prefix_setting.GetString();
      std::wstring prefix_str(str, str + strlen(str));

      if (lex_setting.GetBool("use_regex")) {
        ft_plugin->AddRegexPrefix(lex, prefix_str);
      } else {
        ft_plugin->AddPrefix(lex, prefix_str);
      }

      continue;
    }

    Setting prev_setting = lex_setting.Get("prev");
    if (prev_setting) {
      const char* str = prev_setting.GetString();
      std::wstring prev_str(str, str + strlen(str));
      ft_plugin->AddPrev(lex, prev_str);
      continue;
    }

    Setting next_setting = lex_setting.Get("next");
    if (next_setting) {
      const char* str = next_setting.GetString();
      std::wstring next_str(str, str + strlen(str));
      ft_plugin->AddNext(lex, next_str);
      continue;
    }
  }

#if !JIL_MATCH_WORD_WITH_HASH
  ft_plugin->SortAnyof();
#endif

  return true;
}

}  // namespace jil
