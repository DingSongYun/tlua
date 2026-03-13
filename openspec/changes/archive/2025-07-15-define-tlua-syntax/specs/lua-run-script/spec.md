## ADDED Requirements

### Requirement: Execute transpiled .tlua output
The compiled `lua.exe` SHALL be able to execute `.lua` files that were produced by transpiling `.tlua` source files. The transpiled output MUST run identically to hand-written Lua code.

#### Scenario: Run transpiled hello world
- **WHEN** a `.tlua` file containing:
  ```tlua
  local greeting: string = "Hello from TypingLua!"
  print(greeting)
  ```
  is transpiled to `.lua` and executed with `lua.exe`
- **THEN** standard output SHALL print `Hello from TypingLua!`
- **AND** the process exit code SHALL be 0

#### Scenario: Run transpiled function with typed parameters
- **WHEN** a `.tlua` file containing:
  ```tlua
  local function add(a: number, b: number): number
    return a + b
  end
  print(add(3, 4))
  ```
  is transpiled to `.lua` and executed with `lua.exe`
- **THEN** standard output SHALL print `7`
- **AND** the process exit code SHALL be 0

#### Scenario: Transpiled output has no residual type syntax
- **WHEN** a transpiled `.lua` file is executed with `lua.exe`
- **THEN** no syntax errors related to type annotations (colons, type names) SHALL occur
