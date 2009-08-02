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

pushd support.win32

call updateVersion.bat

call stage_init.bat
call build_msvc71.bat sipXtapi-msvc71
call stage_sipxtapi.bat
call build_msvc71.bat sipx-mediaprocessing-msvc71
call stage_sipxmedia.bat
call build_bin_zip.bat 3_0_0_
call build_src_zip.bat http://scm.sipfoundry.org/rep/sipX/branches/sipXtapi-AOL/ 3_0_0_

IF NOT EXIST ..\..\..\contrib\gips\VoiceEngine\libraries\GIPSVideoEngineWindows_MT.lib GOTO DONE
call stage_gips_init.bat
call build_msvc71.bat gips-mediaprocessing-msvc71
call stage_gips.bat
call build_gips_bin_zip.bat 3_0_0_

:DONE
call revertVersion.bat

popd
