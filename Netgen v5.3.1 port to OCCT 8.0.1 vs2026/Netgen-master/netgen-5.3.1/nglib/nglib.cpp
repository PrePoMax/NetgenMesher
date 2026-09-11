/**************************************************************************/
/* File:   nglib.cpp                                                      */
/* Author: Joachim Schoeberl                                              */
/* Date:   7. May. 2000                                                   */
/**************************************************************************/

/*

  Interface to the netgen meshing kernel

*/
#include <mystdlib.h>
#include <TColgp_Array1OfPnt.hxx>
#include <TopTools_IndexedMapOfShape.hxx>
#include <myadt.hpp>

#include <linalg.hpp>
#include <csg.hpp>
#include <stlgeom.hpp>
#include <geometry2d.hpp>
#include <meshing.hpp>
#include <../visualization/soldata.hpp>

#include <stdio.h>

#include <TColStd_Array1OfInteger.hxx>
#include <Poly_PolygonOnTriangulation.hxx>
#include <Poly_Triangulation.hxx>
#include <ShapeAnalysis_ShapeContents.hxx>
#include <iostream>
#include <OSD_Protection.hxx>
#include <OSD_File.hxx>
#include <vector> 

#ifdef OCCGEOMETRY
#include <occgeom.hpp>
#endif

#include <nginterface.h>

// By Maxx
#include <list>
#include <BOPAlgo_Builder.hxx>
#include <BRep_Builder.hxx>
#include <BRepExtrema_DistShapeShape.hxx>
#include <BRep_TEdge.hxx>
#include <ShapeUpgrade_ShapeDivideClosed.hxx>
#include <GProp.hxx>
#include <BRepCheck_Analyzer.hxx>
#include <BRepLib.hxx>
#include <Precision.hxx>
#include <typeinfo>
#include <map>

namespace netgen {
	extern void MeshFromSpline2D(SplineGeometry2d & geometry,
		Mesh *& mesh,
		MeshingParameters & mp);
}



#ifdef PARALLEL
#include <mpi.h>

namespace netgen
{
	// int id = 0, ntasks = 1;
	MPI_Comm mesh_comm;
}
#endif


namespace netgen
{
	int id = 0, ntasks = 1;
}



/*
// should not be needed (occ currently requires it)
namespace netgen {
#include "../libsrc/visualization/vispar.hpp"
  VisualizationParameters vispar;
  VisualizationParameters :: VisualizationParameters() { ; }
}
*/


namespace nglib {
#include "nglib.h"
}

using namespace netgen;

// constants and types:

namespace nglib
{
	// initialize, deconstruct Netgen library:
	DLL_HEADER void Ng_Init()
	{
		mycout = &cout;
		myerr = &cerr;
		// netgen::testout->SetOutStream (new ofstream ("test.out"));
		testout = new ofstream("test.out");
	}




	// Clean-up functions before ending usage of nglib
	DLL_HEADER void Ng_Exit()
	{
		;
	}




	// Create a new netgen mesh object
	DLL_HEADER Ng_Mesh * Ng_NewMesh()
	{
		Mesh * mesh = new Mesh;
		mesh->AddFaceDescriptor(FaceDescriptor(1, 1, 0, 1));
		return (Ng_Mesh*)(void*)mesh;
	}




	// Delete an existing netgen mesh object
	DLL_HEADER void Ng_DeleteMesh(Ng_Mesh * mesh)
	{
		if (mesh != NULL)
		{
			// Delete the Mesh structures
			((Mesh*)mesh)->DeleteMesh();

			// Now delete the Mesh class itself
			delete (Mesh*)mesh;

			// Set the Ng_Mesh pointer to NULL
			mesh = NULL;
		}
	}




	// Save a netgen mesh in the native VOL format 
	DLL_HEADER void Ng_SaveMesh(Ng_Mesh * mesh, const char* filename)
	{
		((Mesh*)mesh)->Save(filename);
	}




	// Load a netgen native VOL mesh from a given file
	DLL_HEADER Ng_Mesh * Ng_LoadMesh(const char* filename)
	{
		Mesh * mesh = new Mesh;
		mesh->Load(filename);
		return ((Ng_Mesh*)mesh);
	}




	// Merge another mesh file into the currently loaded one
	DLL_HEADER Ng_Result Ng_MergeMesh(Ng_Mesh* mesh, const char* filename)
	{
		Ng_Result status = NG_OK;

		ifstream infile(filename);
		Mesh * m = (Mesh*)mesh;

		if (!infile.good())
		{
			status = NG_FILE_NOT_FOUND;
		}

		if (!m)
		{
			status = NG_ERROR;
		}

		if (status == NG_OK)
		{
			const int num_pts = m->GetNP();
			const int face_offset = m->GetNFD();

			m->Merge(infile, face_offset);

			if (m->GetNP() > num_pts)
			{
				status = NG_OK;
			}
			else
			{
				status = NG_ERROR;
			}
		}

		return status;
	}




	// Merge another mesh file into the currently loaded one
	DLL_HEADER Ng_Result Ng_MergeMesh(Ng_Mesh* mesh1, Ng_Mesh* mesh2)
	{
		return NG_ERROR;
	}




	// Manually add a point to an existing mesh object
	DLL_HEADER void Ng_AddPoint(Ng_Mesh * mesh, double * x)
	{
		Mesh * m = (Mesh*)mesh;
		m->AddPoint(Point3d(x[0], x[1], x[2]));
	}




	// Manually add a surface element of a given type to an existing mesh object
	DLL_HEADER void Ng_AddSurfaceElement(Ng_Mesh * mesh, Ng_Surface_Element_Type et,
		int * pi)
	{
		Mesh * m = (Mesh*)mesh;
		Element2d el(3);
		el.SetIndex(1);
		el.PNum(1) = pi[0];
		el.PNum(2) = pi[1];
		el.PNum(3) = pi[2];
		m->AddSurfaceElement(el);
	}




	// Manually add a volume element of a given type to an existing mesh object
	DLL_HEADER void Ng_AddVolumeElement(Ng_Mesh * mesh, Ng_Volume_Element_Type et,
		int * pi)
	{
		Mesh * m = (Mesh*)mesh;
		Element el(4);
		el.SetIndex(1);
		el.PNum(1) = pi[0];
		el.PNum(2) = pi[1];
		el.PNum(3) = pi[2];
		el.PNum(4) = pi[3];
		m->AddVolumeElement(el);
	}




	// Obtain the number of points in the mesh
	DLL_HEADER int Ng_GetNP(Ng_Mesh * mesh)
	{
		return ((Mesh*)mesh)->GetNP();
	}




	// Obtain the number of surface elements in the mesh
	DLL_HEADER int Ng_GetNSE(Ng_Mesh * mesh)
	{
		return ((Mesh*)mesh)->GetNSE();
	}




	// Obtain the number of volume elements in the mesh
	DLL_HEADER int Ng_GetNE(Ng_Mesh * mesh)
	{
		return ((Mesh*)mesh)->GetNE();
	}




	//  Return point coordinates of a given point index in the mesh
	DLL_HEADER void Ng_GetPoint(Ng_Mesh * mesh, int num, double * x)
	{
		const Point3d & p = ((Mesh*)mesh)->Point(num);
		x[0] = p.X();
		x[1] = p.Y();
		x[2] = p.Z();
	}




	// Return the surface element at a given index "pi"
	DLL_HEADER Ng_Surface_Element_Type
		Ng_GetSurfaceElement(Ng_Mesh * mesh, int num, int * pi)
	{
		const Element2d & el = ((Mesh*)mesh)->SurfaceElement(num);
		for (int i = 1; i <= el.GetNP(); i++)
			pi[i - 1] = el.PNum(i);
		Ng_Surface_Element_Type et;
		switch (el.GetNP())
		{
		case 3: et = NG_TRIG; break;
		case 4: et = NG_QUAD; break;
		case 6:
			switch (el.GetNV())
			{
			case 3: et = NG_TRIG6; break;
			case 4: et = NG_QUAD6; break;
			default:
				et = NG_TRIG6; break;
			}
			break;
		case 8: et = NG_QUAD8; break;
		default:
			et = NG_TRIG; break; // for the compiler
		}
		return et;
	}




	// Return the volume element at a given index "pi"
	DLL_HEADER Ng_Volume_Element_Type
		Ng_GetVolumeElement(Ng_Mesh * mesh, int num, int * pi)
	{
		const Element & el = ((Mesh*)mesh)->VolumeElement(num);
		for (int i = 1; i <= el.GetNP(); i++)
			pi[i - 1] = el.PNum(i);
		Ng_Volume_Element_Type et;
		switch (el.GetNP())
		{
		case 4: et = NG_TET; break;
		case 5: et = NG_PYRAMID; break;
		case 6: et = NG_PRISM; break;
		case 10: et = NG_TET10; break;
		default:
			et = NG_TET; break; // for the compiler
		}
		return et;
	}




	// Set a global limit on the maximum mesh size allowed
	DLL_HEADER void Ng_RestrictMeshSizeGlobal(Ng_Mesh * mesh, double h)
	{
		((Mesh*)mesh)->SetGlobalH(h);
	}




	// Set a local limit on the maximum mesh size allowed around the given point
	DLL_HEADER void Ng_RestrictMeshSizePoint(Ng_Mesh * mesh, double * p, double h)
	{
		((Mesh*)mesh)->RestrictLocalH(Point3d(p[0], p[1], p[2]), h);
	}




	// Set a local limit on the maximum mesh size allowed within a given box region
	DLL_HEADER void Ng_RestrictMeshSizeBox(Ng_Mesh * mesh, double * pmin, double * pmax, double h)
	{
		for (double x = pmin[0]; x < pmax[0]; x += h)
			for (double y = pmin[1]; y < pmax[1]; y += h)
				for (double z = pmin[2]; z < pmax[2]; z += h)
					((Mesh*)mesh)->RestrictLocalH(Point3d(x, y, z), h);
	}




