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


#include "attributetable.h"
#include "displayparams.h"
#include <genlib/stringutils.h>
#include <genlib/vectorhelpers.h>

#include <sstream>


const std::string &dXreimpl::AttributeColumnImpl::getName() const
{
    return m_name;
}

bool dXreimpl::AttributeColumnImpl::isLocked() const
{
    return m_locked;
}

void dXreimpl::AttributeColumnImpl::setLock(bool lock)
{
    m_locked = lock;
}

bool dXreimpl::AttributeColumnImpl::isHidden() const
{
    return m_hidden;
}

void dXreimpl::AttributeColumnImpl::setHidden(bool hidden)
{
    m_hidden = hidden;
}

const std::string &dXreimpl::AttributeColumnImpl::getFormula() const
{
    return m_formula;
}

const dXreimpl::AttributeColumnStats &dXreimpl::AttributeColumnImpl::getStats() const
{
    return m_stats;
}

void dXreimpl::AttributeColumnImpl::updateStats(float val, float oldVal) const
{
    if (m_stats.total < 0)
    {
        m_stats.total = val;
    }
    else
    {
        m_stats.total += val;
        m_stats.total -= oldVal;
    }
    if (val > m_stats.max)
    {
        m_stats.max = val;
    }
    if (m_stats.min < 0 || val < m_stats.min)
    {
        m_stats.min = val;
    }
}

void dXreimpl::AttributeColumnImpl::setName(const std::string &name)
{
    m_name = name;
}

size_t dXreimpl::AttributeColumnImpl::read(std::istream &stream, int version)
{
    m_name = dXstring::readString(stream);
    float val;
    stream.read((char *)&val, sizeof(float));
    m_stats.min = val;
    stream.read((char *)&val, sizeof(float));
    m_stats.max = val;
    stream.read((char *)&m_stats.total, sizeof(double));
    int physical_column;
    stream.read((char *)&physical_column, sizeof(int)); // physical column is obsolete
    stream.read((char *)&m_hidden, sizeof(bool));
    stream.read((char *)&m_locked, sizeof(bool));

    stream.read((char*)&m_displayParams,sizeof(DisplayParams));
    m_formula = dXstring::readString(stream);
    return physical_column;
}

void dXreimpl::AttributeColumnImpl::write(std::ostream &stream, int physicalCol)
{
    dXstring::writeString(stream, m_name);
    float min = (float)m_stats.min;
    float max = (float)m_stats.max;
    stream.write((char *)&min, sizeof(float));
    stream.write((char *)&max, sizeof(float));
    stream.write((char *)&m_stats.total, sizeof(m_stats.total));
    stream.write((char *)&physicalCol, sizeof(int));
    stream.write((char *)&m_hidden, sizeof(bool));
    stream.write((char *)&m_locked, sizeof(bool));
    stream.write((char *)&m_displayParams, sizeof(DisplayParams));
    dXstring::writeString(stream, m_formula);

}


// AttributeRow implementation
float dXreimpl::AttributeRowImpl::getValue(const std::string &column) const
{
    return getValue(m_colManager.getColumnIndex(column));
}

float dXreimpl::AttributeRowImpl::getValue(size_t index) const
{
    checkIndex(index);
    return m_data[index];
}

float dXreimpl::AttributeRowImpl::getNormalisedValue(size_t index) const
{
    checkIndex(index);
    auto& colStats = m_colManager.getColumn(index).getStats();
    if (colStats.max == colStats.min)
    {
        return 0.5f;
    }
    return  m_data[index] < 0 ? -1.0f : float((m_data[index] - colStats.min)/(colStats.max - colStats.min));
}

dXreimpl::AttributeRow& dXreimpl::AttributeRowImpl::setValue(const std::string &column, float value)
{
    return setValue(m_colManager.getColumnIndex(column), value);
}

dXreimpl::AttributeRow& dXreimpl::AttributeRowImpl::setValue(size_t index, float value)
{
    checkIndex(index);
    float oldVal = m_data[index];
    m_data[index] = value;
    if (oldVal < 0.0f)
    {
        oldVal = 0.0f;
    }
    m_colManager.getColumn(index).updateStats(value, oldVal);
    return *this;
}

dXreimpl::AttributeRow& dXreimpl::AttributeRowImpl::setSelection(bool selected)
{
    m_selected = selected;
    return *this;
}

bool dXreimpl::AttributeRowImpl::isSelected() const
{
    return m_selected;
}

void dXreimpl::AttributeRowImpl::addColumn()
{
    m_data.push_back(-1);
}

void dXreimpl::AttributeRowImpl::removeColumn(size_t index)
{
    checkIndex(index);
    m_data.erase(m_data.begin() + index);
}

void dXreimpl::AttributeRowImpl::read(std::istream &stream, int version)
{
    stream.read((char *)&m_layerKey, sizeof(m_layerKey));
    dXvector::readIntoVector(stream, m_data);
}

void dXreimpl::AttributeRowImpl::write(std::ostream &stream)
{
    stream.write((char *)&m_layerKey, sizeof(m_layerKey));
    dXvector::writeVector(stream, m_data);
}

void dXreimpl::AttributeRowImpl::checkIndex(size_t index) const
{
    if( index >= m_data.size())
    {
        throw std::out_of_range("AttributeColumn index out of range");
    }
}



dXreimpl::AttributeRow &dXreimpl::AttributeRowImpl::incrValue(size_t index, float value)
{
    checkIndex(index);
    float val = m_data[index];
    if ( val < 0)
    {
        setValue(index, value);
    }
    else
    {
        setValue(index, val + value);
    }
    return *this;
}


dXreimpl::AttributeRow &dXreimpl::AttributeRowImpl::incrValue(const std::string &colName, float value)
{
    return incrValue(m_colManager.getColumnIndex(colName), value);
}
