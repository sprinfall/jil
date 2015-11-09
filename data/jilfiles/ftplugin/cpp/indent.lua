
-- TODO: Refactoring
function match(line, str, ignore_comments, ignore_spaces)
  local ok1, off1 = line:startWith(ignore_spaces, str)
  if not ok1 then
    return false
  end

  local ok2, off2 = line:endWith(ignore_comments, ignore_spaces, str)
  if not ok2 then
    return false
  end

  if off1 ~= off2 then
    return false
  end

  return true
end

function isPreprocHead(line)
  local ok, off = line:startWith(true, '#')
  if ok and not line:isCommentOrString(off) then
    return true
  end
  return false
end

function isPreprocBody(buffer, ln)
  for prev_ln = ln-1, 1, -1 do
    local prev_line = buffer:getLine(prev_ln)
    if prev_line:isEolEscaped(true) then
      if isPreprocHead(prev_line) then
        return true
      end
    else
      break
    end
  end
  return false
end

function isPreproc(buffer, ln)
  local line = buffer:getLine(ln)
  return isPreprocHead(line) or isPreprocBody(buffer, ln)
end

-- Always skip empty lines.
function getPrevLine(buffer, ln, skip_comment, skip_preproc)
  local prev_ln = buffer:getPrevNonEmptyLine(ln, skip_comment)
  if skip_preproc then
    while prev_ln > 0 and isPreproc(buffer, prev_ln) do
      prev_ln = buffer:getPrevNonEmptyLine(prev_ln, skip_comment)
    end
  end
  return prev_ln
end

function getPrevLineIndent(buffer, ln, skip_comment, skip_preproc)
  local prev_ln = getPrevLine(buffer, ln, skip_comment, skip_preproc)
  if prev_ln ~= 0 then
    return buffer:getIndent(prev_ln)
  end
  return 0
end

function getPrevLineStartWith(buffer, ln, ...)
  for prev_ln = ln-1, 1, -1 do
    local prev_line = buffer:getLine(prev_ln)
    if prev_line:startWith(true, ...) then
      return prev_ln
    end
  end
  return 0
end

function indentByCurrLine(buffer, ln)
  local line = buffer:getLine(ln)

  if line:isCommentOnly() then
    return -1
  end

  if isPreprocHead(line) then
    return 0  -- No indent for pre-process definition.
  end

  if isPreprocBody(buffer, ln) then
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
      return getPrevLineIndent(buffer, ln, true, true)
    end

    local pair_ln = p.y

    -- Check the char before '{'.
    local pair_line = buffer:getLine(pair_ln)
    x = pair_line:getLastNonSpaceChar(p.x)
    if x ~= -1 then
      -- The char before '{' is ')' (function, if else, switch, etc.).
      if pair_line:isChar(x, ')') then
        p:set(x, p.y)  -- ')'
        p = buffer:getUnpairedLeftKey(p, '(', ')')
        if p:valid() and p.y ~= pair_ln then
          pair_ln = p.y  -- '(' and ')' are not in the same line.
        end
      end
    end

    return buffer:getIndent(pair_ln)
  end

  if line:startWith(true, '{') then
    local prev_ln = getPrevLine(buffer, ln, true, true)
    if prev_ln == 0 then
      return 0
    end

    local prev_line = buffer:getLine(prev_ln)
    x = prev_line:getLastNonSpaceChar(-1)
    if prev_line:isChar(x, ')') then
      local p = Point(x, prev_ln)  -- ')'
      p = buffer:getUnpairedLeftKey(p, '(', ')')
      if p:valid() and p.y ~= prev_ln then
        prev_ln = p.y
      end
    end

    return buffer:getIndent(prev_ln)
  end


  if line:startWith(true, 'else') then
    local prev_ln = getPrevLineStartWith(buffer, ln, 'else', 'if')
    if prev_ln ~= 0 then
      return buffer:getIndent(prev_ln)
    else
      return 0
    end
  end

  if line:startWith(true, 'public', 'protected', 'private') then
    local prev_ln = getPrevLineStartWith(buffer, ln, 'class', 'struct')
    if prev_ln ~= 0 then
      return buffer:getIndent(prev_ln)
    else
      return 0
    end
  end

  if line:startWith(true, 'case', 'default') then
    local idt = 0
    local prev_ln = getPrevLineStartWith(buffer, ln, 'switch')
    if prev_ln ~= 0 then
      idt = buffer:getIndent(prev_ln)
      if buffer:getIndentOption('indent_case'):asBool() then
        idt = idt + buffer:getShiftWidth()
      end
    end
    return idt
  end

  return -1
end

function indentByPrevLine(buffer, ln)
  local prev_ln = getPrevLine(buffer, ln, true, true)
  if prev_ln == 0 then
    return 0
  end

  local tabStop = buffer:getTabStop()
  local shiftWidth = buffer:getShiftWidth()

  local prev_line = buffer:getLine(prev_ln)

  local ok, x = prev_line:endWith(true, true, '{')
  if ok then
    local j = prev_line:getLastNonSpaceChar(x)
    if j == -1 then
      -- No char before '{', check the previous line.
      prev_ln = buffer:getPrevNonEmptyLine(prev_ln, true)
      if prev_ln == 0 then  -- No previous line.
        return prev_line:getIndent(tabStop) + shiftWidth  -- TODO: Return?
      end
      prev_line = buffer:getLine(prev_ln)
      j = prev_line:getLastNonSpaceChar(-1)  -- j must be valid.
    end

    -- The char before '{' is ')', find the line with the paired '('.
    if prev_line:isChar(j, ')') then
      local p = Point(j, prev_ln)  -- ')'
      p = buffer:getUnpairedLeftKey(p, '(', ')')
      if p:valid() and p.y ~= prev_ln then
        prev_ln = p.y
        prev_line = buffer:getLine(p.y)
      end
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

  if match(prev_line, 'else', true, true) then
    return prev_line:getIndent(tabStop) + shiftWidth
  end

  -- The EOL of previous line is escaped.
  --if prev_line:endWith(true, true, '\\') then
  --  return prev_line:getIndent(tabStop) + shiftWidth
  --end

  return -1
end

-- Determine indent by the line before previous line.
-- Handle the cases like this:
-- if (a > b)
--     return b;
-- else             (prev_prev_line)
--     return a;    (prev_line)
-- int i;           (line)
function indentByPrevPrevLine(buffer, prev_ln)
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
  elseif match(pprev_line, 'else', true, true) then
    if not pprev_line:endWith(true, true, '{') then
      return pprev_line:getIndent(tabStop)
    end
  end

  return -1
end

function indent(buffer, ln)
  local indent_size = indentByCurrLine(buffer, ln)
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

  indent_size = indentByPrevLine(buffer, ln)
  if indent_size ~= -1 then
    return indent_size
  end

  local prev_ln = getPrevLine(buffer, ln, true, true)
  indent_size = indentByPrevPrevLine(buffer, prev_ln)
  if indent_size ~= -1 then
    return indent_size
  end

  return buffer:getIndent(prev_ln)
end
