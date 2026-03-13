## Requirements

### Requirement: Type erasure transpilation
The transpiler SHALL convert `.tlua` source files to standard `.lua` output files by removing all type annotations (type erasure). The output `.lua` file MUST be valid Lua 5.4 that can be executed by any standard Lua interpreter.

#### Scenario: Local variable type is stripped
- **WHEN** the input `.tlua` contains `local x: number = 42`
- **THEN** the output `.lua` SHALL contain `local x = 42`

#### Scenario: Multiple local variables types are stripped
- **WHEN** the input `.tlua` contains `local a: string, b: number = "hi", 1`
- **THEN** the output `.lua` SHALL contain `local a, b = "hi", 1`

#### Scenario: Global variable type is stripped
- **WHEN** the input `.tlua` contains `x: number = 42`
- **THEN** the output `.lua` SHALL contain `x = 42`

#### Scenario: Function parameter types are stripped
- **WHEN** the input `.tlua` contains `function add(a: number, b: number)`
- **THEN** the output `.lua` SHALL contain `function add(a, b)`

#### Scenario: Function return type is stripped
- **WHEN** the input `.tlua` contains `function greet(): string`
- **THEN** the output `.lua` SHALL contain `function greet()`

#### Scenario: Full function signature stripped
- **WHEN** the input `.tlua` contains `function add(a: number, b: number): number`
- **THEN** the output `.lua` SHALL contain `function add(a, b)`

#### Scenario: Anonymous function types stripped
- **WHEN** the input `.tlua` contains `local f = function(x: number): string`
- **THEN** the output `.lua` SHALL contain `local f = function(x)`

#### Scenario: Complex type expressions are stripped
- **WHEN** the input `.tlua` contains `local items: table<string, number[]>? = nil`
- **THEN** the output `.lua` SHALL contain `local items = nil`

### Requirement: Code preservation during transpilation
The transpiler SHALL preserve all non-type-annotation code exactly as written. This includes comments, whitespace structure, string literals, and all Lua logic.

#### Scenario: Comments are preserved
- **WHEN** the input `.tlua` contains `-- this is a comment` on a line
- **THEN** the output `.lua` SHALL contain the same comment unchanged

#### Scenario: String literals are not modified
- **WHEN** the input `.tlua` contains `local s: string = "local x: number = 42"`
- **THEN** the output `.lua` SHALL contain `local s = "local x: number = 42"` (the string content is preserved, only the declaration type is stripped)

#### Scenario: Multi-line strings are not modified
- **WHEN** the input `.tlua` contains a multi-line string `[[ local x: number ]]`
- **THEN** the string content SHALL NOT be modified (type-like text inside strings is not a type annotation)

#### Scenario: Indentation is preserved
- **WHEN** the input `.tlua` contains indented code with type annotations
- **THEN** the output `.lua` SHALL preserve the same indentation structure

#### Scenario: Blank lines are preserved
- **WHEN** the input `.tlua` contains blank lines between statements
- **THEN** the output `.lua` SHALL preserve those blank lines

### Requirement: Untyped code passes through unchanged
The transpiler SHALL pass through any `.tlua` source that contains no type annotations without any modification. Since `.tlua` is a superset of Lua 5.4, a pure Lua file transpiled SHALL produce identical output.

#### Scenario: Pure Lua file
- **WHEN** the input `.tlua` contains only standard Lua 5.4 code with no type annotations
- **THEN** the output `.lua` SHALL be identical to the input

#### Scenario: File with only comments and code
- **WHEN** the input `.tlua` contains Lua code with regular comments but no type annotations
- **THEN** the output `.lua` SHALL be identical to the input

### Requirement: Per-file transpilation
The transpiler SHALL process each `.tlua` file independently. Transpilation of one file SHALL NOT depend on the contents of any other file. This enables parallel and incremental transpilation.

#### Scenario: Single file transpilation
- **WHEN** the transpiler is invoked with a single `.tlua` file
- **THEN** it SHALL produce exactly one `.lua` output file corresponding to that input

#### Scenario: Independent file processing
- **WHEN** two `.tlua` files reference each other's types
- **THEN** each file SHALL be transpiled independently without requiring access to the other file

### Requirement: Output file naming
The transpiler SHALL produce output files with the `.lua` extension. The output file name SHALL be derived from the input file name by replacing the `.tlua` extension with `.lua`.

#### Scenario: Standard file naming
- **WHEN** the input file is `main.tlua`
- **THEN** the output file SHALL be named `main.lua`

#### Scenario: Nested path file naming
- **WHEN** the input file is `src/utils/helpers.tlua`
- **THEN** the output file SHALL be named `src/utils/helpers.lua` (preserving the directory structure)

### Requirement: Transpilation error handling
The transpiler SHALL report clear error messages when a `.tlua` file contains syntax errors in type annotations. Errors MUST include the file name, line number, and a description of the problem.

#### Scenario: Malformed type annotation
- **WHEN** the input `.tlua` contains `local x: = 42` (missing type expression after colon)
- **THEN** the transpiler SHALL report an error with the file name, line number, and a message indicating a missing type expression

#### Scenario: Unclosed generic type
- **WHEN** the input `.tlua` contains `local m: table<string, number` (missing closing `>`)
- **THEN** the transpiler SHALL report an error indicating an unclosed generic type

#### Scenario: Valid Lua syntax error passes through
- **WHEN** the input `.tlua` contains a Lua syntax error unrelated to type annotations (e.g., `local = 42`)
- **THEN** the transpiler MAY report the error or MAY produce output that will fail when executed by the Lua interpreter

### Requirement: Output is valid Lua 5.4
The transpiled output SHALL always be valid Lua 5.4 source code, provided the input `.tlua` file is syntactically valid. The output MUST be executable by the project's compiled `lua.exe` (Lua 5.4 interpreter).

#### Scenario: Transpiled output executes successfully
- **WHEN** a valid `.tlua` file is transpiled to `.lua`
- **AND** the output `.lua` file is executed with the compiled `lua.exe`
- **THEN** execution SHALL succeed without syntax errors

#### Scenario: Runtime behavior matches intent
- **WHEN** a `.tlua` file containing `local x: number = 42; print(x)` is transpiled and executed
- **THEN** the output SHALL print `42` (type annotations do not affect runtime behavior)

#### Scenario: Method calls work after transpilation
- **WHEN** a `.tlua` file containing `obj:method(arg)` (method call, not type annotation) is transpiled
- **THEN** the output SHALL preserve the method call syntax exactly