# GB Emulator

This is a Gameboy emulator I made as a side project to get into the world of emulation.

Although it doesn't implement some quirks or bugs found in real hardware, it is a complete emulator capable of running some of the most
complex ROMs. I decided not to make it more accurate to keep the scope small, as I intended this to be a quick side project.

## Usage

To open a ROM file, either drag it to the built exe file or pass the path as the first parameter in the command line.
You can use either the keyboard or an xbox controller as input.
Keyboard controls:
 - Arrow keys: D-Pad
 - Z: A button
 - X: B button
 - C: Start button
 - V: Select button

The xbox controller maps the obvious buttons to the GB ones (D-Pad/Left Joystick -> D-Pad, A button -> A button, etc.)

## Build

To build it, run premake in the root directory to generate files for your preferred build system.
I personally use Visual Studio, for which I run 'premake.exe vs2022'.
The generated build files will be inside the 'build/' folder, and the compiled binaries in 'build/bin'

## Shader effects

You might notice that the shader the emulator uses is a text HLSL file inside the 'shader/' folder. This is a deliberate choice.
This shader gets built at runtime by the emulator, so you can edit it to create your own post-processing or fullscreen effects without recompiling.