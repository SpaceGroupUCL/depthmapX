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

#include "attributetable.h"
#include "attributetableindex.h"

class AttributeTableView
{
public:
    AttributeTableView(const dXreimpl::AttributeTable& table );

    const dXreimpl::AttributeTable &m_table;

    // columnIndex < 0 -> not set
    virtual void setDisplayColIndex(int columnIndex);
    int getDisplayColIndex() const{ return m_displayColumn;}

    float getNormalisedValue(const dXreimpl::AttributeKey& key, const dXreimpl::AttributeRow &row) const;
    const DisplayParams& getDisplayParams() const;

    typedef std::vector<dXreimpl::ConstAttributeIndexItem> ConstIndex;
    const ConstIndex& getConstTableIndex() const{return m_index;}

    const dXreimpl::AttributeColumn& getDisplayedColumn() const;

private:
    ConstIndex m_index;
    int m_displayColumn;
};

class AttributeTableHandle : public AttributeTableView
{
public:
    AttributeTableHandle(dXreimpl::AttributeTable &table) : m_mutableTable(table), AttributeTableView(table){}
    typedef std::vector<dXreimpl::AttributeIndexItem> Index;
    const Index& getTableIndex() const {return m_mutableIndex;}
    virtual void setDisplayColIndex(int columnIndex);
private:
    dXreimpl::AttributeTable& m_mutableTable;
    Index m_mutableIndex;

};


