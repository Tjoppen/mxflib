# Microsoft Developer Studio Project File - Name="mxflib" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Static Library" 0x0104

CFG=mxflib - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "mxflib.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "mxflib.mak" CFG="mxflib - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "mxflib - Win32 Release" (based on "Win32 (x86) Static Library")
!MESSAGE "mxflib - Win32 Debug" (based on "Win32 (x86) Static Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "mxflib - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_MBCS" /D "_LIB" /YX /FD /c
# ADD CPP /nologo /W3 /GX /I "..\.." /D "WIN32" /D "NDEBUG" /D "_MBCS" /D "_LIB" /YX /FD /c
# ADD BASE RSC /l 0x809 /d "NDEBUG"
# ADD RSC /l 0x809 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo

!ELSEIF  "$(CFG)" == "mxflib - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_MBCS" /D "_LIB" /YX /FD /GZ /c
# ADD CPP /nologo /MTd /W3 /Gm /GX /ZI /Od /I "..\.." /D "WIN32" /D "_DEBUG" /D "_MBCS" /D "_LIB" /YX /FD /GZ /c
# ADD BASE RSC /l 0x809 /d "_DEBUG"
# ADD RSC /l 0x809 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo

!ENDIF 

# Begin Target

# Name "mxflib - Win32 Release"
# Name "mxflib - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=..\..\mxflib\deftypes.cpp
# End Source File
# Begin Source File

SOURCE=..\..\mxflib\esp_dvdif.cpp
# End Source File
# Begin Source File

SOURCE=..\..\mxflib\esp_mpeg2ves.cpp
# End Source File
# Begin Source File

SOURCE=..\..\mxflib\esp_wavepcm.cpp
# End Source File
# Begin Source File

SOURCE=..\..\mxflib\essence.cpp
# End Source File
# Begin Source File

SOURCE=..\..\mxflib\helper.cpp
# End Source File
# Begin Source File

SOURCE=..\..\mxflib\index.cpp
# End Source File
# Begin Source File

SOURCE=..\..\mxflib\klvobject.cpp
# End Source File
# Begin Source File

SOURCE=..\..\mxflib\mdobject.cpp
# End Source File
# Begin Source File

SOURCE=..\..\mxflib\mdtraits.cpp
# End Source File
# Begin Source File

SOURCE=..\..\mxflib\mdtype.cpp
# End Source File
# Begin Source File

SOURCE=..\..\mxflib\metadata.cpp
# End Source File
# Begin Source File

SOURCE=..\..\mxflib\mxffile.cpp
# End Source File
# Begin Source File

SOURCE=..\..\mxflib\partition.cpp
# End Source File
# Begin Source File

SOURCE=..\..\mxflib\primer.cpp
# End Source File
# Begin Source File

SOURCE=..\..\mxflib\rip.cpp
# End Source File
# Begin Source File

SOURCE=..\..\mxflib\sopsax.cpp
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=..\..\mxflib\datachunk.h
# End Source File
# Begin Source File

SOURCE=..\..\mxflib\debug.h
# End Source File
# Begin Source File

SOURCE=..\..\mxflib\deftypes.h
# End Source File
# Begin Source File

SOURCE=..\..\mxflib\endian.h
# End Source File
# Begin Source File

SOURCE=..\..\mxflib\esp_dvdif.h
# End Source File
# Begin Source File

SOURCE=..\..\mxflib\esp_mpeg2ves.h
# End Source File
# Begin Source File

SOURCE=..\..\mxflib\esp_wavepcm.h
# End Source File
# Begin Source File

SOURCE=..\..\mxflib\essence.h
# End Source File
# Begin Source File

SOURCE=..\..\mxflib\forward.h
# End Source File
# Begin Source File

SOURCE=..\..\mxflib\helper.h
# End Source File
# Begin Source File

SOURCE=..\..\mxflib\index.h
# End Source File
# Begin Source File

SOURCE=..\..\mxflib\klvobject.h
# End Source File
# Begin Source File

SOURCE=..\..\mxflib\mdobject.h
# End Source File
# Begin Source File

SOURCE=..\..\mxflib\mdtraits.h
# End Source File
# Begin Source File

SOURCE=..\..\mxflib\mdtype.h
# End Source File
# Begin Source File

SOURCE=..\..\mxflib\metadata.h
# End Source File
# Begin Source File

SOURCE=..\..\mxflib\mxffile.h
# End Source File
# Begin Source File

SOURCE=..\..\mxflib\mxflib.h
# End Source File
# Begin Source File

SOURCE=..\..\mxflib\partition.h
# End Source File
# Begin Source File

SOURCE=..\..\mxflib\primer.h
# End Source File
# Begin Source File

SOURCE=..\..\mxflib\rip.h
# End Source File
# Begin Source File

SOURCE=..\..\mxflib\smartptr.h
# End Source File
# Begin Source File

SOURCE=..\..\mxflib\sopsax.h
# End Source File
# Begin Source File

SOURCE=..\..\mxflib\system.h
# End Source File
# Begin Source File

SOURCE=..\..\mxflib\types.h
# End Source File
# Begin Source File

SOURCE=..\..\mxflib\waveheader.h
# End Source File
# End Group
# Begin Group "Documentation"

# PROP Default_Filter "*.cfg;*.dox"
# Begin Source File

SOURCE=..\..\dox.bat
# End Source File
# Begin Source File

SOURCE=..\..\doxyfile.cfg
# End Source File
# Begin Source File

SOURCE=..\..\docs\mxflib.dox
# End Source File
# End Group
# Begin Source File

SOURCE=..\..\license.txt
# End Source File
# Begin Source File

SOURCE=..\..\release.txt
# End Source File
# Begin Source File

SOURCE=..\..\types.xml
# End Source File
# Begin Source File

SOURCE=..\..\xmldict.xml
# End Source File
# End Target
# End Project
