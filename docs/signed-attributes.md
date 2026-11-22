# Signed attributes in graph files

Ordinary `.graph` files continue to use version 440. Attribute rows are vectors of 32-bit floats, and `-1` means missing. Older depthmapX versions can open new ordinary saves with the same row layout, including any other finite negative values. A stored `-1` in a legacy file cannot be recovered as a number.

The table exposes `hasValue` so callers can distinguish missing cells from supported numbers, including negative values other than `-1`. A blank table cell displays a missing value. Editing a cell to `-1` or clearing it stores a missing value, as required by version 440.

If IsoGen needs to store numeric `-1` or additional map types, its save workflow can use a separate `.graph2` format with explicit value presence and a distinct format identifier. Ordinary depthmapX saves keep using `.graph`. The `.graph2` reader and writer belong with the IsoGen save workflow.

Column statistics are computed from the cells that have values. Cached totals and bounds from older files are rebuilt from rows on load. Table displays, attribute colours and scatter plots use presence checks so signed values contribute to the analysis. Missing cells stay out of statistics and plots and retain the existing missing-value colour.
