## ADDED Requirements

### Requirement: Annotation prefix format
All type annotations SHALL use the triple-dash prefix `---@` followed by a directive name. The prefix MUST consist of exactly three hyphens followed immediately by the `@` symbol, with no spaces between `---` and `@`.

#### Scenario: Valid annotation prefix
- **WHEN** a Lua source line contains `---@type number`
- **THEN** it SHALL be recognized as a valid type annotation

#### Scenario: Regular comment is not an annotation
- **WHEN** a Lua source line contains `-- @type number` (two hyphens)
- **THEN** it SHALL NOT be recognized as a type annotation

#### Scenario: Space between dashes and at-sign
- **WHEN** a Lua source line contains `--- @type number` (space before `@`)
- **THEN** it SHALL NOT be recognized as a type annotation

### Requirement: Type annotation directive
The system SHALL support the `---@type <type-expr>` directive to annotate the type of the variable declared on the immediately following line.

#### Scenario: Annotate a local variable
- **WHEN** the source contains:
  ```lua
  ---@type number
  local x = 42
  ```
- **THEN** the variable `x` SHALL be associated with type `number`

#### Scenario: Annotate a global variable
- **WHEN** the source contains:
  ```lua
  ---@type string
  name = "hello"
  ```
- **THEN** the variable `name` SHALL be associated with type `string`

### Requirement: Param annotation directive
The system SHALL support the `---@param <name> <type-expr>` directive to annotate the type of a function parameter. The `<name>` MUST match a parameter name in the function signature on or after the annotation.

#### Scenario: Single parameter annotation
- **WHEN** the source contains:
  ```lua
  ---@param x number
  local function square(x)
    return x * x
  end
  ```
- **THEN** parameter `x` of function `square` SHALL be associated with type `number`

#### Scenario: Multiple parameter annotations
- **WHEN** the source contains:
  ```lua
  ---@param a number
  ---@param b number
  local function add(a, b)
    return a + b
  end
  ```
- **THEN** parameter `a` SHALL be associated with type `number` AND parameter `b` SHALL be associated with type `number`

#### Scenario: Param name mismatch
- **WHEN** a `---@param` annotation references a name that does not exist in the following function's parameter list
- **THEN** a diagnostic warning SHALL be reported

### Requirement: Return annotation directive
The system SHALL support the `---@return <type-expr>` directive to annotate the return type of the function declared on or after the annotation.

#### Scenario: Single return type
- **WHEN** the source contains:
  ```lua
  ---@return string
  local function greet()
    return "hello"
  end
  ```
- **THEN** function `greet` SHALL have return type `string`

#### Scenario: Multiple return values
- **WHEN** the source contains:
  ```lua
  ---@return number, string
  local function get_info()
    return 42, "answer"
  end
  ```
- **THEN** function `get_info` SHALL have return types `number` and `string` (comma-separated)

#### Scenario: Void return
- **WHEN** the source contains:
  ```lua
  ---@return void
  local function do_nothing()
  end
  ```
- **THEN** function `do_nothing` SHALL have return type `void`, indicating no meaningful return value

### Requirement: Base type keywords
The type system SHALL recognize the following base type keywords: `nil`, `boolean`, `number`, `integer`, `string`, `table`, `function`, `thread`, `userdata`, `any`, `void`.

#### Scenario: All base types are valid
- **WHEN** a type expression uses any of `nil`, `boolean`, `number`, `integer`, `string`, `table`, `function`, `thread`, `userdata`, `any`, `void`
- **THEN** the type expression SHALL be accepted as valid

#### Scenario: Unknown base type
- **WHEN** a type expression uses a name that is not a recognized base type and is not a user-defined type
- **THEN** a diagnostic error SHALL be reported indicating an unknown type

### Requirement: Union type syntax
The type system SHALL support union types using the pipe operator `|`. A union type `T1|T2` indicates that a value MAY be of type `T1` or type `T2`.

#### Scenario: Two-type union
- **WHEN** the source contains `---@type number|string`
- **THEN** the type expression SHALL be parsed as a union of `number` and `string`

#### Scenario: Multi-type union
- **WHEN** the source contains `---@type number|string|boolean`
- **THEN** the type expression SHALL be parsed as a union of `number`, `string`, and `boolean`

