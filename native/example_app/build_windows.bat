
@rem Check that cl.exe is available
where cl.exe /q
@ if errorlevel 1 (
    @echo Error: cl.exe not found, run this .bat from an MSVC tools command
    @echo   prompt or run the correct vcvarsall.bat beforehand.
    exit 1
)

mkdir build

cl.exe /nologo /Fobuild\ src\test.c src\ufbx.c src\example_app.c /link /OUT:.\example.exe

