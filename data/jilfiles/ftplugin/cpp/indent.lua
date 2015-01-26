
function getprevlineindent(buffer, ln)
  local prev_ln = buffer:prevnonemptyline(ln, true)
  if prev_ln == 0 then
    return 0
  else
    return buffer:getindent(prev_ln)
  end
end

function indent(buffer, ln)
  -- Indent options.
  local indent_namespace = false
  local indent_case = false

  -- TODO
  local tab_stop = 4
  local shift_width = 4

  local x = -1

  local line = buffer:getline(ln)

  ------------------------------------------------------------------------------

  if line:startwith('#', true) then
    -- No indent for macro definition.
    return 0
  end

  ------------------------------------------------------------------------------

  print("Check line start with }")

  local ok, x = line:startwith('}', true)
  if ok then
    -- If the current line starts with '}', indent the same as the line with
    -- the paired '{'.

    -- Find '{'.
    local p = Point(x, ln)  -- '}'
    p = buffer:unpairedleftkey(p, '{', '}')

    if not p:isvalid() then
      -- Can't find '{', indent the same as previous line.
      return getprevlineindent(buffer, ln)
    end

    -- The line with '{'.
    local temp_ln = p.y
    local temp_line = buffer:getline(temp_ln)

    -- Check the char before '{'.
    x = temp_line:lastnonspacechar(p.x)

    if x == -1 then
      -- No char before '{'.
      -- NOTE: Can't use p.x because there might be tabs.
      return temp_line:getindent(tab_stop)
    end

    if temp_line:ischar(x, ')') then
      -- The char before '{' is ')'. (function, if else, switch, etc.)
      p:set(x, temp_ln)  -- ')'
      p = buffer:unpairedleftkey(p, '(', ')', false)

      if p:isvalid() and p.y ~= temp_ln then
        -- Find '(' and '(' and ')' are not in the same line.
        -- Indent the same as the line with '('.
        return buffer:getindent(p.y)
      end

      -- Indent the same as the line with ')'.
      return temp_line:getindent(tab_stop)
    end

    -- Indent the same as the line ending with '{'.
    return temp_line:getindent(tab_stop)
  end

  ------------------------------------------------------------------------------

  if line:startwith('{', true) then
    local prev_ln = buffer:prevnonemptyline(ln, true)
    if prev_ln == 0 then
      return 0
    end

    local prev_line = buffer:getline(prev_ln)

    x = prev_line:lastnonspacechar(-1)

    -- function, if else, switch, etc.
    if prev_line:ischar(x, ')') then
      local p = Point(x, prev_ln)  -- ')'
      p = buffer:unpairedleftkey(p, '(', ')', false)

      if p:isvalid() and p.y ~= prev_ln then
        prev_ln = p.y
        prev_line = buffer:getline(prev_ln)
      end
    end

    return prev_line:getindent(tab_stop)
  end

  ------------------------------------------------------------------------------

  -- By default, use the same indent as the previous line.
  local prev_ln = buffer:prevnonemptyline(ln, true)
  if prev_ln == 0 then
    return 0
  end
  prev_line = buffer:getline(prev_ln)
  return prev_line:getindent(tab_stop)
end
