rem SET ENVIRONMENT TO BUILD PMF ON WINDOWS
rem THIS IS MEANT AS A TEMPLATE, ADAPT AS YOU SEE FIT.

rem --- Qt build environment
SET QMAKESPEC=win32-msvc
SET QTDIR=D:\Qt\6.4.3\msvc2019_64

rem --- paths
set path=%PATH%;%QTDIR%\;%QTDIR%\bin;
set path=%PATH%;%QTDIR%\qt\bin;
set include=%INCLUDE%;%QTDIR%\include\QtGui\;
set include=%INCLUDE%;%QTDIR%\include\QtCore\;
set include=%INCLUDE%;%QTDIR%\include\Qt\;
set include=%INCLUDE%;%QTDIR%\include\;
set include=%INCLUDE%;%QTDIR%\;

rem --- DB2
set DB2PATH=c:\Program Files\IBM\SQLLIB\
set LIBPATH=%LIBPATH%;%DB2PATH%\lib

rem --- POSTGRES
set POSTGRES_INC_64=c:\Program Files\PostgreSQL\16\include
set POSTGRES_LIB_64=c:\Program Files\PostgreSQL\16\lib

rem --- MARIADB
set MARIADB_INC_64=c:\Program Files\MariaDB 10.4\include\mysql\
set MARIADB_LIB_64=c:\Program Files\MariaDB 10.4\lib


rem --- remember PATH
set orgpath=%PATH%

rem --- build, reset path first
set PATH=%orgpath%
set path=%PATH%;C:\Qt\6.4.3\msvc2019_64\
set path=%PATH%;C:\Qt\6.4.3\msvc2019_64\bin\
set "MS_VC_Path=c:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build"
call "%MS_VC_Path%\test.bat"

call C:"\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat"

nmake distclean
qmake pmf6.pro target=all db=sample

nmake
IF %ERRORLEVEL% NEQ 0 (Echo An error was found &Exit /b 1)

