# Text Buffer

Text Buffer 对应于 MVC（Model-View-Controller）里的 M。它是文本在内存中的表示。
你也可以称其为 Text Document，Text Model。因为 Vim 里叫 Buffer，我就沿用了这个名字。

## 字符的表示

`wxString` 是 wxWidgets 提供的字符串类，类似于 MFC 的 `CString`，或 Qt 的 `QString`。
如果定义了宏 `wxUSE_UNICODE`，`wxString` 内部便以 Unicode 表示，但是有两种：
- `wchar_t*`，由宏 `wxUSE_UNICODE_WCHAR` 控制, 此为缺省行为；
- UTF-8 编码的 `char*`，由宏 `wxUSE_UNICODE_UTF8` 控制。

使用 UTF-8，索引相当困难，为了提高性能，可能需要 cache（由宏 `wxUSE_STRING_POS_CACHE` 控制）。
简单来说，UTF-8 版的 `wxString` 目前还处于不可用状态，随意浏览它的代码，会看到注释里有很多 `FIXME`。

就 Text Buffer 来说，用 `wchar_t*` 要比 UTF-8 编码的 `char*` 简单得多，缺点是内存多占了些：VC 一个字符两个字节，GCC 和 Clang 一个字符 4 个字节。

虽然 `wxString` 支持 `wchar_t*` 的实现，Text Buffer 却不必使用 `wxString`。考虑到在 Linux 或 MAC 平台，`wxString` 可能（缺省）会以 UTF-8 实现（也许为了与系统 API 更好的结合），Text Buffer 不应该受到这种平台差异的影响，就目前的设计来说，它应该一直都以 `wchar_t*` 实现。

所以，Text Buffer 最终采用了 `std::wstring`。

并且，后来我意识到，`wxString` 其实只该用来处理界面上的文字，文件路径，等。

## 按行管理

文本始终按行来管理。
一个 Text Buffer，包含多行文本。

```cpp
class TextBuffer {

private:
  std::deque<TextLine*> lines_;
};
```

这里没有用 `vector` 或 `list`，而是用 `deque`，是为了兼顾随机访问和插入删除的效率。