	// Generates volume mesh from an existing surface mesh
	DLL_HEADER Ng_Result Ng_GenerateVolumeMesh(Ng_Mesh * mesh, Ng_Meshing_Parameters * mp)
	{
		Mesh * m = (Mesh*)mesh;

		// Philippose - 30/08/2009
		// Do not locally re-define "mparam" here... "mparam" is a global 
		// object 
		//MeshingParameters mparam;
		mp->Transfer_Parameters();

		m->CalcLocalH(mparam.grading);

		// Serial: MeshVolume + RemoveIllegalElements + OptimizeVolume.
		// With mparam.parallel_meshing_threads != 1 the independent subdomains
		// are meshed and optimized in parallel and committed in domain order.
		MeshVolumeAndOptimize(mparam, *m);

		//GenerateBoundaryLayer2(*m, mparam);

		return NG_OK;
	}




	/* ------------------ 2D Meshing Functions ------------------------- */
	DLL_HEADER void Ng_AddPoint_2D(Ng_Mesh * mesh, double * x)
	{
		Mesh * m = (Mesh*)mesh;

		m->AddPoint(Point3d(x[0], x[1], 0));
	}




	DLL_HEADER void Ng_AddBoundarySeg_2D(Ng_Mesh * mesh, int pi1, int pi2)
	{
		Mesh * m = (Mesh*)mesh;

		Segment seg;
		seg[0] = pi1;
		seg[1] = pi2;
		m->AddSegment(seg);
	}




	DLL_HEADER int Ng_GetNP_2D(Ng_Mesh * mesh)
	{
		Mesh * m = (Mesh*)mesh;
		return m->GetNP();
	}




	DLL_HEADER int Ng_GetNE_2D(Ng_Mesh * mesh)
	{
		Mesh * m = (Mesh*)mesh;
		return m->GetNSE();
	}




	DLL_HEADER int Ng_GetNSeg_2D(Ng_Mesh * mesh)
	{
		Mesh * m = (Mesh*)mesh;
		return m->GetNSeg();
	}




	DLL_HEADER void Ng_GetPoint_2D(Ng_Mesh * mesh, int num, double * x)
	{
		Mesh * m = (Mesh*)mesh;

		Point<3> & p = m->Point(num);
		x[0] = p(0);
		x[1] = p(1);
	}




	DLL_HEADER Ng_Surface_Element_Type
		Ng_GetElement_2D(Ng_Mesh * mesh, int num, int * pi, int * matnum)
	{
		const Element2d & el = ((Mesh*)mesh)->SurfaceElement(num);
		for (int i = 1; i <= el.GetNP(); i++)
			pi[i - 1] = el.PNum(i);

		Ng_Surface_Element_Type et;
		switch (el.GetNP())
		{
		case 3: et = NG_TRIG; break;
		case 4: et = NG_QUAD; break;
		case 6:
			switch (el.GetNV())
			{
			case 3: et = NG_TRIG6; break;
			case 4: et = NG_QUAD6; break;
			default:
				et = NG_TRIG6; break;
			}
			break;
		case 8: et = NG_QUAD8; break;
		default:
			et = NG_TRIG; break; // for the compiler
		}

		if (matnum)
			*matnum = el.GetIndex();

		return et;
	}




	DLL_HEADER void Ng_GetSegment_2D(Ng_Mesh * mesh, int num, int * pi, int * matnum)
	{
		const Segment & seg = ((Mesh*)mesh)->LineSegment(num);
		pi[0] = seg[0];
		pi[1] = seg[1];

		if (matnum)
			*matnum = seg.edgenr;
	}




	DLL_HEADER Ng_Geometry_2D * Ng_LoadGeometry_2D(const char * filename)
	{
		SplineGeometry2d * geom = new SplineGeometry2d();
		geom->Load(filename);
		return (Ng_Geometry_2D *)geom;
	}




	DLL_HEADER Ng_Result Ng_GenerateMesh_2D(Ng_Geometry_2D * geom,
		Ng_Mesh ** mesh,
		Ng_Meshing_Parameters * mp)
	{
		// use global variable mparam
		//  MeshingParameters mparam;  
		mp->Transfer_Parameters();

		Mesh * m;
		MeshFromSpline2D(*(SplineGeometry2d*)geom, m, mparam);

		cout << m->GetNSE() << " elements, " << m->GetNP() << " points" << endl;

		*mesh = (Ng_Mesh*)m;
		return NG_OK;
	}




	DLL_HEADER void Ng_HP_Refinement(Ng_Geometry_2D * geom,
		Ng_Mesh * mesh,
		int levels)
	{
		Refinement2d ref(*(SplineGeometry2d*)geom);
		HPRefinement(*(Mesh*)mesh, &ref, levels);
	}




	DLL_HEADER void Ng_HP_Refinement(Ng_Geometry_2D * geom,
		Ng_Mesh * mesh,
		int levels, double parameter)
	{
		Refinement2d ref(*(SplineGeometry2d*)geom);
		HPRefinement(*(Mesh*)mesh, &ref, levels, parameter);
	}




	Array<STLReadTriangle> readtrias; //only before initstlgeometry
	Array<Point<3> > readedges; //only before init stlgeometry

	// loads geometry from STL file
	DLL_HEADER Ng_STL_Geometry * Ng_STL_LoadGeometry(const char * filename, int binary)
	{
		int i;
		STLGeometry geom;
		STLGeometry* geo;
		ifstream ist(filename);

		if (binary)
		{
			geo = geom.LoadBinary(ist);
		}
		else
		{
			geo = geom.Load(ist);
		}

		readtrias.SetSize(0);
		readedges.SetSize(0);

		Point3d p;
		Vec3d normal;
		double p1[3];
		double p2[3];
		double p3[3];
		double n[3];

		Ng_STL_Geometry * geo2 = Ng_STL_NewGeometry();

		for (i = 1; i <= geo->GetNT(); i++)
		{
			const STLTriangle& t = geo->GetTriangle(i);
			p = geo->GetPoint(t.PNum(1));
			p1[0] = p.X(); p1[1] = p.Y(); p1[2] = p.Z();
			p = geo->GetPoint(t.PNum(2));
			p2[0] = p.X(); p2[1] = p.Y(); p2[2] = p.Z();
			p = geo->GetPoint(t.PNum(3));
			p3[0] = p.X(); p3[1] = p.Y(); p3[2] = p.Z();
			normal = t.Normal();
			n[0] = normal.X(); n[1] = normal.Y(); n[2] = normal.Z();

			Ng_STL_AddTriangle(geo2, p1, p2, p3, n);
		}

		return geo2;
	}




	// generate new STL Geometry
	DLL_HEADER Ng_STL_Geometry * Ng_STL_NewGeometry()
	{
		return (Ng_STL_Geometry*)(void*)new STLGeometry;
	}




	// after adding triangles (and edges) initialize
	DLL_HEADER Ng_Result Ng_STL_InitSTLGeometry(Ng_STL_Geometry * geom)
	{
		return Ng_STL_InitSTLGeometryEx(geom, 0);
	}

	DLL_HEADER Ng_Result Ng_STL_InitSTLGeometryEx(Ng_STL_Geometry * geom,
		int allow_open_boundaries)
	{
		STLGeometry* geo = (STLGeometry*)geom;
		geo->SetAllowOpenBoundaries(allow_open_boundaries != 0);
		geo->InitSTLGeometry(readtrias);
		readtrias.SetSize(0);

		if (readedges.Size() != 0)
		{
			/*
			for (int i = 1; i <= readedges.Size(); i+=2)
			{
			cout << "e(" << readedges.Get(i) << "," << readedges.Get(i+1) << ")" << endl;
			}
			*/
			geo->AddEdges(readedges);
		}

		if (geo->GetStatus() == STLTopology::STL_GOOD || geo->GetStatus() == STLTopology::STL_WARNING) return NG_OK;
		return NG_SURFACE_INPUT_ERROR;
	}




	// automatically create edges:
	DLL_HEADER Ng_Result Ng_STL_MakeEdges(Ng_STL_Geometry * geom,
		Ng_Mesh* mesh,
		Ng_Meshing_Parameters * mp)
	{
		STLGeometry* stlgeometry = (STLGeometry*)geom;
		Mesh* me = (Mesh*)mesh;

		// Philippose - 27/07/2009
		// Do not locally re-define "mparam" here... "mparam" is a global 
		// object 
		//MeshingParameters mparam;
		mp->Transfer_Parameters();

		me->SetGlobalH(mparam.maxh);
		me->SetLocalH(stlgeometry->GetBoundingBox().PMin() - Vec3d(10, 10, 10),
			stlgeometry->GetBoundingBox().PMax() + Vec3d(10, 10, 10),
			0.3);

		me->LoadLocalMeshSize(mp->meshsize_filename);
		/*
		if (mp->meshsize_filename)
		{
		ifstream infile (mp->meshsize_filename);
		if (!infile.good()) return NG_FILE_NOT_FOUND;
		me -> LoadLocalMeshSize (infile);
		}
		*/

		STLMeshing(*stlgeometry, *me);

		stlgeometry->edgesfound = 1;
		stlgeometry->surfacemeshed = 0;
		stlgeometry->surfaceoptimized = 0;
		stlgeometry->volumemeshed = 0;

		return NG_OK;
	}



	// automatically generates edge mesh:
	DLL_HEADER Ng_Result Ng_STL_GenerateEdgeMesh(Ng_STL_Geometry * geom,
		Ng_Mesh* mesh,
		Ng_Meshing_Parameters * mp)
	{
		Ng_STL_MakeEdges(geom, mesh, mp);

		STLGeometry* stlgeometry = (STLGeometry*)geom;
		Mesh* me = (Mesh*)mesh;

		stlgeometry->PrepareSurfaceMeshing();

		if (me->GetNSeg() == 0) netgen::STLFindEdges(*stlgeometry, *me);

		return NG_OK;
	}




