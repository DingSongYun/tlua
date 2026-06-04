-- Test file for Fix 4: inline annotations should suppress ---@param / ---@return
-- When a function has BOTH inline annotations AND luadoc annotations,
-- the inline ones should win (and the luadoc ones should be silently ignored).

-- Case 1: conflicting param types
-- Inline says: number, luadoc says: string
-- Expected hover on `x` in body: number (inline wins)
---@param x string
local function caseParam(x: number)
    return x  -- hover here: should infer number
end

-- Case 2: conflicting return types
-- Inline says: number, luadoc says: string
-- Expected hover on call site: number
---@return string
local function caseReturn(): number
    return 42
end

local r = caseReturn()  -- hover on r: should be number

-- Case 3: both param and return inline, both luadoc present
---@param a string
---@param b string
---@return string
local function caseBoth(a: number, b: number): number
    return a + b
end

local sum = caseBoth(1, 2)  -- hover on sum: should be number

-- Case 4: pure luadoc (no inline) -- should still work as before
---@param msg string
---@return string
local function pureDocOnly(msg)
    return msg .. "!"
end

local greeting = pureDocOnly("hi")  -- hover on greeting: should be string

-- Case 5: pure inline (no luadoc) -- should still work
local function pureInline(name: string): boolean
    return #name > 0
end

local ok = pureInline("x")  -- hover on ok: should be boolean

print(r, sum, greeting, ok)
