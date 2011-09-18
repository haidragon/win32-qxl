@echo off

Rem
Rem Build and copy to target based on environment variables set by WinDDK\bin\setenv.
Rem

Rem Just use %BUILD_ALT_DIR%, it is equivalent
Rem set TARGET=%DDK_TARGET_OS%_%_BUILDARCH%_%BUILD_TYPE%
set TARGET=install_%BUILD_ALT_DIR%
echo TARGET=%TARGET%
if not exist %TARGET% mkdir %TARGET%

cd miniport
build -cZg
cd ../display
build -cZg
cd ../
copy display\obj%BUILD_ALT_DIR%\i386\qxldd.dll %TARGET%
copy miniport\obj%BUILD_ALT_DIR%\i386\qxl.sys %TARGET%
copy miniport\qxl.inf %TARGET%
copy display\obj%BUILD_ALT_DIR%\i386\qxldd.pdb %TARGET%
copy miniport\obj%BUILD_ALT_DIR%\i386\qxl.pdb %TARGET%