	// generates mesh, empty mesh be already created.
	DLL_HEADER Ng_Result Ng_STL_GenerateSurfaceMesh(Ng_STL_Geometry * geom,
		Ng_Mesh* mesh,
		Ng_Meshing_Parameters * mp)
	{
		STLGeometry* stlgeometry = (STLGeometry*)geom;
		Mesh* me = (Mesh*)mesh;

		// Philippose - 27/07/2009
		// Do not locally re-define "mparam" here... "mparam" is a global 
		// object
		//MeshingParameters mparam;
		mp->Transfer_Parameters();


		/*
		me -> SetGlobalH (mparam.maxh);
		me -> SetLocalH (stlgeometry->GetBoundingBox().PMin() - Vec3d(10, 10, 10),
		stlgeometry->GetBoundingBox().PMax() + Vec3d(10, 10, 10),
		0.3);
		*/
		/*
		STLMeshing (*stlgeometry, *me);

		stlgeometry->edgesfound = 1;
		stlgeometry->surfacemeshed = 0;
		stlgeometry->surfaceoptimized = 0;
		stlgeometry->volumemeshed = 0;
		*/
		int retval = STLSurfaceMeshing(*stlgeometry, *me);
		if (retval == MESHING3_OK)
		{
			(*mycout) << "Success !!!!" << endl;
			stlgeometry->surfacemeshed = 1;
			stlgeometry->surfaceoptimized = 0;
			stlgeometry->volumemeshed = 0;
		}
		else if (retval == MESHING3_OUTERSTEPSEXCEEDED)
		{
			(*mycout) << "ERROR: Give up because of too many trials. Meshing aborted!" << endl;
		}
		else if (retval == MESHING3_TERMINATE)
		{
			(*mycout) << "Meshing Stopped!" << endl;
		}
		else
		{
			(*mycout) << "ERROR: Surface meshing not successful. Meshing aborted!" << endl;
		}


		if (retval != MESHING3_OK)
			return NG_SURFACE_FAILURE;

		STLSurfaceOptimization(*stlgeometry, *me, mparam);

		return NG_OK;
	}




	// fills STL Geometry
	// positive orientation
	// normal vector may be null-pointer
	DLL_HEADER void Ng_STL_AddTriangle(Ng_STL_Geometry * geom,
		double * p1, double * p2, double * p3,
		double * nv)
	{
		Point<3> apts[3];
		apts[0] = Point<3>(p1[0], p1[1], p1[2]);
		apts[1] = Point<3>(p2[0], p2[1], p2[2]);
		apts[2] = Point<3>(p3[0], p3[1], p3[2]);

		Vec<3> n;
		if (!nv)
			n = Cross(apts[0] - apts[1], apts[0] - apts[2]);
		else
			n = Vec<3>(nv[0], nv[1], nv[2]);

		readtrias.Append(STLReadTriangle(apts, n));
	}

