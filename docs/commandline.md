# depthmapX Command Line Interface

## Overview
The depthmapX command line interface allows to run steps that are traditionally
executed by loading a file into the GUI and clicking buttons/menu entries
by running a command from a command line/shell.

The command line interface is especially useful in cases where a similar kind of
analysis needs to be repeated regularly, as
the steps can be written into a script and run automatically or 
semi-automatically, do not require user input via mouse/GUI and can be 
parallelised into multiple processes or on multiple machines.

The general idea is that each call to the command line interface reads in a
depthmapX graph file, then applies some transformation/analysis to it and saves
the result into a different file. Thus, a pipeline of operations can be
created where each step reads in the input of the previous step, and in the end
we have the end result and all intermediate steps.  This is particularly helpful
if something has gone wrong, as we can go back, figure out which step went wrong
and rerun this and all subsequent steps.

The command line interface also supports importing of data from DXF and csv 
formats and export to csv so that a whole pipeline from plan to statistical
output can be run programmatically.

## The Command Line

The functionality in the depthmapX cli is split up into execution modes - each
mode roughly represents a class of functionalities in the UI. The overall 
commandline looks like:

Mac/Linux: `./depthmapXcli -f <infile.graph> -o <outfile.graph> -m <mode> [mode 
options]`  
Windows: `depthmapXcli.exe -f <infile.graph> -o <outfile.graph> -m <mode>
[mode options]`

### Global option

These are command line parameters that can be used for any run of the 
application, no matter what the used mode is.

- `-m <mode>` This chooses the mode (what depthmapX operation to execute). 
Available modes are
  - `VGA` run visual graph analysis
  - `LINK` create extra links between pixels
  - `VISPREP` various operations to prepare a blank map for VGA or agent 
  analysis
  - `AXIAL` run axial analysis
  - `AGENTS` run an agent analysis
  - `ISOVIST` calculate isovists
  - `EXPORT` export data from the given graph file
  - `IMPORT` import data into a graph file
- `-f <filename>` input graph file to base the operation on
- `-o <output file>` graph file the result of the operation will be written to
- `-h` print a help text and exit
- `-s` enable simple mode (off by default)
- `-t <runtimes csv file>` enables dumping of the time used for various steps of
the processing into the specified file.

Each mode has a set of suboptions to tailor what exactly will we done.

### Mode options for `VGA`

- `-vm` Sets the VGA mode. This option must be set when using `VGA`. These 
reflect the options from the VGA submenu in
the depthmap UI. Possible options are
  - `isovist`
  - `visibility`
  - `metric`
  - `angular`
  - `thruvision`
- `-vg` Turn on global measures (optional). When set, `-vr` must be used to set
a visibility radius.
- `-vl` Turn on local measures (optional).
- `-vr <radius>` Set the visibility radius to a number between 1 and 99 steps.


### Mode options for `LINK`

Use this mode to add links to a grid before running a `VGA` analysis. It takes
tuples of coordinates and links the two closes pixels (as if using the link
tool in the graphical user interface - the coordinates represent the mouse 
positions when clicking). This mode either takes a list of tuples on the command
line, or it reads it in from a file

