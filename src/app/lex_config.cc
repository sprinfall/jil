#include "app/lex_config.h"
#include <string>
#include <vector>
#include "boost/algorithm/string.hpp"
#include "wx/log.h"
#include "wx/filename.h"
#include "editor/lex.h"
#include "editor/ft_plugin.h"
#include "app/config.h"

namespace jil {

using namespace editor;

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
  } else if (major_str == "special") {
    return Lex(kLexSpecial);
  } else {
    return Lex(kLexNone);
  }
}

Quote* ParseQuoteSetting(Lex lex,
                         const Setting& quote_setting,
                         const std::string& flags_str) {
  if (quote_setting.type() != Setting::kArray) {
    return NULL;
  }

  std::string start;
  std::string end;

  if (quote_setting.size() > 0) {
    start = quote_setting[0].GetString();
    if (quote_setting.size() > 1) {
      end = quote_setting[1].GetString();
    }
  }

  if (start.empty()) {
    wxLogError(wxT("Lex quote start is empty!"));
    return NULL;
  }

#if JIL_LEX_REGEX_QUOTE_START
  bool regex = false;
#endif  // JIL_LEX_REGEX_QUOTE_START

  int flags = 0;

  if (!flags_str.empty()) {
    std::vector<std::string> flag_str_array;
    boost::split(flag_str_array,
                 flags_str,
                 boost::is_any_of(", "),
                 boost::token_compress_on);

    for (size_t i = 0; i < flag_str_array.size(); ++i) {
      std::string& flag_str = flag_str_array[i];
      if (flag_str == "multi_line") {
        flags |= Quote::kMultiLine;
      } else if (flag_str == "escape_eol") {
        flags |= Quote::kEscapeEol;
#if JIL_LEX_REGEX_QUOTE_START
      } else if (flag_str == "regex_start") {
        regex = true;
#endif  // JIL_LEX_REGEX_QUOTE_START
      } else {
        wxLogError(wxT("Unknown quote flag: %s!"), flag_str.c_str());
      }
    }
  }

  // Convert to std::wstring.
  std::wstring w_start(start.begin(), start.end());
  std::wstring w_end(end.begin(), end.end());

#if JIL_LEX_REGEX_QUOTE_START
  Quote* quote = regex ? (new RegexQuote) : (new Quote);
  quote->Set(lex, w_start, w_end, flags);
#else
  Quote* quote = new Quote(lex, w_start, w_end, flags);
#endif  // JIL_LEX_REGEX_QUOTE_START

  return quote;
}

}  // namespace

bool LoadLexFile(const wxString& lex_file, FtPlugin* ft_plugin) {
  if (!wxFileName::FileExists(lex_file)) {
    wxLogInfo(wxT("Lex file for [%s] doesn't exist."),
              ft_plugin->id().c_str());
    return false;
  }

  Config lex_cfg;
  if (!lex_cfg.Load(lex_file)) {
    wxLogError(wxT("Failed to parse lex file for [%s]!"),
               ft_plugin->id().c_str());
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
      Quote* quote = ParseQuoteSetting(lex,
                                       quote_setting,
                                       lex_setting.GetString("flags"));
      if (quote != NULL) {
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
      // Assume that the string is pure ascii.
      std::wstring regex_str(str, str + strlen(str));
      ft_plugin->AddRegex(lex, regex_str);
      continue;
    }

    Setting prefix_setting = lex_setting.Get("prefix");
    if (prefix_setting) {
      const char* str = prefix_setting.GetString();
      // Assume that the string is pure ascii.
      std::wstring prefix_str(str, str + strlen(str));
      ft_plugin->AddPrefix(lex, prefix_str);
      continue;
    }

    Setting suffix_setting = lex_setting.Get("suffix");
    if (suffix_setting) {
      const char* str = suffix_setting.GetString();
      // Assume that the string is pure ascii.
      std::wstring suffix_str(str, str + strlen(str));
      ft_plugin->AddSuffix(lex, suffix_str);
      continue;
    }
  }

#if !JIL_MATCH_WORD_WITH_HASH
  ft_plugin->SortAnyof();
#endif

  return true;
}

}  // namespace jil
