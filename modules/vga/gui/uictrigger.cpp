// This file is required to trigger cmake's AUTOUIC in the depthmapX directory
// if it is not included then the ui_*.h files in that directory are not generated
// and then not found by the module which is built first because it is a dependency
// of depthmapX.
#include "depthmapX/ui_ColourScaleDlg.h"
