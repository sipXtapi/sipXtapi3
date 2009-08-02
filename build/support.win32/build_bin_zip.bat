@ECHO OFF

REM Copyright 2008 AOL LLC.
REM Licensed to SIPfoundry under a Contributor Agreement.
REM
REM This library is free software; you can redistribute it and/or
REM modify it under the terms of the GNU Lesser General Public
REM License as published by the Free Software Foundation; either
REM version 2.1 of the License, or (at your option) any later version.
REM
REM This library is distributed in the hope that it will be useful,
REM but WITHOUT ANY WARRANTY; without even the implied warranty of
REM MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
REM Lesser General Public License for more details.
REM
REM You should have received a copy of the GNU Lesser General Public
REM License along with this library; if not, write to the Free Software
REM Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301
REM USA. 
REM  
REM Copyright (C) 2004-2006 SIPfoundry Inc.
REM Licensed by SIPfoundry under the LGPL license.
REM
REM Copyright (C) 2004-2006 Pingtel Corp.  All rights reserved.
REM Licensed to SIPfoundry under a Contributor Agreement.

CALL setRepoEnvVar.bat

IF NOT "%SIPXTAPI_WINZIP_BASE%" == "" GOTO BUILD_IT
SET SIPXTAPI_WINZIP_BASE=C:\Program Files\WinZip

:BUILD_IT
SET TARGET_NAME="..\sipXtapi_WIN32_bin_%1%REPRO_VERSION%_%date:~10,4%-%date:~4,2%-%date:~7,2%.zip"

GOTO BUILD_ZIP

:BUILD_ZIP
  "%SIPXTAPI_WINZIP_BASE%\wzzip" -rp %TARGET_NAME% ..\Build.Win32