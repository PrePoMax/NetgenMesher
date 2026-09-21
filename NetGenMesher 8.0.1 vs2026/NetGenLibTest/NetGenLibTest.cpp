// NetGenLibTest.cpp : Defines the entry point for the console application.
//
#include "stdafx.h"
#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <vector>
#include <chrono>

#include <climits>
#include "TopTools_IndexedMapOfShape.hxx"
#include "TopoDS.hxx"
#include "TopoDS_Face.hxx"
#include "TopoDS_Shape.hxx"
#include "GProp_GProps.hxx"
#include "BRepGProp.hxx"

#include "windows.h"
//#include "nglib.h"

namespace nglib {
#include "nglib.h"
}

using namespace std;
using namespace nglib;


// Methods
void ConvertToUpper(char* data);
void PrintUsage(const char* exePath);
// Assembly - Split
bool AssemblySplitStepToCompounds(int argc, char* argv[]);
bool AssemblySplitIgesToCompounds(int argc, char* argv[]);
bool AssemblySplitBrepToCompounds(int argc, char* argv[]);
bool AssemblySplitBrepToParts(int argc, char* argv[]);
// Assembly - Create
bool CompoundBrep(int argc, char* argv[]);
bool AssemblyBrep(int argc, char* argv[], bool saveAsStep);
// Part
bool SaveBrepAsStep(int argc, char* argv[]);
// Visualization
bool OccVisualization(int argc, char* argv[]);
// Geometry operations
bool ScaleGeometry(int argc, char* argv[]);
bool ReverseBrepFaceOrientations(int argc, char* argv[]);
bool SplitAFaceUsingTwoPoints(int argc, char* argv[]);
bool SplitGeometry(int argc, char* argv[]);
bool RemoveSolids(int argc, char* argv[]);
bool ConvertToShell(int argc, char* argv[]);
bool ConvertToSolid(int argc, char* argv[]);
// Stl Mesh
bool StlMesh(int argc, char* argv[]);
bool StlSurfaceMesh(int argc, char* argv[]);
bool StlEdgeMesh(int argc, char* argv[]);
void ReadEdgeNodes(const char *fileName, int &numOfNodes, double** &nodes);
// Occ Mesh
bool OccMesh(int argc, char* argv[]);
bool OccEdgeMesh(int argc, char* argv[]);
Ng_Meshing_Parameters ReadParameters(const char *fileName, double &deflection);
// The rest
bool Test(int argc, char* argv[]);
char** ReadDataFile(int &argc, char* argv[]);


//BREP_MESH C:\Temp\test.brep C:\Temp\geometry.vol C:\Temp\meshParameters C:\Temp\meshRefinement
//BREP_MESH C:\Temp\Salome\2_box_Partition.brep C:\Temp\geometry.vol C:\Temp\meshParameters C:\Temp\meshRefinement
//BREP_MESH C:\Temp\geometry.brep C:\Temp\geometry.vol C:\Temp\meshParameters C:\Temp\meshRefinement
//BREP_MESH C:\Temp\test.brep C:\Temp\geometry.vol C:\Temp\meshParameters
//BREP_MESH C:\Temp\geometry_partition.brep C:\Temp\geometry_partition.vol C:\Temp\meshParameters C:\Temp\meshRefinement
//BREP_MESH C:\Temp\geometry_partition.brep C:\Temp\geometry_partition.vol C:\Temp\meshParameters C:\Temp\meshRefinement
//
//BREP_ASSEMBLY_SPLIT C:\Temp\Salome\2_box_Partition.brep C:\Temp\geometry.brep
//BREP_ASSEMBLY_SPLIT C:\Temp\Salome\3_box_Compound_Partition.brep C:\Temp\geometry.brep
//BREP_ASSEMBLY_SPLIT C:\Temp\Partition_1.brep C:\Temp\Partition_out.brep
//
//BREP_COMPOUND C:\Temp\box_1.brep C:\Temp\box_2.brep C:\Temp\geometry.brep
//BREP_COMPOUND C:\Temp\box_1.brep C:\Temp\cylinder_2.brep C:\Temp\geometry_partition.brep
//BREP_COMPOUND "C:\Temp\if91LnA0._1.brep" "C:\Temp\if91LnA0._2.brep" "C:\Temp\geometry.brep"
//BREP_COMPOUND "C:\Temp\r4sXIMtY._1.brep" "C:\Temp\r4sXIMtY._2.brep" "C:\Temp\geometry.brep"
//BREP_COMPOUND "C:\Temp\Partition_out_1.brep" "C:\Temp\Partition_out_2.brep" "C:\Temp\Partition_out_3.brep" "C:\Temp\geometry.brep"
//
//BREP_VISUALIZATION C:\Temp\box_2.brep C:\Temp\geometry.vis 0.01
//BREP_VISUALIZATION C:\Temp\lever.brep C:\Temp\geometry.vis 0.01
//BREP_VISUALIZATION C:\Temp\box_2.brep C:\Temp\geometry.vis 0.01
//BREP_VISUALIZATION C:\Temp\Partition_out_1.brep C:\Temp\geometry.vis 0.01
//BREP_VISUALIZATION C:\Temp\edge.brep C:\Temp\geometry.vis 0.01 
//
//BREP_SCALE_GEOMETRY C:\Temp\scale.brep C:\Temp\scale_out.brep 0 0 0 10 10 10
//
//SAVE_BREP_AS_STEP "C:\Temp\geometry.brep" "C:\Temp\geometry.step"
//
//BREP_SCALE_GEOMETRY "C:\\Temp\\geometry.brep\" "C:\\Temp\\geometry.brep\" 0 0 0 1 1 1
//BREP_SCALE_GEOMETRY "C:\Temp\geometry.brep" "C:\Temp\geometry.brep" 0 0 0 1 1 1
//
//STEP_ASSEMBLY_SPLIT_TO_COMPOUNDS "C:\Temp\ptc.stp" "C:\Temp\geometry.brep"
//BREP_CONVERT_SOLID_TO_SHELL "C:\Temp\solidBox.brep" "C:\Temp\geometry.brep"
//IGES_ASSEMBLY_SPLIT_TO_COMPOUNDS "C:\Temp\NX-2D-bracket.igs" "C:\Temp\geometry.brep"
//BREP_COMPOUND "C:\Temp\geometry_face_2.brep" "C:\Temp\geometry_face_3.brep" "C:\Temp\geometry.brep"

// MAIN
//int _tmain(int argc, _TCHAR* argv[])
static int RunCommand(int argc, char* argv[])
{
	// occ_visualization "C:\Temp\Bearing_bracket.STEP" "C:\Temp\Bearing_bracket.vis" 0.01
	// occ_visualization "C:\Temp\Bearing_bracket.STEP" "D:\Pc\Programiranje\NetGenLibOC v2\Working folder\part1.stl" "D:\Pc\Programiranje\NetGenLibOC v2\Working folder\parametres" "D:\Pc\Programiranje\NetGenLibOC v2\Working folder\part1.vol"

	// argv[0]	...	exe
	// argv[1]	...	function
	// argv[2]	...	input file nam
	// argv[3]	...	output file name
	// argv[4]	...	mesh parameters
	// argv[5]	...	mesh refinement
	// argv[6]	...	edge file name
	//
	if (argc < 2)
	{
		PrintUsage(argv[0]);
		return 1;
	}
	//
	ConvertToUpper(argv[1]);
	// Assembly - Split
	if (strcmp(argv[1], "STEP_ASSEMBLY_SPLIT_TO_COMPOUNDS") == 0) return AssemblySplitStepToCompounds(argc, argv);
	else if (strcmp(argv[1], "IGES_ASSEMBLY_SPLIT_TO_COMPOUNDS") == 0) return AssemblySplitIgesToCompounds(argc, argv);
	else if (strcmp(argv[1], "BREP_ASSEMBLY_SPLIT_TO_COMPOUNDS") == 0) return AssemblySplitBrepToCompounds(argc, argv);
	else if (strcmp(argv[1], "BREP_ASSEMBLY_SPLIT_TO_PARTS") == 0) return AssemblySplitBrepToParts(argc, argv);
	// Assembly - Create
	else if (strcmp(argv[1], "BREP_COMPOUND") == 0) return CompoundBrep(argc, argv);
	else if (strcmp(argv[1], "BREP_ASSEMBLY") == 0) return AssemblyBrep(argc, argv, false);
	else if (strcmp(argv[1], "STEP_ASSEMBLY") == 0) return AssemblyBrep(argc, argv, true);
	// Part
	else if (strcmp(argv[1], "SAVE_BREP_AS_STEP") == 0) return SaveBrepAsStep(argc, argv);
	// Visualization
	else if (strcmp(argv[1], "BREP_VISUALIZATION") == 0) return OccVisualization(argc, argv);
	// Geometry operations
	else if (strcmp(argv[1], "BREP_SCALE_GEOMETRY") == 0) return ScaleGeometry(argc, argv);
	else if (strcmp(argv[1], "BREP_REVERSE_FACES") == 0) return ReverseBrepFaceOrientations(argc, argv);
	else if (strcmp(argv[1], "BREP_SPLIT_A_FACE_USING_TWO_POINTS") == 0) return SplitAFaceUsingTwoPoints(argc, argv);
	else if (strcmp(argv[1], "BREP_SPLIT") == 0) return SplitGeometry(argc, argv);
	else if (strcmp(argv[1], "BREP_REMOVE_SOLIDS") == 0) return RemoveSolids(argc, argv);
	else if (strcmp(argv[1], "BREP_CONVERT_SOLID_TO_SHELL") == 0) return ConvertToShell(argc, argv);
	else if (strcmp(argv[1], "BREP_CONVERT_SHELL_TO_SOLID") == 0) return ConvertToSolid(argc, argv);
	// Stl Mesh
	else if (strcmp(argv[1], "STL_MESH") == 0) return StlMesh(argc, argv);
	else if (strcmp(argv[1], "STL_SURFACE_MESH") == 0) return StlSurfaceMesh(argc, argv);
	else if (strcmp(argv[1], "STL_EDGE_MESH") == 0) return StlEdgeMesh(argc, argv);
	// Occ Mesh
	else if (strcmp(argv[1], "BREP_MESH") == 0) return OccMesh(argc, argv);
	else if (strcmp(argv[1], "BREP_EDGE_MESH") == 0) return OccEdgeMesh(argc, argv);
	// The rest
	else if (strcmp(argv[1], "TEST") == 0) return Test(argc, argv);
	else
	{
		//cout << "Netgen (nglib) STL Testing" << endl;
		cerr << "The first parameter must be a valid command." << endl;
		return 1;
	}
	return 0;
}


