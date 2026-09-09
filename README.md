# PiCAN

Work in progress

## Requirements

`cmake` (and a generator like `ninja` preferred or `make`)
`gcc`
`mold` linker optionally for better link times
`clang-format`
`clang-tidy`
`semgrep`
`socketcan` ?
`bash` for scripts

## Dependencies

All managed using `cpm` except `libbacktrace`

## Unlocking Memory Limits

The binary calls `mlockall` with large numbers which will likely fail depending on your system configuration

### Temporary Solution

`sudo setcap cap_ipc_lock+ep <path-to-binary>`

This will need to be done every time the binary is recompiled

### Permanent Solution

`sudo vim /etc/security/limits.d/99-pican-memlock.conf`

Then add to the file:

`@wheel - memlock unlimited`

or swap `@wheel` with only your user, and unlimited with the desired number like `524288`.

logout and back in the limits should be as expected by running `ulimit -l` to see.

## Structure

## Checks (MISRA C++) Static Analysis Pipeline

clang-format
clang-tidy
semgrep