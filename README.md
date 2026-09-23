# NetgenMesher

The meshing and CAD utility that PrePoMax shells out to for Netgen meshing and for every
OpenCASCADE geometry operation - splitting an imported assembly, building the visualisation mesh,
compounding and converting parts. It ships with PrePoMax as `Meshers\Netgen\NetGenMesher.exe` plus
`nglib.dll`; the deployable copy is vendored in the PrePoMax repository under `ThirdParty\netgen`,
which is also where its licence texts and dll inventory are documented.

## Layout

| Folder | What it is |
|---|---|
| `Netgen v5.3.1 port to OCCT 8.0.1 vs2026/` | Netgen 5.3.1 ported to OpenCASCADE 8.0.1. `Netgen-master/netgen-5.3.1/nglib/nglib.cpp` holds the PrePoMax specific entry points; `windows/nglib.sln` builds `nglib.dll`. |
| `NetGenMesher 8.0.1 vs2026/` | `NetGenLibTest/NetGenLibTest.cpp` is the command line front end - it parses the command (`STEP_ASSEMBLY_SPLIT_TO_COMPOUNDS`, `BREP_VISUALIZATION`, `BREP_MESH`, ...) and calls into nglib. `NetGenLibTest.sln` builds `NetGenMesher.exe`. |
| `NetgenMesher/` | Build output. `Release` is the configuration PrePoMax ships. |
| `redist/` | Visual C++ runtime dlls copied next to the output by `copy_runtime_dlls.bat`. |

`BREP_SPLIT` splits a part with a tool built from a plane, from a face continued past its boundary, or from
edges swept along a direction. The tool is described by a small `<property>: <values>` text file whose grammar
follows the `.vis` format - unknown properties are ignored, and a missing property keeps its default - so a new
kind of tool can be added without changing the command line. The caller passes the `Extent` the tool has to
cover, because one tool is applied to every part of a region and a tool sized per part would come out different
for each of them. `Split target: Shell` turns the same tools into a shell split: the faces named by
`Target face ids` are divided and the body they belong to is left whole, so a solid comes back a solid with
more faces instead of being cut in two. It is done by crossing the tool with those faces and imprinting the
curves that come out, since a curve cannot separate a body. `Tool type: TwoPointLine` has no tool shape at
all: it draws the line between `Point 1` and `Point 2` in the parameter space of each target face, clipped
to that face's parameter box, so the line lies on the surface however curved it is and reaches the boundary.
It is what `BREP_SPLIT_A_FACE_USING_TWO_POINTS` should have been - that command projects a straight edge with
`BRepAlgo_NormalProjection` and crashes when the chord between the two points passes through the axis of the
surface. Face and edge ids are read from `Source file` when one is given, which is what lets one part's
geometry split another. The command reports a failure when the tool added no faces, since the modelling kernel
returns a shape whether or not it made the cut. Run the executable with no arguments for the full grammar.

`BREP_MESH` reads two optional entries after `parallel_meshing_threads` in the mesh parameters file.
`transfinite_faces` set to 1 meshes every 4-sided face as a structured triangle grid when the edge
meshing already put the same number of segments on its opposite sides; `transfinite_angle` (135 by
default, as in Gmsh) is the angle below which the edges meeting at a vertex make a corner, so a face
with a flatter corner is not 4-sided. The interior nodes are placed by transfinite interpolation in
the parameter space of the face, and those faces skip the surface optimisation so the grid stays
structured. A face that does not qualify, or whose grid would come out folded, is meshed as before.
To make faces qualify, the edge meshing links the opposite edges of every face bounded by exactly four
edges into chains across neighbouring faces, and divides every edge of a chain into the largest
segment count in it, spaced as the local mesh size would space them. A chain that would make one of
its edges more than about twice as fine as its mesh size asks for is left as it is. A face whose
sides are made of several edges is not matched, and is structured only when its counts happen to
agree.

`BREP_VISUALIZATION` writes the version 2 `.vis` format. It defines solids, shells, faces, edges, and vertices with their
CAD properties before a separate tessellation section. The extensible line format is documented in
[VIS_GEOMETRY_METADATA.md](VIS_GEOMETRY_METADATA.md).

## Building

Both solutions build **x64**, and nglib must be built first because the front end links `nglib.lib`:

    MSBuild "Netgen v5.3.1 port to OCCT 8.0.1 vs2026\Netgen-master\netgen-5.3.1\windows\nglib.sln" -t:Build "-p:Configuration=Release(OCC)" -p:Platform=x64
    MSBuild "NetGenMesher 8.0.1 vs2026\NetGenLibTest.sln" -t:Build -p:Configuration=Release -p:Platform=x64

Both write to `NetgenMesher\Release`. The nglib post build event syncs the OpenCASCADE, runtime,
pthreads and zlib dlls next to them, so that folder is the complete runtime set.

To deploy into PrePoMax, copy `NetGenMesher.exe` and `nglib.dll` from there over
`ThirdParty\netgen` in the PrePoMax repository and commit them - they are a matched pair and should
move together. The PrePoMax post build event copies that folder into `Meshers\Netgen`.

## What is not in this repository

Only sources and project files are tracked. These have to be restored by hand into a fresh clone
before it can build, because they are large and change only when the dependency is upgraded:

- `opencascade-8.0.1/` - an OpenCASCADE 8.0.1 build with the image, video, visualisation and
  parallel runtime dependencies disabled. **One copy is needed in each of the two solution
  folders**, since both resolve it relative to their own `$(SolutionDir)`.
- `pthreads-win32_prebuilt-dll-2-9-1-release/` and `zlib_win64/` - next to the netgen port's
  solution folder.
- `redist/` - the Visual C++ runtime dlls, at the repository root.

`_Old/` holds earlier versions of all of the above and is deliberately untracked.