	// add (optional) edges:
	DLL_HEADER void Ng_STL_AddEdge(Ng_STL_Geometry * geom,
		double * p1, double * p2)
	{
		readedges.Append(Point3d(p1[0], p1[1], p1[2]));
		readedges.Append(Point3d(p2[0], p2[1], p2[2]));
	}




#ifdef OCCGEOMETRY
	// --------------------- OCC Geometry / Meshing Utility Functions -------------------
	inline const char* SurfaceTypeToString(GeomAbs_SurfaceType surfaceType)
	{
		switch (surfaceType)
		{
			case GeomAbs_Plane:   return "Plane";
			case GeomAbs_Cylinder:   return "Cylinder";
			case GeomAbs_Cone: return "Cone";
			case GeomAbs_Sphere: return "Sphere";
			case GeomAbs_Torus: return "Torus";
			case GeomAbs_BezierSurface: return "BezierSurface";
			case GeomAbs_BSplineSurface: return "BSplineSurface";
			case GeomAbs_SurfaceOfRevolution: return "SurfaceOfRevolution";
			case GeomAbs_SurfaceOfExtrusion: return "SurfaceOfExtrusion";
			case GeomAbs_OffsetSurface: return "OffsetSurface";
			case GeomAbs_OtherSurface: return "OtherSurface";
			default: return "Unknown";
		}
	}
	inline const char* CurveTypeToString(GeomAbs_CurveType curveType)
	{
		switch (curveType)
		{
			case GeomAbs_Line:   return "Line";
			case GeomAbs_Circle:   return "Circle";
			case GeomAbs_Ellipse: return "Ellipse";
			case GeomAbs_Hyperbola: return "Hyperbola";
			case GeomAbs_Parabola: return "Parabola";
			case GeomAbs_BezierCurve: return "BezierCurve";
			case GeomAbs_BSplineCurve: return "BSplineCurve";		
			case GeomAbs_OffsetCurve: return "OffsetCurve";
			case GeomAbs_OtherCurve: return "OtherCurve";
			default: return "Unknown";
		}
	}
	void ExportItemVisualization(OCCGeometry * occgeom, TopAbs_ShapeEnum TopAbs_Shape, ofstream &outfile,
	                             int &numOfEdges, int &numOfFallbackEdges, int &numOfFacesWithoutTriangulation)
	{
		TopLoc_Location aLocation;
		Standard_Integer nId;
		gp_Pnt aPnt1;
		int facenr;
		int edgenr;
		Standard_Integer N1;
		Standard_Integer N2;
		Standard_Integer N3;
		//
		TopAbs_ShapeEnum ToAvoid;
		if (TopAbs_Shape == TopAbs_SOLID) ToAvoid = TopAbs_SHAPE;
		else if (TopAbs_Shape == TopAbs_SHELL) ToAvoid = TopAbs_SOLID;
		else if (TopAbs_Shape == TopAbs_FACE) ToAvoid = TopAbs_SHELL;
		//
		// The vertex map covers the whole shape and does not depend on the item, so
		// it is built once here instead of once per solid/shell. The per-item vertex
		// block below is still written exactly as before - the file format is unchanged.
		TopTools_IndexedMapOfShape map;
		TopExp::MapShapes(occgeom->shape, TopAbs_VERTEX, map);
		//
		int itemnr = 0;
		for (TopExp_Explorer exp0(occgeom->shape, TopAbs_Shape, ToAvoid); exp0.More(); exp0.Next())
		{
			itemnr++;
			outfile << "**********" << endl;
			if (TopAbs_Shape == TopAbs_SOLID) outfile << "Solid number: ";
			else if (TopAbs_Shape == TopAbs_SHELL) outfile << "Shell number: ";
			else if (TopAbs_Shape == TopAbs_FACE) outfile << "Free face number: ";
			//
			outfile << itemnr << endl;
			// Vertices	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
			int vertexnr;
			gp_Pnt point;
			TopoDS_Vertex vertex;
			int nVertices = map.Extent();
			//
			outfile << "**********" << endl;
			outfile << "Number of vertices: " << nVertices << " (Id, X, Y, Z)" << endl;
			//
			for (int i = 1; i <= map.Extent(); i++)
			{
				vertex = TopoDS::Vertex(map(i));
				vertexnr = occgeom->vmap.FindIndex(vertex);
				//
				point = BRep_Tool::Pnt(vertex);
				//
				outfile << vertexnr << " " << to_string(point.X()) << " " << to_string(point.Y()) << " " << to_string(point.Z()) << "\n";
			}
			///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
			for (TopExp_Explorer exp1(exp0.Current(), TopAbs_FACE); exp1.More(); exp1.Next())
			{
				TopoDS_Face face = TopoDS::Face(exp1.Current());
				facenr = occgeom->fmap.FindIndex(face);
				//
				BRepAdaptor_Surface faceElementAdaptor(face, Standard_True);
				GeomAbs_SurfaceType surfaceType = faceElementAdaptor.GetType();
				//
				TopAbs_Orientation orientation = face.Orientation();
				//
				outfile << "**********" << endl;
				outfile << "Face number: " << facenr << endl;
				outfile << "Face orientation: " << orientation << endl;
				outfile << "Face type: " << SurfaceTypeToString(surfaceType) << endl;
				//
				Handle(Poly_Triangulation) aTr = BRep_Tool::Triangulation(face, aLocation);
				// If this triangulation is not NULL
				if (!aTr.IsNull())
				{
					// Takes the array of nodes for this triangulation:
					// const TColgp_Array1OfPnt& aNodes = aTr->Node();
					// Create array of node points in absolute coordinate system
					TColgp_Array1OfPnt aPoints(1, aTr->NbNodes());
					for (Standard_Integer i = 1; i <= aTr->NbNodes(); i++)
					{
						aPoints(i) = aTr->Node(i).Transformed(aLocation);
					}
					// Nodes
					outfile << "**********" << endl;
					outfile << "Number of nodes: " << aPoints.Length() << " (Id, X, Y, Z)" << endl;
					//
					for (Standard_Integer i = 1; i <= aPoints.Length(); i++)
					{
						outfile << i << " " << to_string(aPoints(i).X()) << " " << to_string(aPoints(i).Y()) << " " << to_string(aPoints(i).Z()) << "\n";
					}
					// Triangles
					const Poly_Array1OfTriangle& triangles = aTr->Triangles();
					outfile << "**********" << endl;
					outfile << "Number of triangles: " << aTr->NbTriangles() << " (Id, N1, N2, N3)" << endl;
					for (Standard_Integer i = 1; i <= triangles.Length(); i++)
					{
						triangles(i).Get(N1, N2, N3);
						outfile << i << " " << N1 << " " << N2 << " " << N3 << "\n";
					}
					// Edges
					TopTools_IndexedMapOfShape edges;
					TopExp::MapShapes(exp1.Current(), TopAbs_EDGE, edges);
					outfile << "**********" << endl;
					outfile << "Number of edges: " << edges.Extent() << " (Type, Id, N1, N2, N3 ...)" << endl;
					//
					bool searchNodeByNode;
					GProp_GProps system;
					double eps = 1E-9;
					TopoDS_Vertex v1;
					TopoDS_Vertex v2;
					BRepExtrema_DistShapeShape extrema;
					extrema.SetDeflection(eps);
					//
					for (TopExp_Explorer exp2(exp1.Current(), TopAbs_EDGE); exp2.More(); exp2.Next())
					{
						TopoDS_Edge edge = TopoDS::Edge(exp2.Current());
						edgenr = occgeom->emap.FindIndex(edge);
						numOfEdges++;
						//
						BRepAdaptor_Curve edgeElementAdaptor(edge);
						GeomAbs_CurveType edgeType = edgeElementAdaptor.GetType();
						//
						// If this edge is not NULL
						if (!edge.IsNull())
						{
							// Takes the polygon associated to the edge aEdge.
							// The (triangulation, location) pair identifies the stored
							// representation, so the location must be the one the face
							// triangulation was retrieved with - not the edge's own.
							Handle(Poly_PolygonOnTriangulation) aPol = BRep_Tool::PolygonOnTriangulation(edge, aTr, aLocation);
							// If this polygon is not NULL
							searchNodeByNode = aPol.IsNull();
							if (!searchNodeByNode)
							{
								// The polygon's node indices are, by construction, indices into
								// this face's triangulation and lie on this edge, so they are
								// written out directly - no per-node verification needed.
								const TColStd_Array1OfInteger& aNodesOfPol = aPol->Nodes();
								{
									// Edge type and id
									outfile << CurveTypeToString(edgeType) << " " << edgenr;	// edge type and ID - 1st time
									// Takes the array of nodes for this polygon (indexes in the array of nodes for triangulation of theFace)
									for (Standard_Integer i = 1; i <= aNodesOfPol.Length(); i++)
									{
										nId = aNodesOfPol.Value(i);
										outfile << " " << nId;
									}
									outfile << endl;
								}
							}
							// The polygon is NULL
							if (searchNodeByNode)
							{
								// Kept for geometry that genuinely has no polygon on this
								// face's triangulation. It is orders of magnitude more
								// expensive than the path above, so it is counted and
								// reported: a non-zero count means real degenerate geometry,
								// not a lookup that silently failed.
								numOfFallbackEdges++;
								// Find points on triangulation
								BRepGProp::LinearProperties(edge, system);
								if (system.Mass() < eps) continue;
								//
								int startId = -1;
								int endId = -1;
								int minId = -1;
								double min = 1E100;
								//
								v1 = TopExp::FirstVertex(edge);
								extrema.LoadS1(v1);
								for (Standard_Integer i = 1; i <= aPoints.Length(); i++)
								{
									v2 = BRepBuilderAPI_MakeVertex(aPoints(i));
									extrema.LoadS2(v2);
									extrema.Perform();
									//
									if (extrema.IsDone() == 1)
									{
										if (extrema.Value() < eps)
										{
											startId = i;
											break;
										}
										if (extrema.Value() < min)
										{
											min = extrema.Value();
											minId = i;
										}
									}
								}
								if (startId == -1) startId = minId;
								//
								min = 1E100;
								minId = -1;
								v1 = TopExp::LastVertex(edge);
								extrema.LoadS1(v1);
								for (Standard_Integer i = 1; i <= aPoints.Length(); i++)
								{
									v2 = BRepBuilderAPI_MakeVertex(aPoints(i));
									extrema.LoadS2(v2);
									extrema.Perform();
									//
									if (extrema.IsDone() == 1)
									{
										if (extrema.Value() < eps)
										{
											endId = i;
											break;
										}
										if (extrema.Value() < min)
										{
											min = extrema.Value();
											minId = i;
										}
									}
								}
								if (endId == -1) endId = minId;
								//
								outfile << CurveTypeToString(edgeType) << " " << edgenr; // edge type and ID - 2nd time
								outfile << " " << startId;
								//
								extrema.LoadS1(edge);
								for (Standard_Integer i = 1; i <= aPoints.Length(); i++)
								{
									if (i == startId || i == endId) continue;
									//
									v2 = BRepBuilderAPI_MakeVertex(aPoints(i));
									extrema.LoadS2(v2);
									extrema.Perform();
									//
									if (extrema.IsDone() == 1 && extrema.Value() < eps)
									{
										outfile << " " << i;
									}
								}
								//
								outfile << " " << endId;
								outfile << endl;
							}
						}
					}
				}
				else
				{
					// The face is written without its nodes, triangles and edges, so it reaches
					// PrePoMax as a hole in the part. Say so instead of letting it pass unnoticed.
					numOfFacesWithoutTriangulation++;
					// The caller is mid-way through its "Exporting ..." line, so start a new one
					cout << endl << "WARNING: face " << facenr << " (" << SurfaceTypeToString(surfaceType)
						 << ") could not be triangulated and is missing from the visualization." << endl;
				}
			}
		}
		//
		GProp_GProps props;
		if (TopAbs_Shape == TopAbs_SOLID) BRepGProp::VolumeProperties(occgeom->shape, props);
		else if (TopAbs_Shape == TopAbs_SHELL) BRepGProp::SurfaceProperties(occgeom->shape, props);
		else if (TopAbs_Shape == TopAbs_FACE) BRepGProp::SurfaceProperties(occgeom->shape, props);
		//
		double mass = props.Mass();
		gp_Pnt cg = props.CentreOfMass();
		gp_Mat im = props.MatrixOfInertia();
		gp_Mat H;
		GProp::HOperator(cg, gp_Pnt(0, 0, 0), mass, H);
		gp_Mat im0 = im;
		im0.Add(H);
		//
		outfile.precision(8);
		//
		outfile << "**********" << endl;
		outfile << "Geometry properties" << endl;
		outfile << "**********" << endl;
		if (TopAbs_Shape == TopAbs_SOLID) outfile << "Volume: ";
		else if (TopAbs_Shape == TopAbs_SHELL) outfile << "Area: ";
		else if (TopAbs_Shape == TopAbs_FACE) outfile << "Area: ";
		outfile << mass << endl;
		outfile << "Center of mass: " << cg.Coord().X() << " " << cg.Coord().Y() << " " << cg.Coord().Z() << endl;
		outfile << "Matrix of inertia around the center of mass:" << endl;
		outfile << im(1, 1) << " " << im(1, 2) << " " << im(1, 3) << endl;
		outfile << im(2, 1) << " " << im(2, 2) << " " << im(2, 3) << endl;
		outfile << im(3, 1) << " " << im(3, 2) << " " << im(3, 3) << endl;
		outfile << "Matrix of inertia around the origin:" << endl;
		outfile << im0(1, 1) << " " << im0(1, 2) << " " << im0(1, 3) << endl;
		outfile << im0(2, 1) << " " << im0(2, 2) << " " << im0(2, 3) << endl;
		outfile << im0(3, 1) << " " << im0(3, 2) << " " << im0(3, 3) << endl;
	}
	void SearchConnectedShapes(int currShapeNr, list<int> * connection, list<int> * connectedShapes, int * shapesToVisit)
	{
		bool found = (std::find(connection->begin(), connection->end(), currShapeNr) != connection->end());
		if (found) return;
		//
		connection->push_back(currShapeNr);
		shapesToVisit[currShapeNr] = 0;
		//
		list<int> connectedShape = connectedShapes[currShapeNr];
		if (connectedShape.size() > 0)
		{
			for (std::list<int>::iterator it = connectedShape.begin(); it != connectedShape.end(); it++)
			{
				SearchConnectedShapes(*it, connection, connectedShapes, shapesToVisit);
			}
			connectedShapes[currShapeNr].clear();
		}
	}
	TopoDS_Shape ReverseFaceOrientation(const TopoDS_Shape& shape, const TopoDS_Face& face)
	{
		//
		// https://www.opencascade.com/content/reverse-face-orientation
		//
		Handle(BRepTools_ReShape) rebuild = new BRepTools_ReShape();
		//rebuild->e ->ModeConsiderOrientation() = Standard_True;
		TopoDS_Shape newface = face.Complemented();
		rebuild->Replace(face, newface);
		TopoDS_Shape newshape = rebuild->Apply(shape, TopAbs_FACE);
		return newshape;
	}
	OCCGeometry * ReverseBrepFaceOrientations(Ng_OCC_Geometry * geom, int * faceIds, int numOfIds)
	{
		OCCGeometry * occgeom = (OCCGeometry*)geom;
		//
		int facenr;
		bool faceFound;
		TopoDS_Face face;
		//
		for (int i = 0; i < numOfIds; i++)
		{
			faceFound = false;
			//
			for (TopExp_Explorer exp0(occgeom->shape, TopAbs_FACE); exp0.More(); exp0.Next())
			{
				face = TopoDS::Face(exp0.Current());
				facenr = occgeom->fmap.FindIndex(face);
				//
				if (facenr == faceIds[i])
				{
					faceFound = true;
					break;
				}
			}
			if (faceFound) occgeom->shape = ReverseFaceOrientation(occgeom->shape, face);
		}
		//
		return occgeom;
	}
	bool WriteShapeToBrepFile(const TopoDS_Shape& shape, const char* filename, int numOfSplitFaces)
	{
		bool result = false;
		TopoDS_Shape splitShape = shape;
		//
		if (numOfSplitFaces > 1)
		{
			ShapeUpgrade_ShapeDivideClosed divider = ShapeUpgrade_ShapeDivideClosed(shape);
			//divider.SetPrecision(0.01);
			//divider.SetMinTolerance(0.1 * 0.01);
			//divider.SetMaxTolerance(0.01);
			divider.SetNbSplitPoints(numOfSplitFaces - 1);
			if (divider.Perform())
			{
				splitShape = divider.Result();
				// The divider rebuilds the pcurves of the edges it touches but leaves their
				// SameParameter and SameRange flags set, so the flags no longer describe the
				// edge. BRepMesh trusts them and drops the triangulation of the affected faces,
				// which then arrive in PrePoMax as holes in the part. Recomputing the flags
				// here keeps the written file valid. The shape must not be copied first -
				// BRepBuilderAPI_Copy of a freshly divided shape makes SameParameter throw.
				try
				{
					BRepLib::SameParameter(splitShape, Precision::Confusion(), Standard_True);
				}
				catch (Standard_Failure& ex)
				{
					cout << endl << "WARNING: the edges of " << filename << " could not be repaired after the"
						 << " periodic face split (" << typeid(ex).name() << ")."
						 << " The geometry is written unrepaired and may be invalid." << endl;
				}
			}
		}
		//
		BRepTools::Write(splitShape, filename, Message_ProgressRange());
		return result;
	}
	void ExtractBodies(const TopoDS_Shape& shape, BRep_Builder& builder, TopoDS_Compound& faceCompound, TopoDS_Compound& shellCompound,	
					   TopoDS_Compound& solidCompound, bool closeShells)
	{
		TopoDS_Shell shell;
		//
		for (TopExp_Explorer exp0(shape, TopAbs_SOLID); exp0.More(); exp0.Next())
		{
			builder.Add(solidCompound, exp0.Current());
		}
		//
		for (TopExp_Explorer exp0(shape, TopAbs_SHELL, TopAbs_SOLID); exp0.More(); exp0.Next())
		{
			shell = TopoDS::Shell(exp0.Current());
			//
			if (closeShells && BRep_Tool::IsClosed(shell))
			{
				// Try to convert it into a solid
				TopoDS_Solid solid;
				BRepBuilderAPI_MakeSolid solidMaker;
				solidMaker.Add(shell);
				solid = solidMaker.Solid();
				// Optional: check if the resulting solid is valid
				BRepCheck_Analyzer analyzer(solid);
				if (analyzer.IsValid())
				{
					std::cout << "Successfully built solid from shell." << std::endl;
					// Use or store the solid as needed
					builder.Add(solidCompound, solid);
				}
				else
				{
					// Solid is invalid
					builder.Add(shellCompound, shell);
				}
			}
			else
			{
				// Shell is not closed and cannot be converted to a solid
				builder.Add(shellCompound, shell);
			}
		}
		//
		for (TopExp_Explorer exp0(shape, TopAbs_FACE, TopAbs_SHELL); exp0.More(); exp0.Next())
		{
			builder.MakeShell(shell);			// create the shell
			builder.Add(shell, exp0.Current()); // add the face to the shell
			builder.Add(shellCompound, shell);
		}
	}
	int WriteAssemblyAsSolidCompounds(OCCGeometry* occgeom, int countStart, const char* filename, int numOfSplitFaces)
	{
		// occgeom->RebuildGeometry();
		// Search for shared surfaces
		int numOfFaces = occgeom->fmap.Extent();
		Array<int> face2solid[2];
		for (int i = 0; i < 2; i++)
		{
			face2solid[i].SetSize(numOfFaces);
			face2solid[i] = -1;
		}
		int numOfSolids;
		int facenr;
		int currSolidnr = 0;
		int solidnr0 = 0;
		TopoDS_Solid solid;
		TopoDS_Solid * solids;
		list<int> * connectedSolids;	// for each solidId store a list of connected solids
		//
		numOfSolids = occgeom->somap.Extent();
		solids = new TopoDS_Solid[numOfSolids];
		connectedSolids = new list<int>[numOfSolids];
		//
		for (TopExp_Explorer exp0(occgeom->shape, TopAbs_SOLID); exp0.More(); exp0.Next())
		{
			currSolidnr++;
			solid = TopoDS::Solid(exp0.Current());
			solids[currSolidnr - 1] = solid;
			//
			for (TopExp_Explorer exp1(solid, TopAbs_FACE); exp1.More(); exp1.Next())
			{
				TopoDS_Face face = TopoDS::Face(exp1.Current());
				facenr = occgeom->fmap.FindIndex(face);
				// First solid
				if (face2solid[0][facenr - 1] == -1)
					face2solid[0][facenr - 1] = currSolidnr - 1;
				// Second solid
				else
				{
					face2solid[1][facenr - 1] = currSolidnr - 1;
					//
					solidnr0 = face2solid[0][facenr - 1];
					connectedSolids[solidnr0].push_back(currSolidnr - 1);
					connectedSolids[currSolidnr - 1].push_back(solidnr0);
				}
			}
		}
		// Find connected parts
		int * solidsToVisit;
		solidsToVisit = new int[numOfSolids];
		for (int i = 0; i < numOfSolids; i++) solidsToVisit[i] = 1;
		list<int> * connection;
		list<list<int>> allConnections;
		//
		for (int i = 0; i < numOfSolids; i++)
		{
			if (solidsToVisit[i] == 1)
			{
				connection = new list<int>();
				SearchConnectedShapes(i, connection, connectedSolids, solidsToVisit);
				//
				allConnections.push_back(*connection);
			}
		}
		// Export																									
		char solidFileNameWithPath[_MAX_FNAME];
		char shellFileNameWithPath[_MAX_FNAME];
		char full_path[_MAX_PATH];
		char drive[_MAX_DRIVE];
		char dir[_MAX_DIR];
		char fname[_MAX_FNAME];
		char ext[_MAX_EXT];
		//
		_splitpath_s(filename, drive, dir, fname, ext);
		// Export Solids
		currSolidnr = countStart;
		for (std::list<list<int>>::iterator it1 = allConnections.begin(); it1 != allConnections.end(); it1++)
		{
			stringstream ss;
			list<int>& oneConnection = *it1;
			if (oneConnection.size() == 1)
			{
				//ss << fname << "_solid_" << currSolidnr;
				ss << fname << "_solid_" << std::setfill('0') << std::setw(4) << currSolidnr;
				//
				_makepath_s(solidFileNameWithPath, drive, dir, ss.str().c_str(), ext);
				WriteShapeToBrepFile(solids[*oneConnection.begin()], solidFileNameWithPath, numOfSplitFaces);
				//BRepTools::Write(solids[*oneConnection.begin()], solidFileNameWithPath, Message_ProgressRange());
			}
			else if (oneConnection.size() > 1)
			{
				BOPAlgo_Builder aBuilder;
				//
				for (std::list<int>::iterator it2 = oneConnection.begin(); it2 != oneConnection.end(); it2++)
				{
					aBuilder.AddArgument(solids[*it2]);
				}
				//
				aBuilder.Perform();
				if (aBuilder.HasErrors()) continue;
				//
				const TopoDS_Shape& aRes = aBuilder.Shape();
				//
				//ss << fname << "_compound_solid_" << currSolidnr;
				ss << fname << "_compound_solid_" << std::setfill('0') << std::setw(4) << currSolidnr;
				//
				_makepath_s(solidFileNameWithPath, drive, dir, ss.str().c_str(), ext);
				WriteShapeToBrepFile(aRes, solidFileNameWithPath, numOfSplitFaces);
				//BRepTools::Write(aRes, solidFileNameWithPath, Message_ProgressRange());
			}
			currSolidnr++;
		}
		//
		delete[] solids;
		delete[] connectedSolids;
		delete[] solidsToVisit;
		//
		return currSolidnr;
	}
	int WriteAssemblyAsShellCompounds(OCCGeometry* occgeom, int countStart, const char* filename, int numOfSplitFaces)
	{
		// Search for shared edges
		int numOfEdges = occgeom->emap.Extent();
		list<int> oneEdgeShells;
		list<int> * edgeShells;
		edgeShells = new list<int>[numOfEdges];
		list<int> visitedEdges;
		//
		int numOfShells;
		int edgeNr;
		int currShellNr = 0;
		TopoDS_Shell shell;
		TopoDS_Shell * shells;
		list<int> * connectedShells;	// for each shellId store a list of connected shells
		//
		//numOfShells = occgeom->shmap.Extent();
		// Count shells that are not solids
		numOfShells = 0;
		for (TopExp_Explorer exp0(occgeom->shape, TopAbs_SHELL, TopAbs_SOLID); exp0.More(); exp0.Next())
		{
			numOfShells++;
		}
		shells = new TopoDS_Shell[numOfShells];
		connectedShells = new list<int>[numOfShells];
		//
		for (TopExp_Explorer exp0(occgeom->shape, TopAbs_SHELL, TopAbs_SOLID); exp0.More(); exp0.Next())
		{
			currShellNr++;
			shell = TopoDS::Shell(exp0.Current());
			shells[currShellNr - 1] = shell;
			//
			visitedEdges.clear();
			//
			for (TopExp_Explorer exp1(shell, TopAbs_EDGE); exp1.More(); exp1.Next())
			{
				TopoDS_Edge edge = TopoDS::Edge(exp1.Current());
				edgeNr = occgeom->emap.FindIndex(edge);
				// Check if edge was visited: a shell contains multiple faces sharing the same edge more than once
				bool found = (std::find(visitedEdges.begin(), visitedEdges.end(), edgeNr) != visitedEdges.end());
				if (!found)
				{
					// Add edge
					visitedEdges.push_back(edgeNr);
					// First shell
					if (edgeShells[edgeNr - 1].size() == 0)
						edgeShells[edgeNr - 1].push_back(currShellNr - 1);
					// The rest of the shells
					else
					{
						oneEdgeShells = edgeShells[edgeNr - 1];
						for (std::list<int>::iterator it = oneEdgeShells.begin(); it != oneEdgeShells.end(); it++)
						{
							connectedShells[*it].push_back(currShellNr - 1);
							connectedShells[currShellNr - 1].push_back(*it);
						}
						edgeShells[edgeNr - 1].push_back(currShellNr - 1);	// must not be oneEdgeShell
					}
				}
			}
		}
		// Find connected shells
		int * shellsToVisit;
		shellsToVisit = new int[numOfShells];
		for (int i = 0; i < numOfShells; i++) shellsToVisit[i] = 1;
		list<int> * connection;
		list<list<int>> allConnections;
		//
		for (int i = 0; i < numOfShells; i++)
		{
			if (shellsToVisit[i] == 1)
			{
				connection = new list<int>();
				SearchConnectedShapes(i, connection, connectedShells, shellsToVisit);
				//
				allConnections.push_back(*connection);
			}
		}
		// Export																									
		char shellFileNameWithPath[_MAX_FNAME];
		char full_path[_MAX_PATH];
		char drive[_MAX_DRIVE];
		char dir[_MAX_DIR];
		char fname[_MAX_FNAME];
		char ext[_MAX_EXT];
		//
		_splitpath_s(filename, drive, dir, fname, ext);
		// Shells
		int shellNr = countStart;
		for (std::list<list<int>>::iterator it1 = allConnections.begin(); it1 != allConnections.end(); it1++)
		{
			stringstream ss;
			list<int>& oneConnection = *it1;
			if (oneConnection.size() == 1)
			{
				//ss << fname << "_shell_" << shellNr;
				ss << fname << "_shell_" << std::setfill('0') << std::setw(4) << shellNr;
				//
				_makepath_s(shellFileNameWithPath, drive, dir, ss.str().c_str(), ext);
				//BRepTools::Write(shells[*oneConnection.begin()], shellFileNameWithPath, Message_ProgressRange());
				WriteShapeToBrepFile(shells[*oneConnection.begin()], shellFileNameWithPath, numOfSplitFaces);
			}
			else if (oneConnection.size() > 1)
			{
				BOPAlgo_Builder aBuilder;
				//
				for (std::list<int>::iterator it2 = oneConnection.begin(); it2 != oneConnection.end(); it2++)
				{
					aBuilder.AddArgument(shells[*it2]);
				}
				//
				aBuilder.Perform();
				if (aBuilder.HasErrors()) continue;
				//
				const TopoDS_Shape& aRes = aBuilder.Shape();
				//
				//ss << fname << "_shell_" << shellNr;
				ss << fname << "_shell_" << std::setfill('0') << std::setw(4) << shellNr;
				//
				_makepath_s(shellFileNameWithPath, drive, dir, ss.str().c_str(), ext);
				//
				//BRepTools::Write(aRes, shellFileNameWithPath, Message_ProgressRange());
				WriteShapeToBrepFile(aRes, shellFileNameWithPath, numOfSplitFaces);
			}
			shellNr++;
		}
		delete[] edgeShells;
		delete[] shells;
		delete[] connectedShells;
		delete[] shellsToVisit;
		//
		return shellNr;
	}
	void DumpShapeTopology(const TopoDS_Shape& shape)
	{
		std::map<TopAbs_ShapeEnum, int> counts;
		//
		for (TopExp_Explorer exp(shape, TopAbs_SHAPE); exp.More(); exp.Next())
		{
			counts[exp.Current().ShapeType()]++;
		}
		//
		std::cout << "=== Topology summary ===\n";
		for (int i = TopAbs_COMPOUND; i <= TopAbs_VERTEX; ++i)
		{
			auto type = static_cast<TopAbs_ShapeEnum>(i);
			std::cout << TopAbs::ShapeTypeToString(type)
				<< ": " << counts[type] << "\n";
		}
	}

	
	
