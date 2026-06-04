-- Launcher used by VSCode + actboy168.lua-debug for TypingLua files.
--
-- It executes the stripped Lua file, but gives the loaded chunk a synthetic
-- source name that points at the original typed .lua file and embeds the
-- stripped source text. actboy168.lua-debug understands this source-name form:
--
--   --@<source-path>:<start-line>\n<source-content>
--
-- This lets breakpoints set in the original typed file verify against the
-- stripped Lua content instead of failing to parse inline type annotations.

local generated = arg and arg[1]
local original = arg and arg[2]

if not generated or not original then
    error('Usage: tlua-debug-launcher.lua <generated.lua> <original-relative-path> [args...]', 0)
end

local function readAll(path)
    local f, err = io.open(path, 'rb')
    if not f then
        error(("Cannot read '%s': %s"):format(path, err), 0)
    end
    local content = f:read('a')
    f:close()
    return content
end

local code = readAll(generated)

-- Important: keep this path relative. actboy168.lua-debug's inline source
-- parser uses ':' before the line number, so absolute Windows paths like
-- D:/... would be ambiguous. The launch config passes ${relativeFile} here.
local sourcePath = original:gsub('\\', '/')
local chunkName = ('--@%s:1\n%s'):format(sourcePath, code)

-- Prefix one blank line when compiling. actboy168.lua-debug records nested
-- function proto ranges from Lua's debug info, but computes inline-source proto
-- ranges with a +1 adjustment internally. The leading newline makes those two
-- ranges line up while source.line() maps runtime lines back to the original
-- typed source lines.
local fn, err = load('\n' .. code, chunkName)
if not fn then
    error(err, 0)
end

-- Make arg look like the original script was launched directly.
local scriptArgs = {}
_G.arg = { [0] = sourcePath }
for i = 3, #arg do
    local n = i - 2
    scriptArgs[n] = arg[i]
    _G.arg[n] = arg[i]
end

return fn(table.unpack(scriptArgs))
