@echo off

echo #ifndef COMMIT_ID>version.h
for /F %%i in ('git rev-parse --short HEAD') do echo #define COMMIT_ID 0x%%i>>version.h
echo #ifndef COMMIT_ID>>version.h
echo #error "COMMIT_ID Not define">>version.h
echo #define COMMIT_ID 000000>>version.h
echo #endif>>version.h
echo #endif>>version.h

set OUTFILE=OD/OD_Flash.h
echo /* OD_PERSIST_COMM */>%OUTFILE%

set COPY_ON=0
for /f "delims=" %%i in (OD/OD.c) do call :CHECKLINE "%%i"
exit 0

:CHECKLINE
if %1 == "OD_ATTR_PERSIST_COMM OD_PERSIST_COMM_t OD_PERSIST_COMM = {" (
	set COPY_ON=1
rem	call :COPYLINE "OD_PERSIST_COMM_t OD_PERSIST_COMM_FACTORY = {"
	call :COPYLINE ".OD_PERSIST_COMM = {"
	exit /b
)
if %1 == "};" (
	if %COPY_ON% == 1 (
		set COPY_ON=0
		call :COPYLINE "},"
	)
	exit /b
)
if %COPY_ON% == 1 call :COPYLINE %1
exit /b

:COPYLINE
set line=%1
echo %line:"=%>>%OUTFILE%
exit /b
