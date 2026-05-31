# In order of decreasing importance / urgency

## Bug fixes

- check that all control flow paths return an appropriate value for non-void function

## Unfinished Sections

- revamp diagnostics
  - provide info about the error (e.g. provide context at the function regarding
  function call type mismatch)

- implement CFG, SSA and use memory read-modify-write operations
- generate function prologue and epilogue

- pass error handling to caller so we can provide specific token errors

## Future additions

- support multiple variable declarations in one statement
- assign the last statement in any block as the block's return value
- add more ast variants
- add support for funptrs

- const, inline, static, etc...

- structs
  - allow default values
  - mangle function names with their type so we can use var.func(...) as an alias
  for ... func(type var, ...)

- arrays
  - use `[num]type` or `[]type` for fixed-size arrays (number left empty for array
  of 1 item)
  - use `[..]type` for dynamic arrays
  - dynamic arrays will get their own Array module

- strings
  - currently `".."` strings are parsed into plain char arrays. Once string
  module is developed, parse them directly into dynamic array String type

- allocators
  - implement custom allocators that can be used for dynamic structures
  - custom malloc, pool / arena, temporary

- ranges
  - `start..end` will be exclusive
  - `start..=end` will be inclusive

- for loops
  - currently only C-style
  - possible to add `for x in xs` or even `for xs`, second one will automatically
  generate the variable name to use in the block (either e, elem, it for items
  in unnamed structures (i.e. arrays, sets, hashmaps) or using the structure's
  field names)

- modules
  - modules will be loaded with the `import` keyword which will read the module's
  symbol table in order to provide the symbols for linking
  - the `include` keyword can be used to directly include source content from
  another file
  - for modules that are platform / os specific, the module folder will contain
  all the supported platform's `.modc` files (e.g. Windows.modc, MacOS.modc,
  Linux.modc) and there will be a file that loads the symbols according to
  build flags

- build system will be included in the compiler

- preprocessor
  - using `#`
  - `#macro` placed before a function and then `#code` for code blocks that go
  into the macro functions (inspired by Jai's `#expand`)
  - `#if {} else {}`
  - `#comptime`

- comptime
  - unsure how to do this

- defer
  - defer code execution to the end of the function
  - should not be too difficult? Just store the ast and insert it at the end of
  the function

## Not-decided upon

- automatically detecting variable types / auto

- whether or not to make the language less verbose
  - move type declarations to after the variable (fits in with making them optional)
    - i.e. var: type = ...
  - moving function return type to the end of the declaration, easier to grep for
    - i.e. func(...) -> type {}
  - use custom tokens to differentiate between const and var instead of using
  the `const` specifier
