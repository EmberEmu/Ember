@ECHO Off
SETLOCAL ENABLEDELAYEDEXPANSION

SET nodb="false"

IF NOT "%1" == "world" (
	IF NOT "%1" == "login" (
		SET nodb="true" 
		CALL :NoDBError 
))

SET UTC=YYYYMMDDHHmmss
CALL :GetFormattedCurrentUTCDate UTC

FOR /f %%i in ('git rev-parse HEAD') do set COMMIT=%%i
FOR /f %%i in ('git describe --tags') do set TAG=%%i
SET output=%UTC%_%TAG%_%COMMIT%.sql

IF !nodb! == "false" (
	SET file=%1/migrations/%output%
) ELSE (
	SET file=%output%
)

ECHO -- Paste your migration query below>>!file!
GOTO EndOfScript

:GetFormattedCurrentUTCDate outString
FOR /F "tokens=* DELIMS=^=" %%a IN ('WMIC Path Win32_UTCTime Get Year^,Month^,Day^,Hour^,Minute^,Second /Value') DO (
 SET LINE=%%a
 FOR /f "tokens=1-2 delims=^=" %%i IN ("!LINE!") DO (
  IF "%%i" == "Year" ( SET year=%%j)
  IF "%%i" == "Month" ( SET month=%%j)
  IF "%%i" == "Day" ( SET day=%%j)
  IF "%%i" == "Hour" ( SET hour=%%j)
  IF "%%i" == "Minute" ( SET minute=%%j)
  IF "%%i" == "Second" ( SET second=%%j)
 )
)
 
IF %month% LSS 10 SET month=0%month%
IF %day% LSS 10 SET day=0%day%
IF %hour% LSS 10 SET hour=0%hour%
IF %minute% LSS 10 SET minute=0%minute%
IF %second% LSS 10 SET second=0%second%
SET %1=%year%%month%%day%%hour%%minute%%second%
GOTO EndOfScript

:NoDBError
echo No valid database argument specified (world or login), file will be created in current directory instead. Please move it to the desired directory.
pause
GOTO EndOfScript

:EndOfScript