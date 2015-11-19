-- Indent function for Go

go = go or {}

-- Get the previous line which starts with any of the given strings.
go.getPrevLineStartWith = function(buffer, ln, ...)
  for prev_ln = ln-1, 1, -1 do
    local prev_line = buffer:getLine(prev_ln)
    if prev_line:startWith(true, true, ...) then
      return prev_ln
    end
  end
  return 0
end

go.indentByCurrLine = function(buffer, ln)
  local line = buffer:getLine(ln)

  if line:isCommentOnly() then
    return -1
  end

  local l_keys = { '{', '(' }
  local r_keys = { '}', ')' }

  for i = 1, 2 do
    local ok, x = line:startWith(true, true, r_keys[i])
    if ok then
      local p = Point(x, ln)
      p = buffer:getUnpairedLeftKey(p, l_keys[i], r_keys[i])
      if p:valid() then
        return buffer:getIndent(p.y)
      else
        return buffer:getPrevLineIndent(ln, true)
      end
    end
  end

  if line:startWith(true, true, 'case', 'default') then
    local prev_ln = go.getPrevLineStartWith(buffer, ln, 'switch')
    if prev_ln ~= 0 then
      return buffer:getIndent(prev_ln)
    else
      return buffer:getPrevLineIndent(ln, true)
    end
  end

  return -1
end

go.indentByPrevLine = function(buffer, ln, prev_ln)
  local tabStop = buffer:getTabStop()

  local prev_line = buffer:getLine(prev_ln)

  if prev_line:endWith(true, true, '{', '(', ':') then
    return prev_line:getIndent(tabStop) + tabStop
  end

  return -1
end

-- The indent function.
go.indent = function(buffer, ln)
  local indent_size = go.indentByCurrLine(buffer, ln)
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

  local prev_ln = buffer:getPrevNonEmptyLine(ln, true)
  if prev_ln == 0 then
    return 0
  end

  indent_size = go.indentByPrevLine(buffer, ln, prev_ln)
  if indent_size ~= -1 then
    return indent_size
  end

  return buffer:getIndent(prev_ln)
end

