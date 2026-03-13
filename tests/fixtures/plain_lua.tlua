-- Pure Lua 5.4 code — no type annotations
-- Should pass through the transpiler unchanged

local function fibonacci(n)
  if n <= 1 then
    return n
  end
  return fibonacci(n - 1) + fibonacci(n - 2)
end

for i = 0, 10 do
  print(string.format("fib(%d) = %d", i, fibonacci(i)))
end

-- Table operations
local t = {}
for i = 1, 5 do
  t[i] = i * i
end

-- Method-style call (colon syntax)
local s = "hello world"
print(s:upper())
print(s:sub(1, 5))