- `-lf <links file>` Reads in the link coordinates from a file.
- `-lnk <link coordinates> Defines one link to be made in the format 
`x1,y1,x2,y2` - e.g. to link from 0.0,0.0 to 1.0,2.3, specify 0.0,0.0,1.0,2.3.

The file format for the links file is 4 tabulator separated columns with headers
x1, x2, y1, and y2. So the file for the example above would be:
```
x1  y1  x2  y2
0.0 0.0 1.0 2.3
```

### Mode options for `VISPREP`

This mode can be used to run the preparation steps required after a map (dxf) 
has been imported into depthmapX, but before a VGA or agent analysis can be
run. It will define a grid, fill the graph, and run the connectivity 
calculation. It takes the following options
- `-pg <gridsize>` defines the grid size in units of your drawing
- `-pp <coordinate>` Defines a point where the fill algorithm starts in `x,y`
format. This option can be repeated if separate parts of the plan need to be
filled.
- `-pf <point file>` Reads the points for filling from the specified file (tab
separated, headers `X` and `Y`). This option cannot be combined with `-pp`
- `-pr <max visibility>` This restricts the visiblity in the connectivity 
calculation to the given value. The default value is unrestricted (`-1`)
- `-pb` Enables creating a boundary graph.

Example: `./depthmapXcli_macos -f gallery.graph -o gallery_prep.graph -m VISPREP
-pg 0.4 -pf 3.0,4.0 -pr 5`

### Mode options for `AXIAL`

This modes runs the various things that depthmapX can do around axial analysis.
It has two different kinds of options: command options that specify what to run,
and modifiers that impact how these are run. 

Each command options expects an input file that has the required inputs for 
these options (e.g. an axial analysis expects the graph file to contain an
axial map). The command options are
- `-xl <coordinate>` Calculate an all lines map starting at the point given in
`x,y` format.
- `-xf` reduce an all lines map to a fewest lines map. Expects an axial map
that can be reduced to be in the graph file.
- `-xu` Process unlink data (not yet supported on the command line)
- `-xa` run the actual axial analysis. Expects an axial map to be present in 
the graph file.

These command flags can be combined within one run, they will always run in the
order `-xl -xf -xu -xa` and each step can produce the input for the next.

In addition to these flags, the following modifiers are available
- `-xac` Include choice (betweenness) calculations
- `-xal` Include local measures
- `-xar` Include RA, RRA and total depth calculations


### Mode options for `AGENTS`
- `-am <agent mode>` one of
  - `standard`
  - `los-length` (Line of Sight length)
  - `occ-length` (Occluded length)
  - `occ-any` (Any occlusions)
  - `occ-group-45` (Occlusion group bins - 45 degrees)
  - `occ-group-60` (Occlusion group bins - 60 degrees)
  - `occ-furthest` (Furthest occlusion per bin)
  - `bin-far-dist` (Per bin far distance weighted)
  - `bin-angle` (Per bin angle weighted)
  - `bin-far-dist-angle` (Per bin far-distance and angle weighted)
  - `bin-memory` (Per bin memory)
- `-ats <timesteps>` set total system timesteps to be run
- `-arr <rate>` set agent release rate (likelyhood of release per timestep)
- `-atrails <no of trails>` record trails for this amount of agents (set to 0 
to record all, with max possible currently = 50)
- `-afov <fov>` set agent field-of-view (bins)
- `-asteps <steps>` set agent steps before turn decision
- `-alife <timesteps>` set agent total lifetime (in timesteps)
- `-alocseed <seed>` set agents to start at random locations with specific seed (0 to 10)
- `-alocfile <agent starting points file>`
- `-aloc <coordinates>` provide the single agent starting point in format 
`x1,y1` for example `-aloc 0.1,0.2`.
- `-ot <output type>` available output types (may use more than one
  - `graph` (graph file, default)
  - `gatecounts` (csv with cells of grid with gate counts)
  - `trails` (csv with lines showing path traversed by each agent)

### Mode options for `ISOVIST`
- `-ii <x,y[,angle,viewangle]>` Define an isoivist at position x,y with
    optional direction angle and view angle for partial isovists
- `-if <isovist file>` load isovist definitions from a file (csv)
    the relevant headers must be called x, y, angle and viewangle
    the latter two are optional.

Those two arguments cannot be mixed
Angles for partial isovists are in degrees, counted anti-clockwise with 0
pointing to the right.


### Mode options for `EXPORT`
This mode exports data from a graph file to a csv for further analysis in 
other tools, e.g. Excel or a statistics package. Thus, the output file (`-o` 
argument) should be a csv file, not a graph file in this mode.
- `-em <export mode>` one of
  - `pointmap-data-csv`
  - `pointmap-connections-csv`

### Mode options for `IMPORT`
The file provided by -f here will be used as the base. If that fileis not a 
graph, a new graph will be created and the file will be imported.
- `-if <file(s) to import>` one or more files to import

Example for importing a dxf:

`./depthmapXcli -f in.dxf -o out.graph`
