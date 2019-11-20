CPP = g++-4.2
CPPOPT = -g -O2 -Wall -DNDEBUG
# CPPOPT = -g -D_OCHDEBUGA -DNDEBUG
LINKOPT =
CMD = $(CPP) $(CPPOPT)
# generate objective files under ./linuxd for debug; ./linuxr for release
ObjDir = linuxr
Executable = sim_linux

ObjFiles = $(ObjDir)/ConProvisionMain.o $(ObjDir)/Simulator.o $(ObjDir)/NetMan.o $(ObjDir)/Circuit.o $(ObjDir)/LightpathDB.o $(ObjDir)/Lightpath_Seg.o $(ObjDir)/Lightpath.o $(ObjDir)/ConnectionDB.o $(ObjDir)/Connection.o $(ObjDir)/TopoReader.o $(ObjDir)/Graph.o $(ObjDir)/SimplexLink.o $(ObjDir)/Vertex.o $(ObjDir)/WDMNetwork.o $(ObjDir)/UniFiber.o $(ObjDir)/OXCNode.o $(ObjDir)/Channel.o $(ObjDir)/EventList.o $(ObjDir)/Event.o $(ObjDir)/Log.o $(ObjDir)/Stat.o $(ObjDir)/AbstractPath.o $(ObjDir)/AbsPath.o $(ObjDir)/AbstractGraph.o $(ObjDir)/OchException.o $(ObjDir)/OchObject.o

$(Executable): $(ObjFiles)
	$(CMD) -o $@ $(ObjFiles)

$(ObjDir)/ConProvisionMain.o: ConProvisionMain.cpp
	$(CMD) -c ConProvisionMain.cpp -o $@

$(ObjDir)/Simulator.o: Simulator.cpp
	$(CMD) -c Simulator.cpp -o $@

$(ObjDir)/NetMan.o: NetMan.cpp
	$(CMD) -c NetMan.cpp -o $@

$(ObjDir)/Circuit.o: Circuit.cpp
	$(CMD) -c Circuit.cpp -o $@

$(ObjDir)/LightpathDB.o: LightpathDB.cpp
	$(CMD) -c LightpathDB.cpp -o $@

$(ObjDir)/Lightpath_Seg.o: Lightpath_Seg.cpp
	$(CMD) -c Lightpath_Seg.cpp -o $@

$(ObjDir)/Lightpath.o: Lightpath.cpp
	$(CMD) -c Lightpath.cpp -o $@

$(ObjDir)/ConnectionDB.o: ConnectionDB.cpp Connection.cpp
	$(CMD) -c ConnectionDB.cpp -o $@

$(ObjDir)/Connection.o: Connection.cpp
	$(CMD) -c Connection.cpp -o $@

$(ObjDir)/TopoReader.o: TopoReader.cpp WDMNetwork.h
	$(CMD) -c TopoReader.cpp -o $@

$(ObjDir)/Graph.o: Graph.cpp
	$(CMD) -c Graph.cpp -o $@

$(ObjDir)/SimplexLink.o: SimplexLink.cpp
	$(CMD) -c SimplexLink.cpp -o $@

$(ObjDir)/Vertex.o: Vertex.cpp
	$(CMD) -c Vertex.cpp -o $@

$(ObjDir)/WDMNetwork.o: WDMNetwork.cpp OchInc.h
	$(CMD) -c WDMNetwork.cpp -o $@

$(ObjDir)/UniFiber.o: UniFiber.cpp OchInc.h
	$(CMD) -c UniFiber.cpp -o $@

$(ObjDir)/OXCNode.o: OXCNode.cpp OchInc.h
	$(CMD) -c OXCNode.cpp -o $@

$(ObjDir)/Channel.o: Channel.cpp
	$(CMD) -c Channel.cpp -o $@

$(ObjDir)/EventList.o: EventList.cpp
	$(CMD) -c EventList.cpp -o $@

$(ObjDir)/Event.o: Event.cpp
	$(CMD) -c Event.cpp -o $@

$(ObjDir)/Log.o: Log.cpp
	$(CMD) -c Log.cpp -o $@

$(ObjDir)/Stat.o: Stat.cpp
	$(CMD) -c Stat.cpp -o $@

$(ObjDir)/AbstractPath.o: AbstractPath.cpp
	$(CMD) -c AbstractPath.cpp -o $@

$(ObjDir)/AbsPath.o: AbsPath.cpp
	$(CMD) -c AbsPath.cpp -o $@

$(ObjDir)/AbstractGraph.o: AbstractGraph.cpp
	$(CMD) -c AbstractGraph.cpp -o $@

$(ObjDir)/OchException.o: OchException.cpp OchException.h
	$(CMD) -c OchException.cpp -o $@

$(ObjDir)/OchObject.o: OchObject.cpp OchInc.h
	$(CMD) -c OchObject.cpp -o $@

clean:
	rm -f $(ObjDir)/*.o
