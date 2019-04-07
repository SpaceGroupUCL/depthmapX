#pragma once

#include "salalib/axialmap.h"
#include "salalib/shapemap.h"
#include "salalib/spacepixfile.h"
#include "genlib/comm.h"

namespace MapConverter {

std::unique_ptr<ShapeGraph> convertDrawingToAxial(Communicator *comm, const std::string& name,
                                                  const std::vector<SpacePixelFile> &drawingFiles);
std::unique_ptr<ShapeGraph> convertDataToAxial(Communicator *comm, const std::string& name,
                                               ShapeMap& shapemap, bool copydata = false);
std::unique_ptr<ShapeGraph> convertDrawingToConvex(Communicator *, const std::string& name,
                                                   const std::vector<SpacePixelFile> &drawingFiles);
std::unique_ptr<ShapeGraph> convertDataToConvex(Communicator *, const std::string& name,
                                                ShapeMap& shapemap, bool copydata = false);
std::unique_ptr<ShapeGraph> convertDrawingToSegment(Communicator *comm, const std::string& name,
                                                    const std::vector<SpacePixelFile> &drawingFiles);
std::unique_ptr<ShapeGraph> convertDataToSegment(Communicator *comm, const std::string& name,
                                                 ShapeMap& shapemap, bool copydata = false);
std::unique_ptr<ShapeGraph> convertAxialToSegment(Communicator *, ShapeGraph& axialMap,
                                                  const std::string& name, bool keeporiginal = true,
                                                  bool pushvalues = false, double stubremoval = 0.0);

}
