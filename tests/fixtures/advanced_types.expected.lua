-- Advanced type annotations: arrays, generics, nested types
local nums = {1, 2, 3}
local matrix = {{1, 2}, {3, 4}}
local items = nil

-- Generic table types
local scores = {}
local config = {}

-- Combined complex types
local data = nil

-- Global typed assignment
x = 42

-- Varargs function
function sum(a, ...)
  return a
end

print(nums[1], matrix[1][1])
print(items, type(scores), type(config), data)
print(x)
print(sum(1, 2, 3))
