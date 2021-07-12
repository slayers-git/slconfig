# SLConfig

SLConfig is a simple config parser library written in C89, it can be used to parse standart UNIX configuration files with some additional functionality added (optionally).

## Features

- Optionally enviromental variables can be obtained and set to a key
- Optional variables inside of the configuration files

## Example configuration file syntax

```
# standart UNIX configuration syntax:
# comment
key1 = value
key2 = "value within quotes"

# optional functionality:
shell_of_choice = ${SHELL} # example usage of enviromental variables
# example usage of variables
$prefix = "[myprefix]"
error = $(prefix)" error: "
warn  = $(prefix)" warning: "
```

Example C implementation can be found in `example.c` file

## Building

To build and install SLConfig library on a \*nix system, in your shell type
```bash
mkdir build && cd build\
cmake && make && sudo make install
```

If you also want to compile example.c, add `-DSLC_EXAMPLES=1` to the cmake command.
