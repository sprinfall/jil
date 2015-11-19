-- Indent function for Cfg (libconfig)

cfg = cfg or {}

cfg.indentByCurrLine = function(buffer, ln)
  local line = buffer:getLine(ln)

  if line:isCommentOnly() then
    return -1
  end

  local l_keys = { '{', '(', '[' }
  local r_keys = { '}', ')', ']' }

  for i = 1, 3 do
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

  return -1
end

cfg.indentByPrevLine = function(buffer, ln)
  local prev_ln = buffer:getPrevNonEmptyLine(ln, true)
  if prev_ln == 0 then
    return 0
  end

  local tab_stop = buffer:getTabStop()

  local prev_line = buffer:getLine(prev_ln)
  if prev_line:endWith(true, true, '{', '(', '[') then
    return prev_line:getIndent(tab_stop) + tab_stop
  end

  return -1
end

cfg.indent = function(buffer, ln)
  local indent_size = cfg.indentByCurrLine(buffer, ln)
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

  indent_size = cfg.indentByPrevLine(buffer, ln)
  if indent_size ~= -1 then
    return indent_size
  end

  local prev_ln = buffer:getPrevNonEmptyLine(ln, true)
  return buffer:getIndent(prev_ln)
end
