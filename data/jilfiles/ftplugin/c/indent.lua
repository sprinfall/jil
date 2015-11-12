-- Indent function for C

-- All functions are defined under this "namespace" to avoid conflict
-- among different file types.
c = c or {}

dofile('../cpp/indent.lua')

-- Reuse cpp indent function.
c.indent = cpp.indent
