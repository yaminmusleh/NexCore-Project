# NexCore Studio

A native desktop editor for the NexCore language, written in C++20 with Qt 6
Widgets. It builds your `.nex` files with the real `nexc` compiler and runs the
programs it produces. Studio contains no compiler code: `nexc` and its
command-line use are unchanged and keep working on their own.

**Platform:** Linux (developed and tested on Ubuntu). Windows is planned, not done.

## What it does

- Editor with line numbers, `.nex` syntax highlighting, current-line highlight,
  automatic indentation, find and replace
- Several files in tabs, a sidebar with the open files and a folder explorer,
  recent files, and a window layout that is remembered between launches
- **Build** (F7), **Run** (F5) and **Stop** (Shift+F5) using the real compiler
- Compiler errors shown in the output panel and Problems list, underlined in the
  editor at the reported line and column, with the message on hover

Help > Keyboard Shortcuts shows the shortcuts that are currently active.

## Requirements

On Ubuntu or Debian:

```bash
sudo apt install qt6-base-dev qt6-wayland build-essential cmake nasm binutils
```

- Qt 6 Widgets, a C++20 compiler and CMake 3.21 or newer build Studio.
- `nasm` and `ld` (from `binutils`) are needed when you build a program, because
  `nexc` runs them. Studio checks for them and says so if they are missing.
- The compiler itself must be built first (see the repository's main README). Its
  own `CMakeLists.txt` asks for a newer CMake than Studio does.

## Build and run

From the repository root:

```bash
cmake -S studio -B build-studio -DCMAKE_BUILD_TYPE=Release
cmake --build build-studio -j
./build-studio/nexcore-studio            # or: ./build-studio/nexcore-studio file.nex
```

If the compiler runs out of memory, use `-j2` instead of `-j`.

Studio is a separate CMake project. The compiler's `CMakeLists.txt` does not
mention it, so the compiler builds exactly as before.

## Install (optional)

```bash
cmake --install build-studio --prefix ~/.local
```

This installs the program, a launcher entry ("NexCore Studio" in your
applications menu) and the icon, all under `~/.local`. No root is needed. The
launcher holds the full path of the program, so `~/.local/bin` does not have to
be in your `PATH` for the menu entry to work (it does for typing
`nexcore-studio` in a terminal).

To remove it again:

```bash
xargs rm -f < build-studio/install_manifest.txt
```

If the launcher or icon does not show up, log out and in again, or run
`update-desktop-database ~/.local/share/applications`.

## Finding the compiler

Studio looks for `nexc` in this order and uses the first one that is an
executable file:

1. the path you chose with **Build > Set Compiler Path...** (remembered)
2. the `NEXC_PATH` environment variable
3. relative to the Studio program: `./nexc`, `../build/nexc`, `../nexc`,
   `../../build/nexc`
4. your `PATH`

If none is found, building asks you to locate it. With the usual layout
(`build/nexc` and `build-studio/nexcore-studio` in the same repository) it is
found automatically.

## How a build works

- The current file is saved first, then `nexc` is started with that file's full
  path as its only argument.
- `nexc` writes `out.asm`, `out.o` and `out` into the folder it runs in, so Studio
  runs it in a fresh private folder under `~/.cache/NexCore/NexCore Studio/`.
  Nothing is written next to your source files, and one build never reuses
  another build's output.
- A build counts as successful only if `nexc` exits normally with code 0, reports
  no `-- line N col M:` errors, and produced an executable.
- **Run** builds first and then runs the program with your file's folder as its
  working directory. The language has no input, so the program gets none.
- Only one build or run is active at a time. **Stop** ends it.

## Files Studio keeps

- Settings (window layout, last folder, recent files, compiler path):
  `~/.config/NexCore/NexCore Studio.conf`
- Build folders: `~/.cache/NexCore/NexCore Studio/build-*` (removed on the next
  build; leftovers older than a day are removed at start-up)

To reset everything, delete those two locations.

## Files it opens

Only valid UTF-8 text up to 10 MB opens. Anything else is refused with a message,
because saving a file Studio could not read exactly would change its bytes. Files
are saved with `\n` line endings.

## Troubleshooting

**"Compiler not found"** - build `nexc` first (`cmake -S . -B build && cmake --build build`
in the repository root), then use Build > Set Compiler Path... or set `NEXC_PATH`.

**"nasm was not found in PATH"** - `sudo apt install nasm binutils`.

**The window does not start, with a message about the Qt platform plugin** -
install the platform packages: `sudo apt install qt6-wayland libxcb-cursor0`.
You can also force one with `QT_QPA_PLATFORM=xcb` or `QT_QPA_PLATFORM=wayland`.

**The launcher has no icon, or the window is not grouped with it** - install it
(see above) so the desktop can find the launcher and icon. A copy started
straight from `build-studio/` still shows a built-in window icon.

**A build fails with a message but no line number** - the compiler failed in a
later stage (for example the code generator) and does not report positions. The
message appears in Build Output and as an entry without a location in Problems.

**The underline is not where I expected** - it marks the first token the parser
could not accept, which can come after the actual mistake (a missing value before
`;` is reported at the `;`). The message is the compiler's own wording.

**Start over** - `rm -rf build-studio` and rebuild.

## Source layout

```
studio/
  CMakeLists.txt
  packaging/        launcher entry template and icon
  src/
    main.cpp            application setup and dark theme
    MainWindow.*        window, menus, tabs, file handling, build/run wiring
    CodeEditor.*        editor widget, gutter, indentation, error marks
    NexHighlighter.*    .nex highlighting
    FindBar.*           find and replace bar
    Sidebar.*           open files list and folder explorer
    OutputPanel.*       build output, problems, program output
    BuildController.*   runs nexc and the program (no widgets)
```

`NexHighlighter` follows the lexical rules in `docs/Nex.atg`. If you add keywords
or tokens to the grammar, update `src/NexHighlighter.cpp` to match.

## Known limits

- Linux only for now; one window; the open tabs are not restored at start-up.
- The toolbar uses text buttons, not icons.
- Error messages are shown exactly as the compiler prints them.