# Microsoft Developer Studio Project File - Name="CONPROV_TIME_LINK" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Console Application" 0x0103

CFG=CONPROV_TIME_LINK - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "CONPROV_TIME_LINK.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "CONPROV_TIME_LINK.mak" CFG="CONPROV_TIME_LINK - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "CONPROV_TIME_LINK - Win32 Release" (based on "Win32 (x86) Console Application")
!MESSAGE "CONPROV_TIME_LINK - Win32 Debug" (based on "Win32 (x86) Console Application")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "CONPROV_TIME_LINK - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "CONPROV_TIME_LINK___Win32_Release"
# PROP BASE Intermediate_Dir "CONPROV_TIME_LINK___Win32_Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "CONPROV_TIME_LINK___Win32_Release"
# PROP Intermediate_Dir "CONPROV_TIME_LINK___Win32_Release"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_CONSOLE" /D "_MBCS" /YX /FD /c
# ADD CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_CONSOLE" /D "_MBCS" /YX /FD /c
# ADD BASE RSC /l 0x410 /d "NDEBUG"
# ADD RSC /l 0x410 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib  kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /machine:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib  kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /machine:I386

!ELSEIF  "$(CFG)" == "CONPROV_TIME_LINK - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "CONPROV_TIME_LINK___Win32_Debug"
# PROP BASE Intermediate_Dir "CONPROV_TIME_LINK___Win32_Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "CONPROV_TIME_LINK___Win32_Debug"
# PROP Intermediate_Dir "CONPROV_TIME_LINK___Win32_Debug"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_CONSOLE" /D "_MBCS" /YX /FD /GZ  /c
# ADD CPP /nologo /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_CONSOLE" /D "_MBCS" /YX /FD /GZ  /c
# ADD BASE RSC /l 0x410 /d "_DEBUG"
# ADD RSC /l 0x410 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib  kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /debug /machine:I386 /pdbtype:sept
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib  kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /debug /machine:I386 /pdbtype:sept

!ENDIF 

# Begin Target

# Name "CONPROV_TIME_LINK - Win32 Release"
# Name "CONPROV_TIME_LINK - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\AbsPath.cpp
# End Source File
# Begin Source File

SOURCE=.\AbstractGraph.cpp
# End Source File
# Begin Source File

SOURCE=.\AbstractPath.cpp
# End Source File
# Begin Source File

SOURCE=.\BinaryHeap.cpp
# End Source File
# Begin Source File

SOURCE=.\Channel.cpp
# End Source File
# Begin Source File

SOURCE=.\Circuit.cpp
# End Source File
# Begin Source File

SOURCE=.\Connection.cpp
# End Source File
# Begin Source File

SOURCE=.\ConnectionDB.cpp
# End Source File
# Begin Source File

SOURCE=.\ConProvisionMain.cpp
# End Source File
# Begin Source File

SOURCE=.\Event.cpp
# End Source File
# Begin Source File

SOURCE=.\EventList.cpp
# End Source File
# Begin Source File

SOURCE=.\Graph.cpp
# End Source File
# Begin Source File

SOURCE=.\Lightpath.cpp
# End Source File
# Begin Source File

SOURCE=.\Lightpath_Seg.cpp
# End Source File
# Begin Source File

SOURCE=.\LightpathDB.cpp
# End Source File
# Begin Source File

SOURCE=.\Log.cpp
# End Source File
# Begin Source File

SOURCE=.\MappedLinkList.cpp
# End Source File
# Begin Source File

SOURCE=.\NetMan.cpp
# End Source File
# Begin Source File

SOURCE=.\OchException.cpp
# End Source File
# Begin Source File

SOURCE=.\OchMemDebug.cpp
# End Source File
# Begin Source File

SOURCE=.\OchObject.cpp
# End Source File
# Begin Source File

SOURCE=.\OXCNode.cpp
# End Source File
# Begin Source File

SOURCE=.\SimplexLink.cpp
# End Source File
# Begin Source File

SOURCE=.\Simulator.cpp
# End Source File
# Begin Source File

SOURCE=.\Stat.cpp
# End Source File
# Begin Source File

SOURCE=.\TopoReader.cpp
# End Source File
# Begin Source File

SOURCE=.\UniFiber.cpp
# End Source File
# Begin Source File

SOURCE=.\Vertex.cpp
# End Source File
# Begin Source File

SOURCE=.\WDMNetwork.cpp
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=.\AbsPath.h
# End Source File
# Begin Source File

SOURCE=.\AbstractGraph.h
# End Source File
# Begin Source File

SOURCE=.\AbstractPath.h
# End Source File
# Begin Source File

SOURCE=.\BinaryHeap.h
# End Source File
# Begin Source File

SOURCE=.\Channel.h
# End Source File
# Begin Source File

SOURCE=.\Circuit.h
# End Source File
# Begin Source File

SOURCE=.\Connection.h
# End Source File
# Begin Source File

SOURCE=.\ConnectionDB.h
# End Source File
# Begin Source File

SOURCE=.\ConstDef.h
# End Source File
# Begin Source File

SOURCE=.\Event.h
# End Source File
# Begin Source File

SOURCE=.\EventList.h
# End Source File
# Begin Source File

SOURCE=.\Graph.h
# End Source File
# Begin Source File

SOURCE=.\Lightpath.h
# End Source File
# Begin Source File

SOURCE=.\Lightpath_Seg.h
# End Source File
# Begin Source File

SOURCE=.\LightpathDB.h
# End Source File
# Begin Source File

SOURCE=.\Log.h
# End Source File
# Begin Source File

SOURCE=.\MacroDef.h
# End Source File
# Begin Source File

SOURCE=.\MappedLinkList.h
# End Source File
# Begin Source File

SOURCE=.\NetMan.h
# End Source File
# Begin Source File

SOURCE=.\OchDebug.h
# End Source File
# Begin Source File

SOURCE=.\OchException.h
# End Source File
# Begin Source File

SOURCE=.\OchInc.h
# End Source File
# Begin Source File

SOURCE=.\OchMemDebug.h
# End Source File
# Begin Source File

SOURCE=.\OchObject.h
# End Source File
# Begin Source File

SOURCE=.\OXCNode.h
# End Source File
# Begin Source File

SOURCE=.\SimplexLink.h
# End Source File
# Begin Source File

SOURCE=.\Simulator.h
# End Source File
# Begin Source File

SOURCE=.\Stat.h
# End Source File
# Begin Source File

SOURCE=.\TopoReader.h
# End Source File
# Begin Source File

SOURCE=.\TypeDef.h
# End Source File
# Begin Source File

SOURCE=.\UniFiber.h
# End Source File
# Begin Source File

SOURCE=.\Vertex.h
# End Source File
# Begin Source File

SOURCE=.\WDMNetwork.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# End Group
# End Target
# End Project
