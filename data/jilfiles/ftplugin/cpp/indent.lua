-- Indent function for C++

-- All functions are defined under this "namespace" to avoid conflict
-- among different file types.
cpp = cpp or {}

cpp.isPreprocHead = function(line)
  local ok, off = line:startWith(true, '#')
  if ok and not line:isCommentOrString(off) then
    return true
  end
  return false
end

cpp.isPreprocBody = function(buffer, ln)
  for prev_ln = ln-1, 1, -1 do
    local prev_line = buffer:getLine(prev_ln)
    if prev_line:isEolEscaped(true) then
      if cpp.isPreprocHead(prev_line) then
        return true
      end
    else
      break
    end
  end
  return false
end

-- Determine if the line is pre-processing (macro, etc.)
cpp.isPreproc = function(buffer, ln)
  local line = buffer:getLine(ln)
  return cpp.isPreprocHead(line) or cpp.isPreprocBody(buffer, ln)
end

-- Get the previous non-empty line by optionally skipping comment and
-- pre-processing lines.
cpp.getPrevLine = function(buffer, ln, skip_comment, skip_preproc)
  local prev_ln = buffer:getPrevNonEmptyLine(ln, skip_comment)
  if skip_preproc then
    while prev_ln > 0 and cpp.isPreproc(buffer, prev_ln) do
      prev_ln = buffer:getPrevNonEmptyLine(prev_ln, skip_comment)
    end
  end
  return prev_ln
end

-- Get the indent of the previous non-empty line.
cpp.getPrevLineIndent = function(buffer, ln, skip_comment, skip_preproc)
  local prev_ln = cpp.getPrevLine(buffer, ln, skip_comment, skip_preproc)
  if prev_ln ~= 0 then
    return buffer:getIndent(prev_ln)
  end
  return 0
end

-- Get the previous line which starts with any of the given strings.
cpp.getPrevLineStartWith = function(buffer, ln, ...)
  for prev_ln = ln-1, 1, -1 do
    local prev_line = buffer:getLine(prev_ln)
    if prev_line:startWith(true, ...) then
      return prev_ln
    end
  end
  return 0
end

