-- Complex type annotations
local x = 42
local y = nil
local callback = nil
local t = {}

-- Multiple typed variables
local a, b = 1, "hello"

-- Function with complex types
function process(data, cb)
  return 0
end

print(x, y, a, b)
print(process({}, nil))
