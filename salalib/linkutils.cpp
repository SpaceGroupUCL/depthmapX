// Copyright (C) 2017 Petros Koutsolampros

// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.

// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.

#include "linkutils.h"
#include <sstream>

namespace depthmapX {
    std::vector<PixelRefPair> pixelateMergeLines(const std::vector<Line>& mergeLines, PointMap& currentMap)
    {
        std::vector<PixelRefPair> mergePixelPairs;

        std::vector<Line>::const_iterator iter = mergeLines.begin(), end =
        mergeLines.end();
        for ( ; iter != end; ++iter )
        {
            const Line & mergeLine = *iter;
            const PixelRef & a = currentMap.pixelate(mergeLine.start(), false);
            const PixelRef & b = currentMap.pixelate(mergeLine.end(), false);

            mergePixelPairs.push_back(PixelRefPair(a, b));
        }
        return mergePixelPairs;
    }

    void mergePixelPairs(const std::vector<PixelRefPair>& links, PointMap& currentMap) {

        // check if any link pixel already exists on the map
        std::vector<PixelRefPair>::const_iterator iter = links.begin(), end =
        links.end();
        for ( ; iter != end; ++iter )
        {
            const PixelRefPair link = *iter;

            // check in limits:
            if (!currentMap.includes(link.a) || !currentMap.getPoint(link.a).filled()
                    || !currentMap.includes(link.b) || !currentMap.getPoint(link.b).filled())
            {
                std::stringstream message;
                message << "Line ends not both on painted analysis space (index: "
                        << (iter - links.begin() + 1)
                        << ")" << std::flush;
                throw depthmapX::InvalidLinkException(message.str().c_str());
            }

            // check if we were given coordinates that fall on a previously
            // merged cell, in which case the newest given will replace the
            // oldest and effectively delete the whole link
            if(currentMap.isPixelMerged(link.a)
                    || currentMap.isPixelMerged(link.b))
            {
                // one of the cells is already on the map
                std::stringstream message;
                message << "Link pixel found that is already linked on the map (index: "
                        << (iter - links.begin() + 1)
                        << ")"
                        << std::flush;
                throw depthmapX::InvalidLinkException(message.str().c_str());
            }

            // also check if given links have overlapping pixels:
            std::vector<PixelRefPair>::const_iterator prevIter = links.begin();
            for ( ; prevIter != iter; ++prevIter )
            {
                const PixelRefPair prevLink = *prevIter;

                // PixelRefPair internal == operator only checks a with a and b with b
                // but we also need to check the inverse
                if(link.a == prevLink.a
                        || link.b == prevLink.b
                        || link.a == prevLink.b
                        || link.b == prevLink.a)
                {
                    // one of the cells has already been seen.
                    std::stringstream message;
                    message << "Overlapping link found (index: "
                            << (iter - links.begin() + 1)
                            << ")" << std::flush;
                    throw depthmapX::InvalidLinkException(message.str().c_str());
                }
            }
        }
        std::for_each(links.begin(), links.end(),
                      [&](const PixelRefPair &pair)->void{ currentMap.mergePixels(pair.a,pair.b); });
    }

    std::vector<SimpleLine> getMergedPixelsAsLines(PointMap& currentMap)
    {
        std::vector<SimpleLine> mergedPixelsAsLines;
        std::vector<std::pair<PixelRef, PixelRef>> mergedPixelPairs = currentMap.getMergedPixelPairs();

        std::vector<std::pair<PixelRef, PixelRef>>::const_iterator iter = mergedPixelPairs.begin(), end =
        mergedPixelPairs.end();
        for ( ; iter != end; ++iter )
        {
            std::pair<PixelRef, PixelRef> pixelPair = *iter;
            mergedPixelsAsLines.push_back(SimpleLine(
                                              currentMap.depixelate(pixelPair.first),
                                              currentMap.depixelate(pixelPair.second)
                                              ));
        }
        return mergedPixelsAsLines;
    }
}