-- Current line has '{' at index 'x', and '{' starts a block.
-- This function returns the head line of the block.
-- Example:
-- if (condition1 &&  // return this line
--     condition2 &&
--     condition3) {  // current line
--                 x
cpp.getBlockHead = function(buffer, curr_ln, x)
  local ln = curr_ln
  local line = buffer:getLine(ln)

  local j = line:findLastNonSpace(x)
  if j == -1 then
    -- No char before '{', check the previous line.
    ln = cpp.getPrevLine(buffer, ln, true, true)
    if ln == 0 then
      return curr_ln
    else
      line = buffer:getLine(ln)
      j = line:findLastNonSpace(-1)  -- j must be valid.
    end
  end

  if line:isChar(j, ')') then
    -- The char before '{' is ')', find the line with the paired '('.
    local p = Point(j, ln)  -- ')'
    p = buffer:getUnpairedLeftKey(p, '(', ')')
    if p:valid() then
      return p.y
    end
  end

  -- The char before '{' is not ')'.
  return ln

--  local k = line:findLastChar(')', true, j+1)
--  if k ~= -1 then
end

-- Check the current line to determine the indent.
cpp.indentByCurrLine = function(buffer, ln)
  local line = buffer:getLine(ln)

  if line:isCommentOnly() then
    return -1
  end

  if cpp.isPreprocHead(line) then
    return 0  -- No indent for pre-process definition.
  end

  if cpp.isPreprocBody(buffer, ln) then
    if buffer:getIndentOption('indent_preproc_body'):asBool() then
      return buffer:getShiftWidth()
    else
      return 0  -- The same as pre-proc head.
    end
  end

  local ok, x = line:startWith(true, '}')
  if ok then
    -- Try to find '{'.
    local p = Point(x, ln)  -- '}'
    p = buffer:getUnpairedLeftKey(p, '{', '}')

    -- Can't find '{', indent the same as previous line.
    if not p:valid() then
      return cpp.getPrevLineIndent(buffer, ln, true, true)
    else
      local head_ln = cpp.getBlockHead(buffer, p.y, p.x)
      return buffer:getIndent(head_ln)
    end
  end

  if line:startWith(true, '{') then
    local head_ln = cpp.getBlockHead(buffer, ln, 0)
    if head_ln == ln then
      return 0
    else
      return buffer:getIndent(head_ln)
    end
  end

  if line:startWith(true, 'else') then
    local prev_ln = cpp.getPrevLineStartWith(buffer, ln, 'else', 'if')
    if prev_ln ~= 0 then
      return buffer:getIndent(prev_ln)
    else
      return 0
    end
  end

  if line:startWith(true, 'public', 'protected', 'private') then
    local prev_ln = cpp.getPrevLineStartWith(buffer, ln, 'class', 'struct')
    if prev_ln ~= 0 then
      return buffer:getIndent(prev_ln)
    else
      return 0
    end
  end

  if line:startWith(true, 'case', 'default') then
    local indent_size = 0
    local prev_ln = cpp.getPrevLineStartWith(buffer, ln, 'switch')
    if prev_ln ~= 0 then
      indent_size = buffer:getIndent(prev_ln)
      if buffer:getIndentOption('indent_case'):asBool() then
        indent_size = indent_size + buffer:getShiftWidth()
      end
    end
    return indent_size
  end

  return -1
end

-- Check the previous line to determine the indent.
cpp.indentByPrevLine = function(buffer, ln)
  local prev_ln = cpp.getPrevLine(buffer, ln, true, true)
  if prev_ln == 0 then
    return 0
  end

  local tabStop = buffer:getTabStop()
  local shiftWidth = buffer:getShiftWidth()

  local prev_line = buffer:getLine(prev_ln)

  local ok, x = prev_line:endWith(true, true, '{')
  if ok then
    local head_ln = cpp.getBlockHead(buffer, prev_ln, x)
    if head_ln ~= prev_ln then
      prev_ln = head_ln
      prev_line = buffer:getLine(prev_ln)
    end

    if not buffer:getIndentOption('indent_namespace'):asBool() then
      if prev_line:startWith(true, 'namespace') then
        return prev_line:getIndent(tabStop)
      end
    end

    return prev_line:getIndent(tabStop) + shiftWidth
  end

  -- public:, protected:, private:, case label:, etc.
  if prev_line:endWith(true, true, ':') then
    return prev_line:getIndent(tabStop) + shiftWidth
  end

  -- Handle multi-line if, for and function parameters.
  -- E.g.,
  -- if (name == "a" &&
  --     name != "b")
  x = prev_line:getUnpairedLeftKey('(', ')', -1)
  if x ~= -1 then
    -- Indent the same as the first non-space char after the '('.
    return prev_line:getTabbedLength(tabStop, x + 1)
  end

  ok, x = prev_line:endWith(true, true, ')')
  if ok then
    local pair_line = prev_line
    local p = Point(x, prev_ln)
    p = buffer:getUnpairedLeftKey(p, '(', ')')
    if p:valid() and p.y ~= prev_ln then
      pair_line = buffer:getLine(p.y)
    end

    if pair_line:startWith(true, 'if', 'else if', 'while', 'for') then
      return pair_line:getIndent(tabStop) + shiftWidth
    end
  end

  if prev_line:equal('else', true, true) then
    return prev_line:getIndent(tabStop) + shiftWidth
  end

  -- The EOL of previous line is escaped.
  --if prev_line:endWith(true, true, '\\') then
  --  return prev_line:getIndent(tabStop) + shiftWidth
  --end

  return -1
end

-- Check the line before previous line to determine the indent.
-- Handle the cases like this:
-- if (a > b)
--     return b;
-- else             (prev_prev_line)
--     return a;    (prev_line)
-- int i;           (line)
cpp.indentByPrevPrevLine = function(buffer, prev_ln)
  local pprev_ln = buffer:getPrevNonEmptyLine(prev_ln, true)
  if pprev_ln == 0 then
    return -1
  end

  local tabStop = buffer:getTabStop()

  local pprev_line = buffer:getLine(pprev_ln)
  if pprev_line:startWith(true, 'if', 'else if', 'while', 'for') then
    if pprev_line:getUnpairedLeftKey('(', ')', -1) == -1 then
      if not pprev_line:endWith(true, true, '{') then
        return pprev_line:getIndent(tabStop)
      end
    end
  elseif pprev_line:equal('else', true, true) then
    if not pprev_line:endWith(true, true, '{') then
      return pprev_line:getIndent(tabStop)
    end
  end

  return -1
end

-- The indent function.
cpp.indent = function(buffer, ln)
  local indent_size = cpp.indentByCurrLine(buffer, ln)
  if indent_size ~= -1 then
    return indent_size
  end

  local line = buffer:getLine(ln)
  if line:isCommentOnly() then
    local prev_ln = buffer:getPrevNonEmptyLine(ln, false)
    if prev_ln ~= 0 then
      local prev_line = buffer:getLine(prev_ln)
      if prev_line:isCommentOnly() then
        return prev_line:getIndent(buffer:getTabStop())
      end
    end
  end

  indent_size = cpp.indentByPrevLine(buffer, ln)
  if indent_size ~= -1 then
    return indent_size
  end

  local prev_ln = cpp.getPrevLine(buffer, ln, true, true)
  indent_size = cpp.indentByPrevPrevLine(buffer, prev_ln)
  if indent_size ~= -1 then
    return indent_size
  end

  return buffer:getIndent(prev_ln)
end

