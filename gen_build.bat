@echo off
set workDir=%~dp0
rem cd /d %~dp0


rem set path=%workDir%third_party\python_26;%path%
rem 产生cp1工程。
rem python build\gyp_chromium -D"component=shared_library" -G msvs_version=2013 base\base.gyp
rem python build\gyp_chromium -D"component=shared_library" -G msvs_version=2013 tools\tools.gyp

python ..\..\build\gyp_chromium -D"component=shared_library" -G msvs_version=2013 skynet.gyp
rem python ..\..\build\gyp_chromium -D"component=shared_library" -G msvs_version=2013 strife-src\strife.gyp

if not "%1" == "no-pause" (
pause
)
