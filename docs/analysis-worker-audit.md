# Analysis worker audit

`RenderThread::run` catches C++ exceptions at the dispatch boundary. Qt delivers progress, update and redraw requests to the document thread. The document stays busy through worker exit, then releases the communicator, destroys the progress dialog and requests the final redraw.

| `CMSCommunicator` operations | Failure paths reviewed |
| --- | --- |
| `IMPORT`, `IMPORTMIF` | Drawing loader return codes and MapInfo-specific diagnostics. Import entry points are responsible for configuring the streams before starting the worker. |
| `MAKEPOINTS`, `MAKEGRAPH`, `ANALYSEGRAPH` | Fill and visibility graph construction can return failure. Analysis depends on a valid displayed point map. Existing failure and progress handling stays in the graph code. |
| `MAKEISOVIST`, `MAKEISOVISTSFROMFILE`, `MAKEISOVISTPATH` | Malformed list input has its own diagnostic. A multi-isovist run can add some shapes before a later operation fails or is cancelled. |
| `MAKEALLLINEMAP`, `MAKEFEWESTLINEMAP` | The selected seed or existing all-line map is a prerequisite. Map creation can fail after starting to change the document. |
| `MAKEDRAWING`, `MAKEUSERMAP`, `MAKEUSERMAPSHAPE`, `MAKEUSERSEGMAP`, `MAKEUSERSEGMAPSHAPE`, `MAKEGATESMAP`, `MAKECONVEXMAP`, `MAKESEGMENTMAP` | Conversion cases assume valid source layers and report empty input. Some can add an output layer or remove a source layer. |
| `MAKEBOUNDARYMAP` | The case is a no-op. |
| `AXIALANALYSIS`, `SEGMENTANALYSISTULIP`, `SEGMENTANALYSISANGULAR`, `TOPOMETANALYSIS` | The graph and analysis type must match. Legacy routines update attributes in place, so an exception can leave partial results. |
| `POINTDEPTH`, `METRICPOINTDEPTH`, `ANGULARPOINTDEPTH`, `TOPOLOGICALPOINTDEPTH` | Modes share the graph entry point but use different map types and selections. A stale selection or invalid internal index can bypass exception handling if the underlying access is unchecked. |
| `AGENTANALYSIS` | The worker preserves the specific diagnostic for missing isovist data. An agent run can produce part of its output before it stops. |
| `FROMCONNECTOR` | A missing analysis object now raises a diagnostic. Success flags are initialized and preserve the requested redraw level. The dispatcher catches exception types from the module. |

Phase 1 routes worker UI requests through Qt and guards module dispatch. Operations that temporarily clear layer flags restore their affected flags on exception exit, using the surviving layer collections. The save regression forces a built-in axial analysis to fail and verifies that its shape map and shapes survive a save and reload.

The state guard preserves layer visibility to the writer. An analysis that edits attributes or layers in place can still leave partial results. Rollback and validation of internal graph indices require per-operation tests of failed and cancelled runs.
