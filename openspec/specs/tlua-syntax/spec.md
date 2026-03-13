## Requirements

### Requirement: .tlua file format
The TypingLua project SHALL define `.tlua` as the source file extension for Typed Lua files. A `.tlua` file SHALL be a strict superset of standard Lua 5.4 — any valid Lua 5.4 source file MUST also be a valid `.tlua` file.

#### Scenario: Valid Lua file is valid .tlua
- **WHEN** a standard Lua 5.4 source file is renamed from `.lua` to `.tlua`
- **THEN** it SHALL be accepted as a valid `.tlua` file without any modifications

#### Scenario: .tlua file with type annotations
- **WHEN** a `.tlua` file contains inline type annotations (e.g., `local x: number = 42`)
- **THEN** it SHALL be accepted as a valid `.tlua` file

#### Scenario: .tlua file is not valid standard Lua
- **WHEN** a `.tlua` file containing inline type annotations is executed directly by a standard Lua 5.4 interpreter
- **THEN** a syntax error SHALL occur because the interpreter does not understand type annotations

### Requirement: Local variable type annotation
The `.tlua` syntax SHALL support type annotations on local variable declarations using the colon style `local <name>: <type-expr>`. The type annotation is placed between the variable name and the assignment operator (or end of statement).

#### Scenario: Local with type and initializer
- **WHEN** the source contains `local x: number = 42`
- **THEN** the variable `x` SHALL be associated with type `number` and initialized to `42`

#### Scenario: Local with type and no initializer
- **WHEN** the source contains `local name: string`
- **THEN** the variable `name` SHALL be associated with type `string` and have no explicit initializer (defaults to `nil` at runtime)

#### Scenario: Multiple local variables with types
- **WHEN** the source contains `local a: string, b: number = "hi", 1`
- **THEN** variable `a` SHALL be associated with type `string`, variable `b` SHALL be associated with type `number`, and they SHALL be initialized to `"hi"` and `1` respectively

#### Scenario: Multiple locals with partial type annotations
- **WHEN** the source contains `local a: string, b = "hi", 1`
- **THEN** variable `a` SHALL be associated with type `string`, variable `b` SHALL have no explicit type (implicit `any`), and both SHALL be initialized correctly

#### Scenario: Local without type annotation
- **WHEN** the source contains `local x = 42` (no type annotation)
- **THEN** variable `x` SHALL have no explicit type annotation (implicit `any`), preserving backward compatibility with standard Lua syntax

### Requirement: Global variable type annotation
The `.tlua` syntax SHALL support type annotations on global variable assignments using the colon style `<name>: <type-expr> = <expr>`. This applies to the first assignment of a global variable.

#### Scenario: Global with type annotation
- **WHEN** the source contains `x: number = 42`
- **THEN** the global variable `x` SHALL be associated with type `number` and assigned the value `42`

#### Scenario: Global without type annotation
- **WHEN** the source contains `x = 42` (no type annotation)
- **THEN** the global variable `x` SHALL have no explicit type annotation, preserving standard Lua semantics

### Requirement: Function parameter type annotation
The `.tlua` syntax SHALL support type annotations on function parameters using the colon style `<param-name>: <type-expr>` within the parameter list.

#### Scenario: Single typed parameter
- **WHEN** the source contains `function square(x: number)`
- **THEN** parameter `x` of function `square` SHALL be associated with type `number`

#### Scenario: Multiple typed parameters
- **WHEN** the source contains `function add(a: number, b: number)`
- **THEN** parameter `a` SHALL be associated with type `number` AND parameter `b` SHALL be associated with type `number`

#### Scenario: Mixed typed and untyped parameters
- **WHEN** the source contains `function f(a: number, b, c: string)`
- **THEN** parameter `a` SHALL have type `number`, parameter `b` SHALL have no explicit type (implicit `any`), and parameter `c` SHALL have type `string`

#### Scenario: Anonymous function with typed parameters
- **WHEN** the source contains `local g = function(x: number, y: string)`
- **THEN** the anonymous function's parameters SHALL be typed accordingly

#### Scenario: Local function with typed parameters
- **WHEN** the source contains `local function f(a: number)`
- **THEN** parameter `a` of local function `f` SHALL be associated with type `number`

### Requirement: Function return type annotation
The `.tlua` syntax SHALL support return type annotations on function declarations using a colon after the closing parenthesis: `function <name>(<params>): <return-type-list>`.

