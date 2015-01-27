
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
  local indent_namespace = buffer:getindentoption("indent_namespace"):asboolean()
  local indent_case = buffer:getindentoption("indent_case"):asboolean()

  local tabstop = buffer:tabstop()
  local shiftwidth = buffer:shiftwidth()

  local x = -1

  local line = buffer:getline(ln)

  ------------------------------------------------------------------------------

  if line:startwith('#', true) then
    -- No indent for macro definition.
    return 0
  end

  ------------------------------------------------------------------------------

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
      return temp_line:getindent(tabstop)
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
      return temp_line:getindent(tabstop)
    end

    -- Indent the same as the line ending with '{'.
    return temp_line:getindent(tabstop)
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

    return prev_line:getindent(tabstop)
  end

  ------------------------------------------------------------------------------

  -- By default, use the same indent as the previous line.
  local prev_ln = buffer:prevnonemptyline(ln, true)
  if prev_ln == 0 then
    return 0
  end
  prev_line = buffer:getline(prev_ln)

  -- TEST
  if indent_namespace then
    return prev_line:getindent(tabstop) + shiftwidth
  else
    return prev_line:getindent(tabstop)
  end

end
