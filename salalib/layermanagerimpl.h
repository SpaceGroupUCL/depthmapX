// Copyright (C) 2017 Christian Sailer

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

#pragma once
#include "layermanager.h"
#include <map>
#include <vector>

class LayerManagerImpl : public LayerManager
{


    // LayerManager interface
public:
    LayerManagerImpl();
    virtual size_t addLayer(const std::string &layerName);
    virtual const std::string& getLayerName(size_t index) const;
    virtual size_t getLayerIndex(const std::string &layerName) const;
    virtual void setLayerVisible(size_t layerIndex, bool visible);
    virtual bool isLayerVisible(size_t layerIndex) const;

    virtual KeyType getKey(size_t layerIndex) const;
    virtual bool isVisible(const KeyType &key) const;

    virtual void read(std::istream& stream);
    virtual void write(std::ostream& stream ) const;

private:
    void checkIndex(size_t index) const;

private:
    int64_t m_visibleLayers;
    std::vector<std::string> m_layers;
    std::map<std::string, size_t> m_layerLookup;


};