	DLL_HEADER Ng_OCC_Geometry * Ng_OCC_NewGeometry()
	{
		return (Ng_OCC_Geometry*)(void*)new OCCGeometry;
	}

	// Delete the OCC Geometry Object
	DLL_HEADER Ng_Result Ng_OCC_DeleteGeometry(Ng_OCC_Geometry * geom)
	{
		if (geom != NULL)
		{
			delete (OCCGeometry*)geom;
			geom = NULL;
			return NG_OK;
		}

		return NG_ERROR;
	}

	// Loads geometry from STEP File
	DLL_HEADER Ng_OCC_Geometry * Ng_OCC_Load_STEP(const char * filename)
	{
		// Call the STEP File Load function. Note.. the geometry class 
		// is created and instantiated within the load function
		OCCGeometry * occgeom = LoadOCC_STEP(filename);

		//TopoDS_Shape top_shape = occgeom->shape;
		//DumpShapeTopology(top_shape);

		return ((Ng_OCC_Geometry *)occgeom);
	}

	// Loads geometry from IGES File
	DLL_HEADER Ng_OCC_Geometry * Ng_OCC_Load_IGES(const char * filename)
	{
		// Call the IGES File Load function. Note.. the geometry class 
		// is created and instantiated within the load function
		OCCGeometry * occgeom = LoadOCC_IGES(filename);

		return ((Ng_OCC_Geometry *)occgeom);
	}