#### Scenario: Single return type
- **WHEN** the source contains `function greet(): string`
- **THEN** function `greet` SHALL have return type `string`

#### Scenario: Multiple return types
- **WHEN** the source contains `function get_info(): number, string`
- **THEN** function `get_info` SHALL have return types `number` and `string`

#### Scenario: Multiple return types with parentheses
- **WHEN** the source contains `function get_info(): (number, string)`
- **THEN** function `get_info` SHALL have return types `number` and `string`, equivalent to the non-parenthesized form

#### Scenario: Parameters and return type together
- **WHEN** the source contains `function add(a: number, b: number): number`
- **THEN** parameters `a` and `b` SHALL have type `number`, and the function return type SHALL be `number`

#### Scenario: Anonymous function with return type
- **WHEN** the source contains `local f = function(x: number): string`
- **THEN** the anonymous function SHALL have parameter `x` of type `number` and return type `string`

#### Scenario: No return type annotation
- **WHEN** the source contains `function f(a: number)` (no return type)
- **THEN** the function SHALL have no explicit return type (implicit `any`)

#### Scenario: Void return type
- **WHEN** the source contains `function do_nothing(): void`
- **THEN** function `do_nothing` SHALL have return type `void`, indicating no meaningful return value

### Requirement: Base type keywords
The type system SHALL recognize the following 11 base type keywords: `nil`, `boolean`, `number`, `integer`, `string`, `table`, `function`, `thread`, `userdata`, `any`, `void`.

#### Scenario: All base types are valid
- **WHEN** a type annotation uses any of `nil`, `boolean`, `number`, `integer`, `string`, `table`, `function`, `thread`, `userdata`, `any`, `void`
- **THEN** the type expression SHALL be accepted as valid

#### Scenario: Unknown type name
- **WHEN** a type expression uses a name that is not a recognized base type and is not a user-defined type
- **THEN** a diagnostic error SHALL be reported indicating an unknown type

#### Scenario: Type keywords are case-sensitive
- **WHEN** a type expression uses `Number` or `STRING` (incorrect casing)
- **THEN** it SHALL NOT be recognized as a base type keyword

### Requirement: Union type syntax
The type system SHALL support union types using the pipe operator `|`. A union type `T1|T2` indicates that a value MAY be of type `T1` or type `T2`. The pipe operator MAY be surrounded by optional whitespace.

#### Scenario: Two-type union
- **WHEN** the source contains `local x: number|string = 42`
- **THEN** the type of `x` SHALL be parsed as a union of `number` and `string`

#### Scenario: Multi-type union
- **WHEN** the source contains `local x: number|string|boolean`
- **THEN** the type SHALL be parsed as a union of `number`, `string`, and `boolean`

#### Scenario: Spaces around pipe
- **WHEN** the source contains `local x: number | string`
- **THEN** the type SHALL be parsed as a valid union of `number` and `string`

### Requirement: Optional type syntax
The type system SHALL support optional types using the `?` suffix. The type `T?` SHALL be semantically equivalent to `T|nil`.

#### Scenario: Optional string variable
- **WHEN** the source contains `local name: string? = nil`
- **THEN** the type of `name` SHALL be parsed as equivalent to `string|nil`

#### Scenario: Optional function parameter
- **WHEN** the source contains `function f(x: number?)`
- **THEN** parameter `x` SHALL accept values of type `number` or `nil`

### Requirement: Array type syntax
The type system SHALL support array types using the `[]` suffix. The type `T[]` SHALL be semantically equivalent to `table<integer, T>`.

#### Scenario: Number array
- **WHEN** the source contains `local nums: number[] = {1, 2, 3}`
- **THEN** the type of `nums` SHALL be parsed as an array of `number`

#### Scenario: Nested array
- **WHEN** the source contains `local matrix: number[][] = {{1,2},{3,4}}`
- **THEN** the type SHALL be parsed as an array of arrays of `number`

#### Scenario: Optional array
- **WHEN** the source contains `local items: string[]?`
- **THEN** the type SHALL be parsed as `string[]` or `nil` (optional array of strings)

### Requirement: Table generic type syntax
The type system SHALL support generic table types using the syntax `table<K, V>` where `K` is the key type and `V` is the value type.

#### Scenario: String-to-number map
- **WHEN** the source contains `local scores: table<string, number> = {}`
- **THEN** the type SHALL be parsed as a table with `string` keys and `number` values

