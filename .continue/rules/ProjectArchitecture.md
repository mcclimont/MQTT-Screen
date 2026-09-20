# Project Architecture

This is a C++ application for esp32 devices with:

- Project details in `\README.md`
- Platformio configuration in `\platformio.ini`
- Main project script is `\src\main.cpp`
- Source in `\src`
- Include in `\include`
  - Secure project settings are in `\include\config.h` which must not be committed to the source repository. This file is created from from `\include\config.example.h`.
- Lib in `\lib`
- Tests in `\test`
- Build resources auto-managed by platformio.ini are in `\.pio\libdeps\esp32dev`. These sources also provide documentation and examples.

## Coding Standards

- Use c++ for all new files
- Follow the existing naming conventions