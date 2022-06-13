# Building PresentMon

Requirements:

- Visual Studio 2022
- awk and sed in your path (e.g., `%ProgramFiles%\Git\usr\bin`)

Quick-start:

```batch
msbuild /p:Platform=x64,Configuration=Release PresentMon.sln
build\Release\PresentMon-dev-x64.exe
```