#### Scenario: Nested table type
- **WHEN** the source contains `local config: table<string, table<string, number>>`
- **THEN** the type SHALL be parsed as a table with `string` keys and values that are themselves `table<string, number>`

### Requirement: Function signature type syntax
The type system SHALL support function signature types using the syntax `fun(<params>): <return-type>` where `<params>` is a comma-separated list of `name: type` pairs.

#### Scenario: Simple callback type
- **WHEN** the source contains `local cb: fun(x: number): string`
- **THEN** the type SHALL be parsed as a function taking `number` and returning `string`

#### Scenario: No-param function type
- **WHEN** the source contains `local action: fun(): void`
- **THEN** the type SHALL be parsed as a function with no parameters and no meaningful return

#### Scenario: Multi-param function type
- **WHEN** the source contains `local handler: fun(a: number, b: string): boolean`
- **THEN** the type SHALL be parsed as a function with parameters `a: number` and `b: string` returning `boolean`

### Requirement: Type annotation is optional
Type annotations SHALL be entirely optional in all supported positions. A `.tlua` file MAY contain a mix of annotated and unannotated declarations. Unannotated declarations SHALL be treated as having implicit type `any`.

#### Scenario: File with no annotations
- **WHEN** a `.tlua` file contains zero type annotations (pure Lua 5.4 code)
- **THEN** the file SHALL be valid and all declarations SHALL be treated as type `any`

#### Scenario: Partially annotated file
- **WHEN** a `.tlua` file annotates some variables and functions but not others
- **THEN** annotated declarations SHALL carry their specified types, and unannotated declarations SHALL be treated as type `any`

### Requirement: Colon does not conflict with method call syntax
The colon `:` used for type annotations SHALL NOT conflict with Lua's method call syntax (`obj:method()`). The parser SHALL distinguish between these two uses based on syntactic context.

#### Scenario: Method call in expression
- **WHEN** the source contains `obj:method(arg)`
- **THEN** the colon SHALL be parsed as a method call, not a type annotation

#### Scenario: Type annotation in declaration
- **WHEN** the source contains `local x: number = obj:method()`
- **THEN** the first colon (after `x`) SHALL be parsed as a type annotation, and the second colon (after `obj`) SHALL be parsed as a method call

#### Scenario: Method definition with typed parameters
- **WHEN** the source contains `function MyClass:init(name: string): void`
- **THEN** the first colon (after `MyClass`) SHALL be parsed as method syntax, `name: string` SHALL be a typed parameter, and `: void` after `)` SHALL be the return type

### Requirement: Multi-return type list comma disambiguation
In the context of a function return type annotation, commas SHALL separate return types in the return type list. In the context of multi-variable declarations (`local a, b`), commas SHALL separate variables. The parser MUST correctly disambiguate these contexts.

#### Scenario: Multi-return commas are type separators
- **WHEN** the source contains `function f(): number, string`
- **THEN** the commas after `:` SHALL be parsed as separating return types, yielding return types `number` and `string`

#### Scenario: Multi-variable commas are variable separators
- **WHEN** the source contains `local a: number, b: string = 1, "hi"`
- **THEN** the comma after `number` SHALL be parsed as a variable separator, not a union or multi-type separator

### Requirement: EBNF grammar specification
The `.tlua` type annotation syntax SHALL be formally defined in an EBNF grammar. The grammar MUST cover all type annotation injection points (local variable, global variable, function parameter, function return) and all type expression forms (base types, unions, optionals, arrays, table generics, function signatures).

#### Scenario: Grammar covers all injection points
- **WHEN** the EBNF grammar is reviewed
- **THEN** it SHALL contain production rules for `typed_local_stat`, `typed_global_assign`, `typed_param`, and `typed_return_type`

#### Scenario: Grammar covers all type constructors
- **WHEN** the EBNF grammar is reviewed
- **THEN** it SHALL contain production rules for `base_type`, `union_type`, `optional_type`, `array_type`, `table_type`, and `function_type`

#### Scenario: Grammar is unambiguous
- **WHEN** any valid `.tlua` source string is parsed according to the EBNF grammar
- **THEN** there SHALL be exactly one valid parse tree (the grammar MUST be unambiguous)

#### Scenario: Grammar extends Lua 5.4 grammar
- **WHEN** the EBNF grammar is compared with the standard Lua 5.4 grammar
- **THEN** the `.tlua` grammar SHALL be a strict superset — all Lua 5.4 productions remain valid, and new productions only add type annotation capabilities