	// Loads geometry from BREP File
	DLL_HEADER Ng_OCC_Geometry * Ng_OCC_Load_BREP(const char * filename)
	{
		// Call the BREP File Load function. Note.. the geometry class 
		// is created and instantiated within the load function
		OCCGeometry * occgeom = LoadOCC_BREP(filename);

		return ((Ng_OCC_Geometry *)occgeom);
	}

	// Save geometry to STEP File
	DLL_HEADER Ng_Result Ng_OCC_Save_STEP(Ng_OCC_Geometry * geom, const char * filename)
	{
		// Call the STEP File Load function. Note.. the geometry class 
		// is created and instantiated within the load function
		OCCGeometry * occgeom = (OCCGeometry*)geom;
		//
		STEPControl_Writer aWriter;
		IFSelect_ReturnStatus aStat = aWriter.Transfer(occgeom->shape, STEPControl_AsIs);
		aStat = aWriter.Write(filename);
		if (aStat != IFSelect_RetDone)
		{
			cout << "Writing error" << endl;
			return(NG_ERROR);
		}
		else
			return(NG_OK);
	}
	
	// Save geometry to BREP File
	DLL_HEADER Ng_Result Ng_OCC_Save_BREP(Ng_OCC_Geometry * geom, const char * filename)
	{
		OCCGeometry * occgeom = (OCCGeometry*)geom;
		//
		BRepTools::Write(occgeom->shape, filename, Message_ProgressRange());
		//
		return(NG_OK);
	}

	// Locally limit the size of the mesh to be generated at various points based on the topology of the geometry
	DLL_HEADER Ng_Result Ng_OCC_SetLocalMeshSize(Ng_OCC_Geometry * geom, double deflection, Ng_Mesh * mesh, Ng_Meshing_Parameters * mp)
	{
		// Variables
		OCCGeometry * occgeom = (OCCGeometry*)geom;
		Mesh * me = (Mesh*)mesh;
		// Set geometry type
		me->geomtype = Mesh::GEOM_OCC;
		// Load mp to global variable mparam
		mp->Transfer_Parameters();
		//
		occparam.resthcloseedgeenable = mp->closeedgeenable;
		occparam.resthcloseedgefac = mp->closeedgefact;
		// Delete the mesh structures in order to start with a clean slate
		me->DeleteMesh();
		// Build visualization
		occgeom->BuildVisualizationMesh(deflection);
		// Set local mesh size
		OCCSetLocalMeshSize(*occgeom, *me);
		//
		return(NG_OK);
	}

	// Mesh the edges and add Face descriptors to prepare for surface meshing
	DLL_HEADER Ng_Result Ng_OCC_GenerateEdgeMesh(Ng_OCC_Geometry * geom, Ng_Mesh * mesh, Ng_Meshing_Parameters * mp)
	{
		OCCGeometry * occgeom = (OCCGeometry*)geom;
		Mesh * me = (Mesh*)mesh;

		mp->Transfer_Parameters();

		OCCFindEdges(*occgeom, *me);

		if ((me->GetNP()) && (me->GetNFD()))
		{
			return NG_OK;
		}
		else
		{
			return NG_ERROR;
		}
	}

	// Mesh the edges and add Face descriptors to prepare for surface meshing
	DLL_HEADER Ng_Result Ng_OCC_GenerateSurfaceMesh(Ng_OCC_Geometry * geom, Ng_Mesh * mesh, Ng_Meshing_Parameters * mp)
	{
		try 
		{
			int numpoints = 0;

			OCCGeometry * occgeom = (OCCGeometry*)geom;
			Mesh * me = (Mesh*)mesh;

			// Set the internal meshing parameters structure from the nglib meshing 
			// parameters structure
			mp->Transfer_Parameters();


			// Only go into surface meshing if the face descriptors have already been added
			if (!me->GetNFD())
				return NG_ERROR;

			numpoints = me->GetNP();

			// Initially set up only for surface meshing without any optimisation
			int perfstepsend = MESHCONST_MESHSURFACE;

			// Check and if required, enable surface mesh optimisation step
			if (mp->optsurfmeshenable)
			{
				perfstepsend = MESHCONST_OPTSURFACE;
			}

			OCCMeshSurface(*occgeom, *me, perfstepsend);

			me->CalcSurfacesOfNode();

			if (me->GetNP() < numpoints)	// Added for support of coarse meshes - cube
				return NG_ERROR;

			if (me->GetNSE() <= 0)
				return NG_ERROR;

			return NG_OK;
		}
		catch (...)
		{
			return NG_ERROR;
		}
	}
	// Extract the face map from the OCC geometry
	// The face map basically gives an index to each face in the geometry, 
	// which can be used to access a specific face
	DLL_HEADER Ng_Result Ng_OCC_GetFMap(Ng_OCC_Geometry * geom, Ng_OCC_TopTools_IndexedMapOfShape * FMap)
	{
		OCCGeometry* occgeom = (OCCGeometry*)geom;
		TopTools_IndexedMapOfShape *occfmap = (TopTools_IndexedMapOfShape *)FMap;

		// Copy the face map from the geometry to the given variable
		occfmap->Assign(occgeom->fmap);

		if (occfmap->Extent())
		{
			return NG_OK;
		}
		else
		{
			return NG_ERROR;
		}
	}

