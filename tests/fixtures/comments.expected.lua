-- Comments should be preserved exactly
-- This is a line comment
local x = 1 -- inline comment

--[[ This is a
block comment ]]
local y = 2

-- Whitespace and indentation preserved
if true then
  local z = 3
  -- nested comment
  print(z)
end