#### Scenario: No spaces around pipe
- **WHEN** the source contains `---@type number | string` (spaces around `|`)
- **THEN** the type expression SHALL still be parsed as a valid union of `number` and `string`

### Requirement: Optional type syntax
The type system SHALL support optional types using the `?` suffix. The type `T?` SHALL be equivalent to `T|nil`.

#### Scenario: Optional string
- **WHEN** the source contains `---@type string?`
- **THEN** the type expression SHALL be parsed as equivalent to `string|nil`

#### Scenario: Optional in param
- **WHEN** the source contains `---@param name string?`
- **THEN** parameter `name` SHALL accept values of type `string` or `nil`

### Requirement: Array type syntax
The type system SHALL support array types using the `[]` suffix. The type `T[]` SHALL be equivalent to `table<integer, T>`.

#### Scenario: Number array
- **WHEN** the source contains `---@type number[]`
- **THEN** the type expression SHALL be parsed as an array of `number` (equivalent to `table<integer, number>`)

#### Scenario: Nested array
- **WHEN** the source contains `---@type string[][]`
- **THEN** the type expression SHALL be parsed as an array of arrays of `string`

### Requirement: Table generic type syntax
The type system SHALL support generic table types using the syntax `table<K, V>` where `K` is the key type and `V` is the value type.

#### Scenario: String-to-number map
- **WHEN** the source contains `---@type table<string, number>`
- **THEN** the type expression SHALL be parsed as a table with `string` keys and `number` values

#### Scenario: Nested table type
- **WHEN** the source contains `---@type table<string, table<string, number>>`
- **THEN** the type expression SHALL be parsed as a table with `string` keys and values that are themselves `table<string, number>`

### Requirement: Function signature type syntax
The type system SHALL support function signature types using the syntax `fun(<params>): <return-type>` where `<params>` is a comma-separated list of `name: type` pairs.

#### Scenario: Simple callback type
- **WHEN** the source contains `---@type fun(x: number): string`
- **THEN** the type expression SHALL be parsed as a function that takes a `number` parameter and returns `string`

#### Scenario: No-param function type
- **WHEN** the source contains `---@type fun(): void`
- **THEN** the type expression SHALL be parsed as a function with no parameters and no meaningful return value

#### Scenario: Multi-param function type
- **WHEN** the source contains `---@type fun(a: number, b: string): boolean`
- **THEN** the type expression SHALL be parsed as a function with parameters `a: number` and `b: string` that returns `boolean`

### Requirement: EBNF grammar specification
The type annotation syntax SHALL be formally defined in an EBNF grammar document. The grammar MUST cover all annotation directives (`@type`, `@param`, `@return`) and all type expressions (base types, unions, optionals, arrays, table generics, function signatures).

#### Scenario: Grammar covers all directives
- **WHEN** the EBNF grammar is reviewed
- **THEN** it SHALL contain production rules for `type_annotation`, `param_annotation`, and `return_annotation`

#### Scenario: Grammar covers all type constructors
- **WHEN** the EBNF grammar is reviewed
- **THEN** it SHALL contain production rules for `base_type`, `union_type`, `optional_type`, `array_type`, `table_type`, and `function_type`

#### Scenario: Grammar is unambiguous
- **WHEN** any valid type annotation string is parsed according to the EBNF grammar
- **THEN** there SHALL be exactly one valid parse tree (the grammar MUST be unambiguous)

### Requirement: Backward compatibility with standard Lua
All type annotations SHALL be embedded within Lua comments. Any Lua source file containing type annotations MUST remain valid and executable by a standard Lua 5.4 interpreter without modification.

#### Scenario: Annotated file runs in standard Lua
- **WHEN** a Lua file containing `---@type`, `---@param`, and `---@return` annotations is executed with the standard Lua 5.4 interpreter
- **THEN** the interpreter SHALL execute the file without errors, ignoring annotation comments

#### Scenario: Annotations do not affect runtime behavior
- **WHEN** a Lua file with type annotations is executed
- **THEN** the runtime behavior SHALL be identical to executing the same file with all annotation comments removed