	DLL_HEADER Ng_Result Ng_OCC_ExportVisualization(Ng_OCC_Geometry * geom, double deflection, const char* filename)
	{
		return Ng_OCC_ExportVisualization(geom, deflection, true, 0.5, filename);
	}
	
	DLL_HEADER Ng_Result Ng_OCC_ExportVisualization(Ng_OCC_Geometry* geom, double linearDeflection, bool relative, double angularDeflection, const char* filename)
	{
		OCCGeometry* occgeom = (OCCGeometry*)geom;

		//occgeom->RebuildGeometry();

		// Build visualization		
		cout << "Building visualization mesh:" << endl;
		occgeom->BuildVisualizationMesh(linearDeflection, relative, angularDeflection);
		//
		TopLoc_Location aLocation;
		Standard_Integer nId;
		gp_Pnt aPnt1;
		int facenr;
		int edgenr;
		int solidnr = 0;
		int shellnr = 0;
		Standard_Integer N1;
		Standard_Integer N2;
		Standard_Integer N3;
		//
		const string& stringName = filename;
		ofstream outfile(stringName.c_str());
		// How many edges took the expensive node-by-node fallback. Should be 0 for
		// well-formed geometry; anything else is worth looking at.
		int numOfEdges = 0;
		int numOfFallbackEdges = 0;
		// Faces BRepMesh could not triangulate. They are written without any nodes,
		// triangles or edges, so PrePoMax silently ends up with a part full of holes.
		int numOfFacesWithoutTriangulation = 0;
		// Free Solids
		int numOfSolids = occgeom->somap.Extent();
		if (numOfSolids > 0)
		{
			cout << "Exporting solids: " << numOfSolids;
			ExportItemVisualization(occgeom, TopAbs_SOLID, outfile, numOfEdges, numOfFallbackEdges, numOfFacesWithoutTriangulation);
			cout << " ... done" << endl;
		}

		// Free Shells
		int numOfShells = 0;
		for (TopExp_Explorer exp0(occgeom->shape, TopAbs_SHELL, TopAbs_SOLID); exp0.More(); exp0.Next())
		{
			numOfShells++;
		}
		if (numOfShells > 0)
		{
			cout << "Exporting shells: " << numOfShells;
			ExportItemVisualization(occgeom, TopAbs_SHELL, outfile, numOfEdges, numOfFallbackEdges, numOfFacesWithoutTriangulation);
			cout << " ... done" << endl;
		}
		// Free Faces
		int numOfFaces = 0;
		for (TopExp_Explorer exp0(occgeom->shape, TopAbs_FACE, TopAbs_SHELL); exp0.More(); exp0.Next())
		{
			numOfFaces++;
		}
		if (numOfFaces > 0)
		{
			cout << "Exporting free faces: " << numOfFaces;
			ExportItemVisualization(occgeom, TopAbs_FACE, outfile, numOfEdges, numOfFallbackEdges, numOfFacesWithoutTriangulation);
			cout << " ... done" << endl;
		}
		cout << "Edges: " << numOfEdges << " total, " << numOfFallbackEdges
			 << " needed the node-by-node fallback" << endl;
		if (numOfFacesWithoutTriangulation > 0)
			cout << "WARNING: " << numOfFacesWithoutTriangulation
				 << " face(s) could not be triangulated. The geometry is incomplete and most likely invalid." << endl;
		cout << "Closing visualization file.";
		outfile.close();
		cout << " ... done" << endl;
		//
		return(NG_OK);
	}

	DLL_HEADER Ng_Result Ng_OCC_WriteAssemblyAsCompounds(Ng_OCC_Geometry * geom, const char* filename, int numOfSplitFaces, bool closeShells)
	{
		// Call the STEP File Load function. Note.. the geometry class 
		// is created and instantiated within the load function
		//OCCGeometry* occgeom1 = LoadOCC_STEP("C:\\Temp\\PTCmodels\\Test1.stp");
		//TopoDS_Shape top_shape1 = occgeom1->shape;
		//DumpShapeTopology(top_shape1);

		int count = 1;
		OCCGeometry* newoccgeo;
		OCCGeometry* occgeom = (OCCGeometry*)geom;
		TopoDS_Shape top_shape = occgeom->shape;
		// Extract shapes - build a solid from a closed shape
		BRep_Builder builder;
		TopoDS_Compound faceCompound;
		TopoDS_Compound shellCompound;
		TopoDS_Compound solidCompound;
		builder.MakeCompound(faceCompound);
		builder.MakeCompound(shellCompound);
		builder.MakeCompound(solidCompound);
		ExtractBodies(top_shape, builder, faceCompound, shellCompound, solidCompound, closeShells);
		// Solids
		newoccgeo = new OCCGeometry;
		newoccgeo->shape = solidCompound;
		newoccgeo->changed = 1;
		newoccgeo->BuildFMap();
		//newoccgeo->CalcBoundingBox();
		count = WriteAssemblyAsSolidCompounds(newoccgeo, count, filename, numOfSplitFaces);
		// Shells
		newoccgeo = new OCCGeometry;
		newoccgeo->shape = shellCompound;
		newoccgeo->changed = 1;
		newoccgeo->BuildFMap();
		//newoccgeo->CalcBoundingBox();
		count = WriteAssemblyAsShellCompounds(newoccgeo, count, filename, numOfSplitFaces);
		// Free faces
		char shellFileNameWithPath[_MAX_FNAME];
		char full_path[_MAX_PATH];
		char drive[_MAX_DRIVE];
		char dir[_MAX_DIR];
		char fname[_MAX_FNAME];
		char ext[_MAX_EXT];
		//
		_splitpath_s(filename, drive, dir, fname, ext);
		// Export free faces
		int faceNr = count;
		for (TopExp_Explorer exp0(faceCompound, TopAbs_FACE, TopAbs_SHELL); exp0.More(); exp0.Next())
		{
			faceNr++;
			//
			stringstream ss;
			ss << fname << "_face_" << faceNr;
			//
			_makepath_s(shellFileNameWithPath, drive, dir, ss.str().c_str(), ext);
			//
			TopoDS_Face face = TopoDS::Face(exp0.Current());
			//
			//BRepTools::Write(face, shellFileNameWithPath, Message_ProgressRange());
			WriteShapeToBrepFile(face, shellFileNameWithPath, numOfSplitFaces);
		}
		//
		return NG_OK;
	}

	DLL_HEADER int Ng_OCC_WriteAssemblyAsSolidParts(Ng_OCC_Geometry * geom, int countStart, const char* filename, int numOfSplitFaces)
	{
		OCCGeometry * occgeom = (OCCGeometry*)geom;
		// Export
		char solidFileNameWithPath[_MAX_FNAME];
		char full_path[_MAX_PATH];
		char drive[_MAX_DRIVE];
		char dir[_MAX_DIR];
		char fname[_MAX_FNAME];
		char ext[_MAX_EXT];
		//
		_splitpath_s(filename, drive, dir, fname, ext);
		//
		int solidNr = countStart;
		for (TopExp_Explorer exp0(occgeom->shape, TopAbs_SOLID); exp0.More(); exp0.Next())
		{
			solidNr++;
			//
			stringstream ss;
			//ss << fname << "_solid_" << solidNr;
			ss << fname << "_solid_" << std::setfill('0') << std::setw(4) << solidNr;
			//
			_makepath_s(solidFileNameWithPath, drive, dir, ss.str().c_str(), ext);
			//
			TopoDS_Solid solid = TopoDS::Solid(exp0.Current());
			//
			//BRepTools::Write(solid, solidFileNameWithPath, Message_ProgressRange());
			WriteShapeToBrepFile(solid, solidFileNameWithPath, numOfSplitFaces);
		}
		//
		return solidNr;
	}

	DLL_HEADER int Ng_OCC_WriteAssemblyAsShellParts(Ng_OCC_Geometry * geom, int countStart, const char* filename, int numOfSplitFaces)
	{
		countStart = WriteAssemblyAsShellCompounds((OCCGeometry*)geom, countStart, filename, numOfSplitFaces);
		return countStart;
	}

	DLL_HEADER Ng_Result Ng_OCC_CreateCompoundFromBrep(int argc, char* argv[])
	{
		try
		{
			if (CreateCompoundFromBrep(argc, argv))
				return NG_OK;
			else
			{
				PrintMessage(1, "An error occurred in CreateCompoundFromBrep.");
				return NG_ERROR;
			}
		}
		catch (exception ex)
		{
			PrintMessage(1, "An error occurred in Ng_OCC_CreateCompoundFromBrep.");
		}
	}

	DLL_HEADER Ng_OCC_Geometry * Ng_OCC_ScaleGeometry(Ng_OCC_Geometry * geom, double Tx, double Ty, double Tz, double sx, double sy, double sz)
	{
		try
		{
			OCCGeometry * occgeom = (OCCGeometry*)geom;
			occgeom = ScaleGeometry(occgeom, Tx, Ty, Tz, sx, sy, sz);
			return ((Ng_OCC_Geometry *)occgeom);
		}
		catch (exception ex)
		{
			PrintMessage(1, "An error occurred in Ng_OCC_ScaleGeometry.");
		}
	}

	DLL_HEADER Ng_OCC_Geometry * Ng_OCC_ReverseFaceOrientations(Ng_OCC_Geometry * geom, int * faceIds, int numOfIds)
	{
		try
		{			
			OCCGeometry * occgeom = ReverseBrepFaceOrientations(geom, faceIds, numOfIds);
			return ((Ng_OCC_Geometry *)occgeom);
		}
		catch (exception ex)
		{
			PrintMessage(1, "An error occurred in Ng_OCC_ReverseFaceOrientations.");
		}
	}
	
