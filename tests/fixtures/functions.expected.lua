-- Function with typed parameters and return type
function add(a, b)
  return a + b
end

-- Function with no type annotations (passthrough)
function greet(name)
  return "Hello " .. name
end

-- Local function with types
local function multiply(x, y)
  return x * y
end

print(add(1, 2))
print(greet("world"))
print(multiply(3, 4))