// A meshing routine that fails reports it by throwing.  Without a handler the
// runtime terminates the process, which looks like a crash and hides the
// reason, so every command runs under this backstop.
int main(int argc, char* argv[])
{
	try
	{
		return RunCommand(argc, argv);
	}
	catch (const std::exception & exception)
	{
		cerr << "Error: " << exception.what() << endl;
		return 1;
	}
	catch (...)
	{
		cerr << "Error: the operation failed and could not be completed." << endl;
		return 1;
	}
}


// Methods
void ConvertToUpper(char* data)
{
	for (int i = 0; i < strlen(data); i++)
	{
		data[i] = toupper(data[i]);
	}
}
void PrintUsage(const char* exePath)
{
	// argv[0] may carry the full path; show just the file name
	string exeName = (exePath == NULL) ? "NetGenMesher.exe" : string(exePath);
	size_t slash = exeName.find_last_of("\\/");
	if (slash != string::npos) exeName = exeName.substr(slash + 1);
	//
	cout << endl;
	cout << "NetGenMesher - meshing and geometry utility built on Netgen 5.3.1 / OpenCASCADE 8.0.1" << endl;
	cout << endl;
	cout << "Usage:  " << exeName << " <COMMAND> <arguments>" << endl;
	cout << endl;
	cout << "The command is case insensitive. Arguments in [] are optional." << endl;
	cout << "Paths containing spaces must be quoted." << endl;
	cout << endl;
	cout << "Assembly - split" << endl;
	cout << "  STEP_ASSEMBLY_SPLIT_TO_COMPOUNDS   <in.step> <out.brep> [maxSplitFaces]" << endl;
	cout << "  IGES_ASSEMBLY_SPLIT_TO_COMPOUNDS   <in.iges> <out.brep> [maxSplitFaces]" << endl;
	cout << "  BREP_ASSEMBLY_SPLIT_TO_COMPOUNDS   <in.brep> <out.brep> [maxSplitFaces]" << endl;
	cout << "  BREP_ASSEMBLY_SPLIT_TO_PARTS       <in.brep> <out.brep> [maxSplitFaces]" << endl;
	cout << "      One file is written per part, named <out>_solid_0001.brep," << endl;
	cout << "      <out>_shell_0001.brep, ... maxSplitFaces defaults to 1." << endl;
	cout << endl;
	cout << "Assembly - create" << endl;
	cout << "  BREP_COMPOUND                      <in1.brep> <in2.brep> [...] <out.brep>" << endl;
	cout << "  BREP_ASSEMBLY                      <in1.brep> <in2.brep> [...] <out.brep>" << endl;
	cout << "  STEP_ASSEMBLY                      <in1.brep> <in2.brep> [...] <out.step>" << endl;
	cout << "      BREP_COMPOUND imprints the parts on one another; the two assembly" << endl;
	cout << "      commands only gather them into one compound, unchanged." << endl;
	cout << endl;
	cout << "Part" << endl;
	cout << "  SAVE_BREP_AS_STEP                  <in.brep> <out.step>" << endl;
	cout << endl;
	cout << "Visualization" << endl;
	cout << "  BREP_VISUALIZATION                 <in.brep> <out.vis> <linearDeflection>" << endl;
	cout << "                                     <relative> <angularDeflection>" << endl;
	cout << "      relative: 0 = absolute deflection, non-zero = relative to size." << endl;
	cout << endl;
	cout << "Geometry operations" << endl;
	cout << "  BREP_SCALE_GEOMETRY                <in.brep> <out.brep> <Tx> <Ty> <Tz>" << endl;
	cout << "                                     <scaleX> <scaleY> <scaleZ>" << endl;
	cout << "  BREP_REVERSE_FACES                 <in.brep> <out.brep>" << endl;
	cout << "  BREP_SPLIT_A_FACE_USING_TWO_POINTS <in.brep> <out.brep> <faceId>" << endl;
	cout << "                                     <x1> <y1> <z1> <x2> <y2> <z2>" << endl;
	cout << "  BREP_SPLIT                         <in.brep> <out.brep> <toolDefinition>" << endl;
	cout << "      <toolDefinition> is a text file of <property>: <values> lines that" << endl;
	cout << "      define the splitting tool.  Every tool accepts" << endl;
	cout << "        Tool type: Plane | ExtendFace | ExtrudeEdges | TwoPointLine" << endl;
	cout << "        Extent: x_min x_max y_min y_max z_min z_max" << endl;
	cout << "        Fuzzy value: <tolerance>                       (optional)" << endl;
	cout << "        Solid ids: <id> <id> ...                       (optional)" << endl;
	cout << "          cuts only those solids of the shape and gathers every solid," << endl;
	cout << "          cut or not, back into one compound" << endl;
	cout << "        Source file: <in.brep>                         (optional, repeatable)" << endl;
	cout << "          the edge ids that follow it are read from that file; ids given" << endl;
	cout << "          before any source file are read from the shape being split" << endl;
	cout << "        Split target: Shell                            (optional)" << endl;
	cout << "          divides the named faces and leaves the body they belong to" << endl;
	cout << "          whole, instead of cutting through it" << endl;
	cout << "        Target solid: <n>                              (optional, repeatable)" << endl;
	cout << "          the target face ids that follow it are numbered inside that" << endl;
	cout << "          solid of the shape; ids given before any target solid are" << endl;
	cout << "          numbered in the shape itself" << endl;
	cout << "        Target face ids: <id> <id> ...                 (with Split target)" << endl;
	cout << "      Plane adds         Plane origin: x y z" << endl;
	cout << "                         Plane normal: x y z" << endl;
	cout << "      ExtendFace adds    Face ids: <id> <id> ..." << endl;
	cout << "      ExtrudeEdges adds  Edge ids: <id> <id> ..." << endl;
	cout << "      TwoPointLine adds  Point 1: x y z" << endl;
	cout << "                         Point 2: x y z" << endl;
	cout << "          draws the line between the two points in the parameter space" << endl;
	cout << "          of each target surface and divides it along that line; it" << endl;
	cout << "          needs Split target and cannot cut through a body" << endl;
	cout << "                         Extrusion offset: x y z      (optional)" << endl;
	cout << "                         Extrusion vector: x y z" << endl;
	cout << "      The extent is the box the tool has to cover, so that one tool is the" << endl;
	cout << "      same for every part it is applied to.  Face and edge ids are read from" << endl;
	cout << "      the source file when one is given, otherwise from the input file." << endl;
	cout << "  BREP_CONVERT_SOLID_TO_SHELL        <in.brep> <out.brep>" << endl;
	cout << "  BREP_CONVERT_SHELL_TO_SOLID        <in.brep> <out.brep>" << endl;
	cout << endl;
	cout << "STL meshing" << endl;
	cout << "  STL_MESH                           <in.stl> <out.vol> <parameters>" << endl;
	cout << "                                     [refinement] [edgeNodes]" << endl;
	cout << "  STL_SURFACE_MESH                   <in.stl> <out.vol> <parameters>" << endl;
	cout << "                                     [refinement] [edgeNodes]" << endl;
	cout << "  STL_EDGE_MESH                      <in.stl> <out.vol> <parameters>" << endl;
	cout << "                                     [refinement] [edgeNodes]" << endl;
	cout << endl;
	cout << "BREP meshing" << endl;
	cout << "  BREP_MESH                          <in.brep> <out.vol> <parameters>" << endl;
	cout << "                                     [refinement]" << endl;
	cout << "  BREP_EDGE_MESH                     <in.brep> <out.vol> <parameters>" << endl;
	cout << "                                     [refinement]" << endl;
	cout << endl;
	cout << "  <parameters> is a text file of meshing parameters; [refinement] is a" << endl;
	cout << "  local mesh size file; [edgeNodes] is a file of prescribed edge nodes." << endl;
	cout << endl;
	cout << "Examples" << endl;
	cout << "  " << exeName << " STL_MESH part1.stl part1.vol parametres" << endl;
	cout << "  " << exeName << " STEP_ASSEMBLY_SPLIT_TO_COMPOUNDS in.step geometry.brep" << endl;
	cout << "  " << exeName << " BREP_MESH geometry_solid_0001.brep out.vol parametres" << endl;
	cout << endl;
}
// Assembly - Split
bool AssemblySplitStepToCompounds(int argc, char* argv[])
{
	//Define pointer to OCC Geometry
	Ng_OCC_Geometry *occ_geom;
	// Read in the OCC File
	occ_geom = Ng_OCC_Load_STEP(argv[2]);
	if (!occ_geom)
	{
		cout << "Error reading in STEP File: " << argv[2] << endl;
		return 1;
	}
	cout << "Successfully loaded STEP File: " << argv[2] << endl;
	// Save as parts
	int numOfSplitFaces = 1;
	if (argc == 5)
	{
		std::string::size_type sz;     // alias of size_t
		numOfSplitFaces = std::stoi(argv[4], &sz);
	}
	Ng_OCC_WriteAssemblyAsCompounds(occ_geom, argv[3], numOfSplitFaces, true);
	//
	return 0;
}
bool AssemblySplitIgesToCompounds(int argc, char* argv[])
{
	//Define pointer to OCC Geometry
	Ng_OCC_Geometry *occ_geom;
	// Read in the OCC File
	occ_geom = Ng_OCC_Load_IGES(argv[2]);
	if (!occ_geom)
	{
		cout << "Error reading in IGES File: " << argv[2] << endl;
		return 1;
	}
	cout << "Successfully loaded IGES File: " << argv[2] << endl;
	int numOfSplitFaces = 1;
	if (argc == 5)
	{
		std::string::size_type sz;     // alias of size_t
		numOfSplitFaces = std::stoi(argv[4], &sz);
	}
	Ng_OCC_WriteAssemblyAsCompounds(occ_geom, argv[3], numOfSplitFaces, true);
	//
	return 0;
}
bool AssemblySplitBrepToCompounds(int argc, char* argv[])
{
	//Define pointer to OCC Geometry
	Ng_OCC_Geometry *occ_geom;
	// Read in the OCC File
	occ_geom = Ng_OCC_Load_BREP(argv[2]);
	if (!occ_geom)
	{
		cout << "Error reading in BREP File: " << argv[2] << endl;
		return 1;
	}
	cout << "Successfully loaded BREP File: " << argv[2] << endl;
	// Save as parts
	int numOfSplitFaces = 1;
	if (argc == 5)
	{
		std::string::size_type sz;     // alias of size_t
		numOfSplitFaces = std::stoi(argv[4], &sz);
	}
	Ng_OCC_WriteAssemblyAsCompounds(occ_geom, argv[3], numOfSplitFaces, false);
	//
	return 0;
}
bool AssemblySplitBrepToParts(int argc, char* argv[])
{
	//Define pointer to OCC Geometry
	Ng_OCC_Geometry *occ_geom;

	// Read in the OCC File
	occ_geom = Ng_OCC_Load_BREP(argv[2]);
	if (!occ_geom)
	{
		cout << "Error reading in BREP File: " << argv[2] << endl;
		return 1;
	}
	cout << "Successfully loaded BREP File: " << argv[2] << endl;
	// Save parts
	int numOfSplitFaces = 1;
	if (argc == 5)
	{
		std::string::size_type sz;     // alias of size_t
		numOfSplitFaces = std::stoi(argv[4], &sz);
	}
	int numOfSolids = Ng_OCC_WriteAssemblyAsSolidParts(occ_geom, 1, argv[3], numOfSplitFaces) - 1;
	int numOfShells = Ng_OCC_WriteAssemblyAsShellParts(occ_geom, numOfSolids, argv[3], numOfSplitFaces) - numOfSolids;

	return 0;
}
// Assembly - Create
bool CompoundBrep(int argc, char* argv[])
{
	try
	{
		// Result of Netgen Operations
		Ng_Result ng_res;
		//
		ng_res = Ng_OCC_CreateCompoundFromBrep(argc, argv);
		//
		if (ng_res != NG_OK)
		{
			cout << "Error combining BREP files." << endl;
			return 1;
		}
		return 0;
	}
	catch (exception ex)
	{
		return 1;
	}
}
bool AssemblyBrep(int argc, char* argv[], bool saveAsStep)
{
	try
	{
		// Result of Netgen Operations
		Ng_Result ng_res;
		//
		ng_res = Ng_OCC_CreateAssemblyFromBrep(argc, argv, saveAsStep);
		//
		if (ng_res != NG_OK)
		{
			cout << "Error joining BREP files." << endl;
			return 1;
		}
		return 0;
	}
	catch (exception ex)
	{
		return 1;
	}
}
// Part
bool SaveBrepAsStep(int argc, char* argv[])
{
	//Define pointer to OCC Geometry
	Ng_OCC_Geometry *occ_geom;
	// Read in the OCC File
	occ_geom = Ng_OCC_Load_BREP(argv[2]);
	if (!occ_geom)
	{
		cout << "Error reading in BREP File: " << argv[2] << endl;
		return 1;
	}
	cout << "Successfully loaded BREP File: " << argv[2] << endl;
	// Save
	Ng_OCC_Save_STEP(occ_geom, argv[3]);
	//
	return 0;
}
// Visualization
bool OccVisualization(int argc, char* argv[])
{
	//Define pointer to OCC Geometry
	Ng_OCC_Geometry *occ_geom;
	// Read in the OCC File
	cout << "Ng_OCC_Load_BREP" << endl;
	occ_geom = Ng_OCC_Load_BREP(argv[2]);
	if (!occ_geom)
	{
		cout << "Error reading in BREP File: " << argv[2] << endl;
		return 1;
	}
	cout << "Successfully loaded BREP File: " << argv[2] << endl;
	//
	std::string::size_type sz1;     // alias of size_t
	double linearDeflection = std::stod(argv[4], &sz1);
	//
	std::string::size_type sz2;     // alias of size_t
	bool relative = std::stod(argv[5], &sz2);
	//
	std::string::size_type sz3;     // alias of size_t
	double angularDeflection = std::stod(argv[6], &sz3);
	//
	cout << "Ng_OCC_ExportVisualization" << endl;
	Ng_OCC_ExportVisualization(occ_geom, linearDeflection, relative, angularDeflection, argv[3]);
	cout << "Export visualization finished successfully." << endl;
	//
	return 0;
}
// Geometry operations
bool ScaleGeometry(int argc, char* argv[])
{
	try
	{
		//Define pointer to OCC Geometry
		Ng_OCC_Geometry *occ_geom;
		// Result of Netgen Operations
		Ng_Result ng_res;
		int np, ne;
		// Initialise the Netgen Core library
		Ng_Init();
		// Read in the OCC File
		cout << "Ng_OCC_Load_BREP" << endl;
		occ_geom = Ng_OCC_Load_BREP(argv[2]);
		if (!occ_geom)
		{
			cout << "Error reading in BREP File: " << argv[2] << endl;
			return 1;
		}
		cout << "Successfully loaded BREP File: " << argv[2] << endl;
		//
		std::string::size_type sz;     // alias of size_t
		double Tx = std::stod(argv[4], &sz);
		double Ty = std::stod(argv[5], &sz);
		double Tz = std::stod(argv[6], &sz);
		double scaleX = std::stod(argv[7], &sz);
		double scaleY = std::stod(argv[8], &sz);
		double scaleZ = std::stod(argv[9], &sz);
		//
		cout << "Ng_OCC_ScaleGeometry" << endl;
		occ_geom = Ng_OCC_ScaleGeometry(occ_geom, Tx, Ty, Tz, scaleX, scaleY, scaleZ);
		// Save
		cout << "Ng_OCC_Save_BREP" << endl;
		Ng_OCC_Save_BREP(occ_geom, argv[3]);
		//
		cout << "Scale geometry finished successfully." << endl;
		//
		return 0;
	}
	catch (exception ex)
	{
		return 1;
	}
}
bool ReverseBrepFaceOrientations(int argc, char* argv[])
{
	try
	{
		//Define pointer to OCC Geometry
		Ng_OCC_Geometry *occ_geom;
		// Result of Netgen Operations
		Ng_Result ng_res;
		int np, ne;
		// Initialise the Netgen Core library
		Ng_Init();
		// Read in the OCC File
		cout << "Ng_OCC_Load_BREP" << endl;
		occ_geom = Ng_OCC_Load_BREP(argv[2]);
		if (!occ_geom)
		{
			cout << "Error reading in BREP File: " << argv[2] << endl;
			return 1;
		}
		cout << "Successfully loaded BREP File: " << argv[2] << endl;
		//
		int faceId;
		int numOfFaceIds = argc - 4;
		int * faceIds = new int[numOfFaceIds];
		std::string::size_type sz;     // alias of size_t
		for (int i = 0; i < numOfFaceIds; i++)
		{
			faceId = std::stoi(argv[4 + i], &sz);
			faceIds[i] = faceId;
		}
		//
		cout << "Ng_OCC_ReverseBrepFaceOrientations" << endl;
		occ_geom = Ng_OCC_ReverseFaceOrientations(occ_geom, faceIds, numOfFaceIds);
		// Save
		cout << "Ng_OCC_Save_BREP" << endl;
		Ng_OCC_Save_BREP(occ_geom, argv[3]);
		//
		cout << "Reverse face orientations finished successfully." << endl;
		//
		return 0;
	}
	catch (exception ex)
	{
		return 1;
	}
}
bool SplitAFaceUsingTwoPoints(int argc, char* argv[])
{
	try
	{
		//Define pointer to OCC Geometry
		Ng_OCC_Geometry *occ_geom;
		// Result of Netgen Operations
		Ng_Result ng_res;
		int np, ne;
		// Initialise the Netgen Core library
		Ng_Init();
		// Read in the OCC File
		cout << "Ng_OCC_Load_BREP" << endl;
		occ_geom = Ng_OCC_Load_BREP(argv[2]);
		if (!occ_geom)
		{
			cout << "Error reading in BREP File: " << argv[2] << endl;
			return 1;
		}
		cout << "Successfully loaded BREP File: " << argv[2] << endl;
		//
		std::string::size_type sz;     // alias of size_t
		int faceId = std::stoi(argv[4], &sz);
		double x1 = std::stod(argv[5], &sz);
		double y1 = std::stod(argv[6], &sz);
		double z1 = std::stod(argv[7], &sz);
		double x2 = std::stod(argv[8], &sz);
		double y2 = std::stod(argv[9], &sz);
		double z2 = std::stod(argv[10], &sz);
		//
		cout << "Ng_OCC_SplitAFaceUsingTwoPoints" << endl;
		occ_geom = Ng_OCC_SplitAFaceUsingTwoPoints(occ_geom, faceId, x1, y1, z1, x2, y2, z2);
		// Save
		cout << "Ng_OCC_Save_BREP" << endl;
		Ng_OCC_Save_BREP(occ_geom, argv[3]);
		//
		cout << "Split face finished successfully." << endl;
		//
		return 0;
	}
	catch (exception ex)
	{
		return 1;
	}
}
// A tool definition file holds one <property>: <values> line per property, in the style of the
// visualization file.  Unknown properties are ignored so that a property can be added without
// breaking an older executable, and a missing property keeps its default.
static string TrimToolText(const string & text)
{
	size_t first = text.find_first_not_of(" \t\r\n");
	if (first == string::npos) return "";
	size_t last = text.find_last_not_of(" \t\r\n");
	return text.substr(first, last - first + 1);
}
static string ToolKeyFromText(const string & text)
{
	string key;
	for (size_t i = 0; i < text.size(); i++)
	{
		if (text[i] == ' ' || text[i] == '\t') continue;
		key += (char)tolower(text[i]);
	}
	return key;
}
static void ReadToolValues(const string & text, double * values, int numOfValues)
{
	istringstream stream(text);
	for (int i = 0; i < numOfValues; i++) stream >> values[i];
}
// The solids that are left keep their shapes, so a part built from one of them keeps every id
// the geometry gave it and PrePoMax can leave the part it already holds alone
bool RemoveSolids(int argc, char* argv[])
{
	try
	{
		if (argc < 5)
		{
			cerr << "The BREP_REMOVE_SOLIDS command needs an input file, an output file and at least "
			     << "one solid id." << endl;
			return 1;
		}
		//
		vector<int> solidIds;
		for (int i = 4; i < argc; i++)
		{
			istringstream stream(argv[i]);
			int solidId;
			while (stream >> solidId) solidIds.push_back(solidId);
		}
		if (solidIds.size() == 0)
		{
			cerr << "No solid id was given." << endl;
			return 1;
		}
		// Initialise the Netgen Core library
		Ng_Init();
		// Read in the OCC File
		cout << "Ng_OCC_Load_BREP" << endl;
		Ng_OCC_Geometry * occ_geom = Ng_OCC_Load_BREP(argv[2]);
		if (!occ_geom)
		{
			cout << "Error reading in BREP File: " << argv[2] << endl;
			return 1;
		}
		cout << "Successfully loaded BREP File: " << argv[2] << endl;
		//
		TopTools_IndexedMapOfShape RemovalFMap;
		Ng_OCC_TopTools_IndexedMapOfShape * occ_removal_fmap = (Ng_OCC_TopTools_IndexedMapOfShape*)&RemovalFMap;
		Ng_OCC_GetFMap(occ_geom, occ_removal_fmap);
		int numOfFacesBefore = RemovalFMap.Extent();
		//
		cout << "Ng_OCC_RemoveSolids" << endl;
		occ_geom = Ng_OCC_RemoveSolids(occ_geom, &solidIds[0], (int)solidIds.size());
		//
		RemovalFMap.Clear();
		Ng_OCC_GetFMap(occ_geom, occ_removal_fmap);
		if (RemovalFMap.Extent() >= numOfFacesBefore)
		{
			cerr << "No solid was removed from the geometry." << endl;
			return 1;
		}
		// Save
		cout << "Ng_OCC_Save_BREP" << endl;
		Ng_OCC_Save_BREP(occ_geom, argv[3]);
		//
		cout << "Remove solids finished successfully: " << numOfFacesBefore - RemovalFMap.Extent()
		     << " faces were removed with them." << endl;
	}
	catch (const std::exception& e)
	{
		cerr << "Error: " << e.what() << endl;
		return 1;
	}
	return 0;
}
bool SplitGeometry(int argc, char* argv[])
{
	try
	{
		if (argc < 5)
		{
			cerr << "The BREP_SPLIT command needs an input file, an output file and a tool definition file." << endl;
			return 1;
		}
		// The face count is what says whether the split happened, since the modelling kernel returns
		// a shape whether or not it made the cut
		TopTools_IndexedMapOfShape FMap;
		Ng_OCC_TopTools_IndexedMapOfShape * occ_fmap = (Ng_OCC_TopTools_IndexedMapOfShape*)&FMap;
		// Initialise the Netgen Core library
		Ng_Init();
		// Read in the OCC File
		cout << "Ng_OCC_Load_BREP" << endl;
		Ng_OCC_Geometry * occ_geom = Ng_OCC_Load_BREP(argv[2]);
		if (!occ_geom)
		{
			cout << "Error reading in BREP File: " << argv[2] << endl;
			return 1;
		}
		cout << "Successfully loaded BREP File: " << argv[2] << endl;
		//
		Ng_OCC_GetFMap(occ_geom, occ_fmap);
		int numOfFacesBefore = FMap.Extent();
		// Read the tool definition
		ifstream toolFile(argv[4]);
		if (!toolFile)
		{
			cerr << "Error reading the tool definition file: " << argv[4] << endl;
			return 1;
		}
		//
		string toolType;
		vector<string> sourceFileNames;
		vector<int> edgeIdCounts;
		double fuzzyValue = 0;
		double extent[6] = { 0, 0, 0, 0, 0, 0 };
		double origin[3] = { 0, 0, 0 };
		double normal[3] = { 0, 0, 1 };
		double extrusionOffset[3] = { 0, 0, 0 };
		double extrusionVector[3] = { 0, 0, 0 };
		vector<int> faceIds;
		vector<int> faceIdCounts;
		vector<int> edgeIds;
		vector<int> solidIds;
		string splitTarget;
		vector<int> targetSolids;
		vector<int> targetFaceCounts;
		vector<int> targetFaceIds;
		double firstPoint[3] = { 0, 0, 0 };
		double secondPoint[3] = { 0, 0, 0 };
		//
		string line;
		while (getline(toolFile, line))
		{
			size_t colon = line.find(':');
			if (colon == string::npos) continue;
			string key = ToolKeyFromText(line.substr(0, colon));
			string value = TrimToolText(line.substr(colon + 1));
			//
			if (key == "tooltype") toolType = ToolKeyFromText(value);
			// Each source file opens a group; the edge ids that follow it are read from that file
			else if (key == "sourcefile")
			{
				sourceFileNames.push_back(value);
				edgeIdCounts.push_back(0);
				faceIdCounts.push_back(0);
			}
			else if (key == "fuzzyvalue") fuzzyValue = stod(value);
			else if (key == "extent") ReadToolValues(value, extent, 6);
			else if (key == "planeorigin") ReadToolValues(value, origin, 3);
			else if (key == "planenormal") ReadToolValues(value, normal, 3);
			else if (key == "point1") ReadToolValues(value, firstPoint, 3);
			else if (key == "point2") ReadToolValues(value, secondPoint, 3);
			else if (key == "faceid" || key == "faceids")
			{
				istringstream stream(value);
				int id;
				int numOfIds = 0;
				while (stream >> id) { faceIds.push_back(id); numOfIds++; }
				// Ids given before any source file are read from the shape being split
				if (faceIdCounts.size() == 0) faceIdCounts.push_back(numOfIds);
				else faceIdCounts[faceIdCounts.size() - 1] += numOfIds;
			}
			else if (key == "extrusionoffset") ReadToolValues(value, extrusionOffset, 3);
			else if (key == "extrusionvector") ReadToolValues(value, extrusionVector, 3);
			else if (key == "edgeids")
			{
				istringstream stream(value);
				int edgeId;
				int numOfIds = 0;
				while (stream >> edgeId) { edgeIds.push_back(edgeId); numOfIds++; }
				// Ids given before any source file are read from the shape being split
				if (edgeIdCounts.size() == 0) edgeIdCounts.push_back(numOfIds);
				else edgeIdCounts[edgeIdCounts.size() - 1] += numOfIds;
			}
			else if (key == "solidids")
			{
				istringstream stream(value);
				int solidId;
				while (stream >> solidId) solidIds.push_back(solidId);
			}
			else if (key == "splittarget") splitTarget = ToolKeyFromText(value);
			// Each target solid opens a group; the face ids that follow it are numbered inside that solid
			else if (key == "targetsolid")
			{
				targetSolids.push_back(stoi(value));
				targetFaceCounts.push_back(0);
			}
			else if (key == "targetfaceid" || key == "targetfaceids")
			{
				istringstream stream(value);
				int id;
				int numOfIds = 0;
				while (stream >> id) { targetFaceIds.push_back(id); numOfIds++; }
				// Ids given before any target solid are numbered in the shape being split
				if (targetFaceCounts.size() == 0)
				{
					targetSolids.push_back(0);
					targetFaceCounts.push_back(numOfIds);
				}
				else targetFaceCounts[targetFaceCounts.size() - 1] += numOfIds;
			}
		}
		toolFile.close();
		// The tool geometry may live on other parts than the geometry being split, and on more than
		// one of them, because a compound is split as a whole while its pieces carry the picked edges
		vector<Ng_OCC_Geometry *> source_geoms;
		for (size_t i = 0; i < sourceFileNames.size(); i++)
		{
			cout << "Ng_OCC_Load_BREP" << endl;
			Ng_OCC_Geometry * source_geom = Ng_OCC_Load_BREP(sourceFileNames[i].c_str());
			if (!source_geom)
			{
				cout << "Error reading in the source BREP File: " << sourceFileNames[i] << endl;
				return 1;
			}
			cout << "Successfully loaded the source BREP File: " << sourceFileNames[i] << endl;
			source_geoms.push_back(source_geom);
		}
		//
		int * solidIdsData = solidIds.size() == 0 ? NULL : &solidIds[0];
		int numOfSolidIds = (int)solidIds.size();
		// A shell split divides the faces it is given and leaves the body they belong to whole, so
		// it has to be told which faces those are. Without the target the tool cuts through, as before.
		bool splitShell = splitTarget == "shell";
		if (splitShell && targetFaceCounts.size() == 0)
		{
			cerr << "A shell split needs the ids of the surfaces to split." << endl;
			return 1;
		}
		int * targetSolidsData = !splitShell || targetSolids.size() == 0 ? NULL : &targetSolids[0];
		int * targetFaceCountsData = !splitShell || targetFaceCounts.size() == 0 ?
		                             NULL : &targetFaceCounts[0];
		int * targetFaceIdsData = !splitShell || targetFaceIds.size() == 0 ? NULL : &targetFaceIds[0];
		int numOfTargetGroups = splitShell ? (int)targetFaceCounts.size() : 0;
		//
		if (toolType == "plane")
		{
			cout << "Ng_OCC_SplitWithPlane" << endl;
			occ_geom = Ng_OCC_SplitWithPlane(occ_geom, origin, normal, extent, fuzzyValue, solidIdsData,
			                                 numOfSolidIds, targetSolidsData, targetFaceCountsData,
			                                 numOfTargetGroups, targetFaceIdsData);
		}
		else if (toolType == "extendface")
		{
			cout << "Ng_OCC_SplitWithExtendedFace" << endl;
			occ_geom = Ng_OCC_SplitWithExtendedFace(occ_geom,
			                                        source_geoms.size() == 0 ? NULL : &source_geoms[0],
			                                        faceIdCounts.size() == 0 ? NULL : &faceIdCounts[0],
			                                        (int)faceIdCounts.size(),
			                                        faceIds.size() == 0 ? NULL : &faceIds[0],
			                                        extent, fuzzyValue, solidIdsData, numOfSolidIds,
			                                        targetSolidsData, targetFaceCountsData,
			                                        numOfTargetGroups, targetFaceIdsData);
		}
		else if (toolType == "extrudeedges")
		{
			cout << "Ng_OCC_SplitWithExtrudedEdges" << endl;
			occ_geom = Ng_OCC_SplitWithExtrudedEdges(occ_geom,
			                                         source_geoms.size() == 0 ? NULL : &source_geoms[0],
			                                         edgeIdCounts.size() == 0 ? NULL : &edgeIdCounts[0],
			                                         (int)edgeIdCounts.size(),
			                                         edgeIds.size() == 0 ? NULL : &edgeIds[0],
			                                         extrusionOffset, extrusionVector, fuzzyValue,
			                                         solidIdsData, numOfSolidIds, targetSolidsData,
			                                         targetFaceCountsData, numOfTargetGroups,
			                                         targetFaceIdsData);
		}
		else if (toolType == "twopointline")
		{
			// The line belongs to the surface it is drawn on, so this tool divides faces and nothing else
			if (numOfTargetGroups == 0)
			{
				cerr << "A split along the line between two points needs the ids of the surfaces to split."
				     << endl;
				return 1;
			}
			cout << "Ng_OCC_SplitWithSurfaceLine" << endl;
			occ_geom = Ng_OCC_SplitWithSurfaceLine(occ_geom, firstPoint, secondPoint, fuzzyValue,
			                                       targetSolidsData, targetFaceCountsData, numOfTargetGroups,
			                                       targetFaceIdsData);
		}
		else
		{
			cerr << "The splitting tool type is not supported: " << toolType << endl;
			return 1;
		}
		//
		FMap.Clear();
		Ng_OCC_GetFMap(occ_geom, occ_fmap);
		int numOfFacesAfter = FMap.Extent();
		//
		if (numOfFacesAfter <= numOfFacesBefore)
		{
			cerr << "The splitting tool did not split the geometry." << endl;
			return 1;
		}
		// Save
		cout << "Ng_OCC_Save_BREP" << endl;
		Ng_OCC_Save_BREP(occ_geom, argv[3]);
		//
		cout << "Split geometry finished successfully: " << numOfFacesAfter - numOfFacesBefore
		     << " faces were added." << endl;
		//
		return 0;
	}
	catch (exception ex)
	{
		return 1;
	}
}
bool ConvertToShell(int argc, char* argv[])
{
	try
	{
		//Define pointer to OCC Geometry
		Ng_OCC_Geometry* occ_geom;
		// Result of Netgen Operations
		Ng_Result ng_res;
		int np, ne;
		// Initialise the Netgen Core library
		Ng_Init();
		// Read in the OCC File
		cout << "Ng_OCC_Load_BREP" << endl;
		occ_geom = Ng_OCC_Load_BREP(argv[2]);
		if (!occ_geom)
		{
			cout << "Error reading in BREP File: " << argv[2] << endl;
			return 1;
		}
		cout << "Successfully loaded BREP File: " << argv[2] << endl;
		//
		cout << "Ng_OCC_ConvertToShell" << endl;
		occ_geom = Ng_OCC_ConvertToShell(occ_geom);
		// Save
		cout << "Ng_OCC_Save_BREP" << endl;
		Ng_OCC_Save_BREP(occ_geom, argv[3]);
		//
		cout << "Convert ot shell finished successfully." << endl;
		//
		return 0;
	}
	catch (exception ex)
	{
		return 1;
	}
}
bool ConvertToSolid(int argc, char* argv[])
{
	try
	{
		//Define pointer to OCC Geometry
		Ng_OCC_Geometry* occ_geom;
		// Result of Netgen Operations
		Ng_Result ng_res;
		int np, ne;
		// Initialise the Netgen Core library
		Ng_Init();
		// Read in the OCC File
		cout << "Ng_OCC_Load_BREP" << endl;
		occ_geom = Ng_OCC_Load_BREP(argv[2]);
		if (!occ_geom)
		{
			cout << "Error reading in BREP File: " << argv[2] << endl;
			return 1;
		}
		cout << "Successfully loaded BREP File: " << argv[2] << endl;
		//
		cout << "Ng_OCC_ConvertToSolid" << endl;
		occ_geom = Ng_OCC_ConvertToSolid(occ_geom);
		// Save
		cout << "Ng_OCC_Save_BREP" << endl;
		Ng_OCC_Save_BREP(occ_geom, argv[3]);
		//
		cout << "Convert ot solid finished successfully." << endl;
		//
		return 0;
	}
	catch (exception ex)
	{
		return 1;
	}
}
// Stl Mesh
bool StlMesh(int argc, char* argv[])
{
	// Define pointer to a new Netgen Mesh
	Ng_Mesh *mesh;

	// Define pointer to STL Geometry
	Ng_STL_Geometry *stl_geom;

	// Result of Netgen Operations
	Ng_Result ng_res;

	// Initialise the Netgen Core library
	Ng_Init();

	// Actually create the mesh structure
	mesh = Ng_NewMesh();

	int np, ne;

	// Read in the STL File
	stl_geom = Ng_STL_LoadGeometry(argv[2]);
	if (!stl_geom)
	{
		cout << "Error reading in STL File: " << argv[2] << endl;
		return 1;
	}
	cout << "Successfully loaded STL File: " << argv[2] << endl;

	// Set the Meshing Parameters to be used
	double deflection = 0;
	Ng_Meshing_Parameters mp;

	mp = ReadParameters(argv[4], deflection);
	mp.meshsize_filename = argc >= 6 ? argv[5] : NULL;

	// Add user edges if any
	if (argc >= 7)
	{
		int numOfNodes;
		double **nodes;
		ReadEdgeNodes(argv[6], numOfNodes, nodes);

		if (numOfNodes > 0)
		{
			cout << "Adding user edges...." << endl;
			for (int i = 0; i < numOfNodes / 2; i++)
			{
				Ng_STL_AddEdge(stl_geom, nodes[2 * i], nodes[2 * i + 1]);
			}

			for (int i = 0; i < numOfNodes; i++)
				delete[] nodes[i];
			delete[] nodes;
		}
	}

	cout << "Initialise the STL Geometry structure...." << endl;
	ng_res = Ng_STL_InitSTLGeometry(stl_geom);
	if (ng_res != NG_OK)
	{
		cout << "Error Initialising the STL Geometry....Aborting!!" << endl;
		return 1;
	}

	cout << "Start Edge Meshing...." << endl;
	ng_res = Ng_STL_MakeEdges(stl_geom, mesh, &mp);
	if (ng_res != NG_OK)
	{
		cout << "Error in Edge Meshing....Aborting!!" << endl;
		return 1;
	}

	cout << "Start Surface Meshing...." << endl;
	ng_res = Ng_STL_GenerateSurfaceMesh(stl_geom, mesh, &mp);
	if (ng_res != NG_OK)
	{
		cout << "Error in Surface Meshing....Aborting!!" << endl;
		return 1;
	}

	cout << "Start Volume Meshing...." << endl;
	ng_res = Ng_GenerateVolumeMesh(mesh, &mp);
	if (ng_res != NG_OK)
	{
		cout << "Error in Volume Meshing....Aborting!!" << endl;
		return 1;
	}

	if (mp.second_order > 0)
	{
		cout << endl << "Converting mesh to second order elements...." << endl << endl;
		Ng_STL_Generate_SecondOrder(stl_geom, mesh);
	}

	cout << "Meshing successfully completed....!!" << endl << endl;

	// volume mesh output
	np = Ng_GetNP(mesh);
	cout << "Points: " << np << endl;

	ne = Ng_GetNE(mesh);
	cout << "Elements: " << ne << endl;

	// refinement without geomety adaption:
	//Ng_Uniform_Refinement(mesh);

	// refinement with geomety adaption:   
	//Ng_STL_Uniform_Refinement(stl_geom, mesh);

	//cout << "elements after refinement: " << Ng_GetNE(mesh) << endl;
	//cout << "points   after refinement: " << Ng_GetNP(mesh) << endl;

	//cout << "Saving Mesh in VOL Format...." << endl;
	Ng_SaveMesh(mesh, argv[3]);

	Ng_Exit();

	//string mystr;
	//cout << "Press enter to continue...";
	//getline(cin, mystr);

	return 0;
}
bool StlSurfaceMesh(int argc, char* argv[])
{
	// Define pointer to a new Netgen Mesh
	Ng_Mesh *mesh;

	// Define pointer to STL Geometry
	Ng_STL_Geometry *stl_geom;

	// Result of Netgen Operations
	Ng_Result ng_res;

	// Initialise the Netgen Core library
	Ng_Init();

	// Actually create the mesh structure
	mesh = Ng_NewMesh();

	int np, ne;

	// Read in the STL File
	stl_geom = Ng_STL_LoadGeometry(argv[2]);
	if (!stl_geom)
	{
		cout << "Error reading in STL File: " << argv[2] << endl;
		return 1;
	}
	cout << "Successfully loaded STL File: " << argv[2] << endl;

	// Set the Meshing Parameters to be used
	double deflection = 0;
	Ng_Meshing_Parameters mp;

	mp = ReadParameters(argv[4], deflection);
	mp.meshsize_filename = argc >= 6 ? argv[5] : NULL;

	// Add user edges if any
	if (argc >= 7)
	{
		int numOfNodes;
		double **nodes;
		ReadEdgeNodes(argv[6], numOfNodes, nodes);

		if (numOfNodes > 0)
		{
			cout << "Adding user edges...." << endl;
			for (int i = 0; i < numOfNodes / 2; i++)
			{
				Ng_STL_AddEdge(stl_geom, nodes[2 * i], nodes[2 * i + 1]);
			}

			for (int i = 0; i < numOfNodes; i++)
				delete[] nodes[i];
			delete[] nodes;
		}
	}

	cout << "Initialise the STL Geometry structure...." << endl;
	ng_res = Ng_STL_InitSTLGeometryEx(stl_geom, 1);
	if (ng_res != NG_OK)
	{
		cout << "Error Initialising the STL Geometry....Aborting!!" << endl;
		return 1;
	}

	cout << "Start Edge Meshing...." << endl;
	ng_res = Ng_STL_MakeEdges(stl_geom, mesh, &mp);
	if (ng_res != NG_OK)
	{
		cout << "Error in Edge Meshing....Aborting!!" << endl;
		return 1;
	}

	cout << "Start Surface Meshing...." << endl;
	ng_res = Ng_STL_GenerateSurfaceMesh(stl_geom, mesh, &mp);
	if (ng_res != NG_OK)
	{
		cout << "Error in Surface Meshing....Aborting!!" << endl;
		return 1;
	}

	cout << "Surface Mesh successfully created....." << endl;
	cout << "Number of points = " << Ng_GetNP(mesh) << endl;
	cout << "Number of edges = " << Ng_GetNSeg_2D(mesh) << endl;

	Ng_SaveMesh(mesh, argv[3]);

	Ng_Exit();

	return 0;
}
bool StlEdgeMesh(int argc, char* argv[])
{
	// Define pointer to a new Netgen Mesh
	Ng_Mesh *mesh;

	// Define pointer to STL Geometry
	Ng_STL_Geometry *stl_geom;

	// Result of Netgen Operations
	Ng_Result ng_res;

	// Initialise the Netgen Core library
	Ng_Init();

	// Actually create the mesh structure
	mesh = Ng_NewMesh();

	int np, ne;

	// Read in the STL File
	stl_geom = Ng_STL_LoadGeometry(argv[2]);
	if (!stl_geom)
	{
		cout << "Error reading in STL File: " << argv[2] << endl;
		return 1;
	}
	cout << "Successfully loaded STL File: " << argv[2] << endl;

	// Set the Meshing Parameters to be used
	double deflection = 0;
	Ng_Meshing_Parameters mp;

	mp = ReadParameters(argv[4], deflection);
	mp.meshsize_filename = argc >= 6 ? argv[5] : NULL;

	// Add user edges if any
	if (argc >= 7)
	{
		int numOfNodes;
		double **nodes;
		ReadEdgeNodes(argv[6], numOfNodes, nodes);

		if (numOfNodes > 0)
		{
			cout << "Adding user edges...." << endl;
			for (int i = 0; i < numOfNodes / 2; i++)
			{
				Ng_STL_AddEdge(stl_geom, nodes[2 * i], nodes[2 * i + 1]);
			}

			for (int i = 0; i < numOfNodes; i++)
				delete[] nodes[i];
			delete[] nodes;
		}
	}

	cout << "Initialise the STL Geometry structure...." << endl;
	ng_res = Ng_STL_InitSTLGeometryEx(stl_geom, 1);
	if (ng_res != NG_OK)
	{
		cout << "Error Initialising the STL Geometry....Aborting!!" << endl;
		return 1;
	}

	cout << "Start Edge Meshing...." << endl;
	ng_res = Ng_STL_MakeEdges(stl_geom, mesh, &mp);
	if (ng_res != NG_OK)
	{
		cout << "Error in Edge Meshing....Aborting!!" << endl;
		return 1;
	}
	cout << "Number of edges = " << Ng_GetNSeg_2D(mesh) << endl;


	ng_res = Ng_STL_GenerateEdgeMesh(stl_geom, mesh, &mp);
	if (ng_res != NG_OK)
	{
		cout << "Error in Edge Meshing....Aborting!!" << endl;
		return 1;
	}

	cout << "Edge Mesh successfully created....." << endl;
	cout << "Number of points = " << Ng_GetNP(mesh) << endl;
	cout << "Number of edges = " << Ng_GetNSeg_2D(mesh) << endl;


	Ng_SaveMesh(mesh, argv[3]);

	Ng_Exit();

	return 0;
}
void ReadEdgeNodes(const char *fileName, int &numOfNodes, double** &nodes)
{
	Ng_Meshing_Parameters mp;

	ifstream File(fileName);
	string line;

	if (File)
	{
		int numOfEdges = -1;
		numOfNodes = -1;
		// check_overlapping_boundary
		getline(File, line);
		getline(File, line);
		numOfEdges = std::stoi(line);

		if (numOfEdges > 0)
		{
			numOfNodes = 2 * numOfEdges;
			nodes = new double*[numOfNodes];
			for (int i = 0; i < numOfNodes; ++i)
				nodes[i] = new double[3];

			for (int i = 0; i < numOfNodes; ++i)
			{
				getline(File, line);
				istringstream sin(line);

				sin >> nodes[i][0];
				sin >> nodes[i][1];
				sin >> nodes[i][2];
			}
		}

		File.close();
	}
}
// Occ Mesh
bool OccMesh(int argc, char* argv[])
{
	////Define pointer to OCC Geometry
	Ng_OCC_Geometry *occ_geom;

	Ng_Mesh *occ_mesh;

	TopTools_IndexedMapOfShape FMap;

	Ng_OCC_TopTools_IndexedMapOfShape *occ_fmap = (Ng_OCC_TopTools_IndexedMapOfShape*)&FMap;

	//// Result of Netgen Operations
	Ng_Result ng_res;

	int np, ne;

	//// Initialise the Netgen Core library
	Ng_Init();

	//// Read in the OCC File
	occ_geom = Ng_OCC_Load_BREP(argv[2]);
	if (!occ_geom)
	{
		cout << "Error reading in BREP File: " << argv[2] << endl;
		return 1;
	}
	cout << "Successfully loaded BREP File: " << argv[2] << endl;

	occ_mesh = Ng_NewMesh();

	ng_res = Ng_OCC_GetFMap(occ_geom, occ_fmap);

	cout << "ng_res = " << ng_res << endl;

	if (!FMap.Extent())
	{
		cout << "Error retrieving Face map...." << endl;
		return 1;
	}

	cout << "Successfully extracted the Face Map....:" << FMap.Extent() << endl;
	/*
	for (int i = 1; i <= FMap.Extent(); i++)
	{
	TopoDS_Face OCCface;
	OCCface = TopoDS::Face(FMap.FindKey(i));

	GProp_GProps faceProps;
	BRepGProp::SurfaceProperties(OCCface, faceProps);
	cout << "Index: " << i
	<< " :: Area: " << faceProps.Mass()
	<< " :: Hash: " << OCCface.HashCode(1e+6)
	<< endl;
	}*/


	// Set the Meshing Parameters to be used
	double deflection = 0;
	Ng_Meshing_Parameters mp;

	mp = ReadParameters(argv[4], deflection);
	mp.meshsize_filename = argv[5];

	/*mp.uselocalh = 1;
	mp.elementsperedge = 2.0;
	mp.elementspercurve = 2.0;
	mp.maxh = 10.0;
	mp.grading = 0.2;
	mp.closeedgeenable = 0;
	mp.closeedgefact = 1.0;
	mp.optsurfmeshenable = 1;*/


	// Wall-clock time of the meshing phases, reported at the end
	typedef std::chrono::steady_clock PhaseClock;
	PhaseClock::time_point phaseStart = PhaseClock::now();
	double timeLocalSize = 0, timeEdges = 0, timeSurface = 0, timeVolume = 0;
	auto phaseSeconds = [&phaseStart]() {
		PhaseClock::time_point now = PhaseClock::now();
		double seconds = std::chrono::duration<double>(now - phaseStart).count();
		phaseStart = now;
		return seconds;
	};

	cout << "Setting Local Mesh size....." << endl;
	cout << "OCC Mesh Pointer before call = " << occ_mesh << endl;
	Ng_OCC_SetLocalMeshSize(occ_geom, deflection, occ_mesh, &mp);
	cout << "Local Mesh size successfully set....." << endl;
	cout << "OCC Mesh Pointer after call = " << occ_mesh << endl;
	timeLocalSize = phaseSeconds();

	cout << "Creating Edge Mesh....." << endl;
	ng_res = Ng_OCC_GenerateEdgeMesh(occ_geom, occ_mesh, &mp);
	if (ng_res != NG_OK)
	{
		Ng_DeleteMesh(occ_mesh);
		cout << "Error creating Edge Mesh.... Aborting!!" << endl;
		return 1;
	}
	else
	{
		cout << "Edge Mesh successfully created....." << endl;
		cout << "Number of points = " << Ng_GetNP(occ_mesh) << endl;
	}
	timeEdges = phaseSeconds();

	cout << "Creating Surface Mesh....." << endl;

	ng_res = Ng_OCC_GenerateSurfaceMesh(occ_geom, occ_mesh, &mp);
	if (ng_res != NG_OK)
	{
		char fileNameWithPath[_MAX_FNAME];
		char full_path[_MAX_PATH];
		char drive[_MAX_DRIVE];
		char dir[_MAX_DIR];
		char fname[_MAX_FNAME];
		char ext[_MAX_EXT];
		_splitpath_s(argv[3], drive, dir, fname, ext);
		stringstream ss;
		ss << fname << "_error";
		_makepath_s(fileNameWithPath, drive, dir, ss.str().c_str(), ext);
		Ng_SaveMesh(occ_mesh, fileNameWithPath);
		//
		//Ng_DeleteMesh(occ_mesh);
		cout << "Error creating Surface Mesh..... Aborting!!" << endl;
		return 1;
	}
	else
	{
		cout << "Surface Mesh successfully created....." << endl;
		cout << "Number of points = " << Ng_GetNP(occ_mesh) << endl;
		cout << "Number of surface elements = " << Ng_GetNSE(occ_mesh) << endl;
	}
	timeSurface = phaseSeconds();

	cout << "Start Volume Meshing...." << endl;
	ng_res = Ng_GenerateVolumeMesh(occ_mesh, &mp);
	if (ng_res != NG_OK)
	{
		cout << "Error in Volume Meshing....Aborting!!" << endl;
		return 1;
	}
	timeVolume = phaseSeconds();

	if (mp.second_order > 0)
	{
		cout << endl << "Converting mesh to second order elements...." << endl << endl;
		Ng_OCC_Generate_SecondOrder(occ_geom, occ_mesh);
	}

	cout << "Meshing successfully completed....!!" << endl << endl;

	// volume mesh output
	np = Ng_GetNP(occ_mesh);
	cout << "Points: " << np << endl;

	ne = Ng_GetNE(occ_mesh);
	cout << "Elements: " << ne << endl;

	cout << "Phase times [s]: local size " << timeLocalSize
		<< ", edges " << timeEdges
		<< ", surface " << timeSurface
		<< ", volume " << timeVolume
		<< " (" << mp.parallel_meshing_threads << " thread(s) requested)" << endl;

	cout << "Saving Mesh as VOL file....." << endl;
	Ng_SaveMesh(occ_mesh, argv[3]);

	//nglib::Ng_Exit();

	return 0;
}
bool OccEdgeMesh(int argc, char* argv[])
{
	////Define pointer to OCC Geometry
	Ng_OCC_Geometry *occ_geom;

	Ng_Mesh *occ_mesh;

	TopTools_IndexedMapOfShape FMap;

	Ng_OCC_TopTools_IndexedMapOfShape *occ_fmap = (Ng_OCC_TopTools_IndexedMapOfShape*)&FMap;

	//// Result of Netgen Operations
	Ng_Result ng_res;

	int np, ne;

	//// Initialise the Netgen Core library
	Ng_Init();

	//// Read in the OCC File
	occ_geom = Ng_OCC_Load_BREP(argv[2]);
	if (!occ_geom)
	{
		cout << "Error reading in BREP File: " << argv[2] << endl;
		return 1;
	}
	cout << "Successfully loaded BREP File: " << argv[2] << endl;

	occ_mesh = Ng_NewMesh();

	ng_res = Ng_OCC_GetFMap(occ_geom, occ_fmap);

	cout << "ng_res = " << ng_res << endl;

	if (!FMap.Extent())
	{
		cout << "Error retrieving Face map...." << endl;
		return 1;
	}

	cout << "Successfully extracted the Face Map....:" << FMap.Extent() << endl;

	// Set the Meshing Parameters to be used
	double deflection = 0;
	Ng_Meshing_Parameters mp;

	mp = ReadParameters(argv[4], deflection);
	mp.meshsize_filename = argv[5];

	cout << "Setting Local Mesh size....." << endl;
	cout << "OCC Mesh Pointer before call = " << occ_mesh << endl;
	Ng_OCC_SetLocalMeshSize(occ_geom, deflection, occ_mesh, &mp);
	cout << "Local Mesh size successfully set....." << endl;
	cout << "OCC Mesh Pointer after call = " << occ_mesh << endl;

	cout << "Creating Edge Mesh....." << endl;
	ng_res = Ng_OCC_GenerateEdgeMesh(occ_geom, occ_mesh, &mp);
	if (ng_res != NG_OK)
	{
		Ng_DeleteMesh(occ_mesh);
		cout << "Error creating Edge Mesh..... Aborting!!" << endl;
		return 1;
	}

	cout << "Edge Mesh successfully created....." << endl;
	cout << "Number of points = " << Ng_GetNP(occ_mesh) << endl;

	Ng_SaveMesh(occ_mesh, argv[3]);

	Ng_Exit();

	return 0;
}
Ng_Meshing_Parameters ReadParameters(const char *fileName, double &deflection)
{
	//
	#pragma warning(disable:4996) // this prevents the warning for the copy string comman
	//
	Ng_Meshing_Parameters mp;
	//
	ifstream File(fileName);
	string line;
	//
	if (File)
	{
		// uselocalh
		getline(File, line);
		getline(File, line);
		mp.uselocalh = std::stoi(line);
		// maxh
		getline(File, line);
		getline(File, line);
		mp.maxh = std::stod(line);
		// minh
		getline(File, line);
		getline(File, line);
		mp.minh = std::stod(line);
		// fineness
		getline(File, line);
		getline(File, line);
		mp.fineness = std::stod(line);
		// grading
		getline(File, line);
		getline(File, line);
		mp.grading = std::stod(line);
		// elementsperedge
		getline(File, line);
		getline(File, line);
		mp.elementsperedge = std::stod(line);
		// elementspercurve
		getline(File, line);
		getline(File, line);
		mp.elementspercurve = std::stod(line);
		// closeedgeenable
		getline(File, line);
		getline(File, line);
		mp.closeedgeenable = std::stoi(line);
		// closeedgefact
		getline(File, line);
		getline(File, line);
		mp.closeedgefact = std::stod(line);
		// minedgelenenable
		getline(File, line);
		getline(File, line);
		mp.minedgelenenable = std::stoi(line);
		// minedgelen
		getline(File, line);
		getline(File, line);
		mp.minedgelen = std::stoi(line);
		// second_order
		getline(File, line);
		getline(File, line);
		mp.second_order = std::stoi(line);
		// quad_dominated
		getline(File, line);
		getline(File, line);
		mp.quad_dominated = std::stoi(line);
		// meshsize_filename
		getline(File, line);
		getline(File, line);
		int n = line.length();
		// declaring character array 
		mp.meshsize_filename = new char[n + 1];
		// copying the contents of the 
		// string to char array 
		strcpy(mp.meshsize_filename, line.c_str());
		// optsurfmeshenable
		getline(File, line);
		getline(File, line);
		mp.optsurfmeshenable = std::stoi(line);
		// optvolmeshenable
		getline(File, line);
		getline(File, line);
		mp.optvolmeshenable = std::stoi(line);
		// optsteps_3d
		getline(File, line);
		getline(File, line);
		mp.optsteps_3d = std::stoi(line);
		// optsteps_2d
		getline(File, line);
		getline(File, line);
		mp.optsteps_2d = std::stoi(line);
		// invert_tets
		getline(File, line);
		getline(File, line);
		mp.invert_tets = std::stoi(line);
		// invert_trigs
		getline(File, line);
		getline(File, line);
		mp.invert_trigs = std::stoi(line);
		// check_overlap
		getline(File, line);
		getline(File, line);
		mp.check_overlap = std::stoi(line);
		// check_overlapping_boundary
		getline(File, line);
		getline(File, line);
		mp.check_overlapping_boundary = std::stoi(line);
		// Optional OCC visualization deflection used by BREP meshing.
		if (getline(File, line) && getline(File, line) && !line.empty())
			deflection = std::stod(line);
		// Optional positive parallel meshing worker count: 1 = serial.
		if (getline(File, line) && getline(File, line) && !line.empty())
		{
			int threadCount = std::stoi(line);
			if (threadCount < 1)
				throw std::invalid_argument("parallel_meshing_threads must be larger than 0.");
			mp.parallel_meshing_threads = threadCount;
		}
		//
		File.close();
	}
	//
	return mp;
}
// The rest
bool Test(int argc, char* argv[])
{
	return 0;
}
char** ReadDataFile(int &argc, char* argv[])
{
	//
	#pragma warning(disable:4996) // this prevents the warning for the copy string comman
	//
	char* fileName = argv[1];
	ifstream File(fileName);
	string line;
	int lineLen;
	char** parameters = new char*[0];
	//
	char* command;
	//
	if (File)
	{
		// Read command
		getline(File, line);
		getline(File, line);
		lineLen = line.length();
		command = new char[lineLen + 1];
		strcpy(command, line.c_str());

		if (strcmp(command, "STL_MESH") == 0) argc = 6;
		else if (strcmp(command, "STL_SURFACE_MESH") == 0) argc = 6;
		else if (strcmp(command, "STL_EDGE_MESH") == 0) argc = 6;
		else if (strcmp(command, "STEP_ASSEMBLY_SPLIT") == 0) argc = 4;
		else if (strcmp(command, "IGES_ASSEMBLY_SPLIT") == 0) argc = 4;
		else if (strcmp(command, "BREP_ASSEMBLY_SPLIT") == 0) argc = 4;
		else if (strcmp(command, "BREP_VISUALIZATION") == 0) argc = 5;
		else if (strcmp(command, "BREP_MESH") == 0) argc = 5;
		else if (strcmp(command, "BREP_NODE_MESH") == 0) argc = 5;
		//
		parameters = new char*[argc];
		parameters[0] = argv[0];
		parameters[1] = command;
		//
		for (int i = 2; i < argc; i++)
		{
			// Read data
			line = string("");
			getline(File, line);
			getline(File, line);
			lineLen = line.length();
			parameters[i] = new char[lineLen + 1];
			strcpy(parameters[i], line.c_str());
		}
		File.close();
	}
	return parameters;
}