	DLL_HEADER Ng_OCC_Geometry * Ng_OCC_SplitAFaceUsingTwoPoints(Ng_OCC_Geometry * geom, int faceId, double x1, double y1, double z1, double x2, double y2, double z2)
	{
		try
		{
			OCCGeometry * occgeom = (OCCGeometry*)geom;
			occgeom = SplitAFaceUsingTwoPoints(occgeom, faceId, x1, y1, z1, x2, y2, z2);
			return ((Ng_OCC_Geometry *)occgeom);
		}
		catch (exception ex)
		{
			PrintMessage(1, "An error occurred in Ng_OCC_SplitAFaceUsingTwoPoints.");
		}
	}

	DLL_HEADER Ng_OCC_Geometry* Ng_OCC_ConvertToShell(Ng_OCC_Geometry* geom)
	{
		try
		{
			OCCGeometry* occgeom = (OCCGeometry*)geom;
			//
			BRep_Builder builder;
			TopoDS_Compound shellCompound;
			builder.MakeCompound(shellCompound);
			//
			for (TopExp_Explorer exp0(occgeom->shape, TopAbs_SOLID); exp0.More(); exp0.Next())
			{
				for (TopExp_Explorer exp1(exp0.Current(), TopAbs_SHELL); exp1.More(); exp1.Next())
				{
					builder.Add(shellCompound, exp1.Current());
				}
			}
			//
			OCCGeometry* newoccgeo;
			newoccgeo = new OCCGeometry;
			newoccgeo->shape = shellCompound;
			newoccgeo->changed = 1;
			newoccgeo->BuildFMap();
			//
			return ((Ng_OCC_Geometry*)newoccgeo);
		}
		catch (exception ex)
		{
			PrintMessage(1, "An error occurred in Ng_OCC_ConvertToShell.");
		}
	}
	
	DLL_HEADER Ng_OCC_Geometry* Ng_OCC_ConvertToSolid(Ng_OCC_Geometry* geom)
	{
		try
		{
			OCCGeometry* occgeom = (OCCGeometry*)geom;
			//
			BRep_Builder builder;
			TopoDS_Shell shell;
			TopoDS_Compound solidCompound;
			builder.MakeCompound(solidCompound);
			//
			for (TopExp_Explorer exp0(occgeom->shape, TopAbs_SHELL, TopAbs_SOLID); exp0.More(); exp0.Next())
			{
				shell = TopoDS::Shell(exp0.Current());
				//
				if (BRep_Tool::IsClosed(shell))
				{
					// Try to convert it into a solid
					TopoDS_Solid solid;
					BRepBuilderAPI_MakeSolid solidMaker;
					solidMaker.Add(shell);
					solid = solidMaker.Solid();
					// Optional: check if the resulting solid is valid
					BRepCheck_Analyzer analyzer(solid);
					if (analyzer.IsValid())
					{
						std::cout << "Successfully built solid from shell." << std::endl;
						// Use or store the solid as needed
						builder.Add(solidCompound, solid);
					}
				}
			}
			//
			OCCGeometry* newoccgeo;
			newoccgeo = new OCCGeometry;
			newoccgeo->shape = solidCompound;
			newoccgeo->changed = 1;
			newoccgeo->BuildFMap();
			//
			return ((Ng_OCC_Geometry*)newoccgeo);
		}
		catch (exception ex)
		{
			PrintMessage(1, "An error occurred in Ng_OCC_ConvertToSolid.");
		}
	}


	// ------------------ End - OCC Geometry / Meshing Utility Functions ----------------
#endif




   // ------------------ Begin - Meshing Parameters related functions ------------------
   // Constructor for the local nglib meshing parameters class
	DLL_HEADER Ng_Meshing_Parameters::Ng_Meshing_Parameters()
	{
		uselocalh = 1;

		maxh = 1000;
		minh = 0.0;

		fineness = 0.5;
		grading = 0.3;

		elementsperedge = 2.0;
		elementspercurve = 2.0;

		closeedgeenable = 0;
		closeedgefact = 2.0;

		minedgelenenable = 0;
		minedgelen = 1e-4;

		second_order = 0;
		quad_dominated = 0;

		meshsize_filename = 0;

		optsurfmeshenable = 1;
		optvolmeshenable = 1;

		optsteps_2d = 3;
		optsteps_3d = 3;

		invert_tets = 0;
		invert_trigs = 0;

		check_overlap = 1;
		check_overlapping_boundary = 1;
		parallel_meshing_threads = 1;
	}




	// Reset the local meshing parameters to the default values
	DLL_HEADER void Ng_Meshing_Parameters::Reset_Parameters()
	{
		uselocalh = 1;

		maxh = 1000;
		minh = 0;

		fineness = 0.5;
		grading = 0.3;

		elementsperedge = 2.0;
		elementspercurve = 2.0;

		closeedgeenable = 0;
		closeedgefact = 2.0;

		minedgelenenable = 0;
		minedgelen = 1e-4;

		second_order = 0;
		quad_dominated = 0;

		meshsize_filename = 0;

		optsurfmeshenable = 1;
		optvolmeshenable = 1;

		optsteps_2d = 3;
		optsteps_3d = 3;

		invert_tets = 0;
		invert_trigs = 0;

		check_overlap = 1;
		check_overlapping_boundary = 1;
		parallel_meshing_threads = 1;
	}




	// 
	DLL_HEADER void Ng_Meshing_Parameters::Transfer_Parameters()
	{
		mparam.uselocalh = uselocalh;

		mparam.maxh = maxh;
		mparam.minh = minh;

		mparam.grading = grading;
		mparam.curvaturesafety = elementspercurve;
		mparam.segmentsperedge = elementsperedge;

		mparam.secondorder = second_order;
		mparam.quad = quad_dominated;

		mparam.meshsizefilename = meshsize_filename;

		mparam.optsteps2d = optsteps_2d;
		mparam.optsteps3d = optsteps_3d;

		mparam.inverttets = invert_tets;
		mparam.inverttrigs = invert_trigs;

		mparam.checkoverlap = check_overlap;
		mparam.checkoverlappingboundary = check_overlapping_boundary;
		mparam.parallel_meshing_threads = parallel_meshing_threads;
	}
	// ------------------ End - Meshing Parameters related functions --------------------




	// ------------------ Begin - Second Order Mesh generation functions ----------------
	DLL_HEADER void Ng_Generate_SecondOrder(Ng_Mesh * mesh)
	{
		Refinement ref;
		ref.MakeSecondOrder(*(Mesh*)mesh);
	}




	DLL_HEADER void Ng_2D_Generate_SecondOrder(Ng_Geometry_2D * geom,
		Ng_Mesh * mesh)
	{
		((SplineGeometry2d*)geom)->GetRefinement().MakeSecondOrder(*(Mesh*)mesh);
	}




	DLL_HEADER void Ng_STL_Generate_SecondOrder(Ng_STL_Geometry * geom,
		Ng_Mesh * mesh)
	{
		((STLGeometry*)geom)->GetRefinement().MakeSecondOrder(*(Mesh*)mesh);
	}




	DLL_HEADER void Ng_CSG_Generate_SecondOrder(Ng_CSG_Geometry * geom,
		Ng_Mesh * mesh)
	{
		((CSGeometry*)geom)->GetRefinement().MakeSecondOrder(*(Mesh*)mesh);
	}




#ifdef OCCGEOMETRY
	DLL_HEADER void Ng_OCC_Generate_SecondOrder(Ng_OCC_Geometry * geom,
		Ng_Mesh * mesh)
	{
		((OCCGeometry*)geom)->GetRefinement().MakeSecondOrder(*(Mesh*)mesh);
	}
#endif
	// ------------------ End - Second Order Mesh generation functions ------------------




	// ------------------ Begin - Uniform Mesh Refinement functions ---------------------
	DLL_HEADER void Ng_Uniform_Refinement(Ng_Mesh * mesh)
	{
		Refinement ref;
		ref.Refine(*(Mesh*)mesh);
	}




	DLL_HEADER void Ng_2D_Uniform_Refinement(Ng_Geometry_2D * geom,
		Ng_Mesh * mesh)
	{
		((SplineGeometry2d*)geom)->GetRefinement().Refine(*(Mesh*)mesh);
	}




	DLL_HEADER void Ng_STL_Uniform_Refinement(Ng_STL_Geometry * geom,
		Ng_Mesh * mesh)
	{
		((STLGeometry*)geom)->GetRefinement().Refine(*(Mesh*)mesh);
	}




	DLL_HEADER void Ng_CSG_Uniform_Refinement(Ng_CSG_Geometry * geom,
		Ng_Mesh * mesh)
	{
		((CSGeometry*)geom)->GetRefinement().Refine(*(Mesh*)mesh);
	}




#ifdef OCCGEOMETRY
	DLL_HEADER void Ng_OCC_Uniform_Refinement(Ng_OCC_Geometry * geom,
		Ng_Mesh * mesh)
	{
		((OCCGeometry*)geom)->GetRefinement().Refine(*(Mesh*)mesh);
	}
#endif
	// ------------------ End - Uniform Mesh Refinement functions -----------------------
} // End of namespace nglib




// compatibility functions:
namespace netgen
{
	char geomfilename[255];

	DLL_HEADER void MyError(const char * ch)
	{
		cerr << ch;
	}




	//Destination for messages, errors, ...
	DLL_HEADER void Ng_PrintDest(const char * s)
	{
#ifdef PARALLEL
		int id = 0;
		MPI_Comm_rank(MPI_COMM_WORLD, &id);
		if (id != 0) return;
#endif
		(*mycout) << s << flush;
	}




	DLL_HEADER double GetTime()
	{
		return 0;
	}




	void ResetTime()
	{
		;
	}




	void MyBeep(int i)
	{
		;
	}




	void Render()
	{
		;
	}
} // End of namespace netgen




void Ng_Redraw() { ; }
void Ng_ClearSolutionData() { ; }
void Ng_SetSolutionData(Ng_SolutionData * soldata)
{
	delete soldata->solclass;
}
void Ng_InitSolutionData(Ng_SolutionData * soldata) { ; }


