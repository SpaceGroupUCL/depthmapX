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
#include <genlib/readwritehelpers.h>

#include <sstream>
#include <numeric>

const std::string &AttributeColumnImpl::getName() const
{
    return m_name;
}

bool AttributeColumnImpl::isLocked() const
{
    return m_locked;
}

void AttributeColumnImpl::setLock(bool lock)
{
    m_locked = lock;
}

bool AttributeColumnImpl::isHidden() const
{
    return m_hidden;
}

void AttributeColumnImpl::setHidden(bool hidden)
{
    m_hidden = hidden;
}

void AttributeColumnImpl::setFormula(std::string newFormula)
{
    m_formula = newFormula;
}

const std::string &AttributeColumnImpl::getFormula() const
{
    return m_formula;
}

const AttributeColumnStats &AttributeColumnImpl::getStats() const
{
    return m_stats;
}

void AttributeColumnImpl::updateStats(float val, float oldVal) const
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

void AttributeColumnImpl::setName(const std::string &name)
{
    m_name = name;
}

size_t AttributeColumnImpl::read(std::istream &stream)
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

void AttributeColumnImpl::write(std::ostream &stream, int physicalCol)
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
float AttributeRowImpl::getValue(const std::string &column) const
{
    return getValue(m_colManager.getColumnIndex(column));
}

float AttributeRowImpl::getValue(size_t index) const
{
    checkIndex(index);
    return m_data[index];
}

float AttributeRowImpl::getNormalisedValue(size_t index) const
{
    checkIndex(index);
    auto& colStats = m_colManager.getColumn(index).getStats();
    if (colStats.max == colStats.min)
    {
        return 0.5f;
    }
    return  m_data[index] < 0 ? -1.0f : float((m_data[index] - colStats.min)/(colStats.max - colStats.min));
}

AttributeRow& AttributeRowImpl::setValue(const std::string &column, float value)
{
    return setValue(m_colManager.getColumnIndex(column), value);
}

AttributeRow& AttributeRowImpl::setValue(size_t index, float value)
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

AttributeRow& AttributeRowImpl::setSelection(bool selected)
{
    m_selected = selected;
    return *this;
}

bool AttributeRowImpl::isSelected() const
{
    return m_selected;
}

void AttributeRowImpl::addColumn()
{
    m_data.push_back(-1);
}

void AttributeRowImpl::removeColumn(size_t index)
{
    checkIndex(index);
    m_data.erase(m_data.begin() + index);
}

void AttributeRowImpl::read(std::istream &stream)
{
    stream.read((char *)&m_layerKey, sizeof(m_layerKey));
    dXreadwrite::readIntoVector(stream, m_data);
}

void AttributeRowImpl::write(std::ostream &stream)
{
    stream.write((char *)&m_layerKey, sizeof(m_layerKey));
    dXreadwrite::writeVector(stream, m_data);
}

void AttributeRowImpl::checkIndex(size_t index) const
{
    if( index >= m_data.size())
    {
        throw std::out_of_range("AttributeColumn index out of range");
    }
}



AttributeRow &AttributeRowImpl::incrValue(size_t index, float value)
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


AttributeRow &AttributeRowImpl::incrValue(const std::string &colName, float value)
{
    return incrValue(m_colManager.getColumnIndex(colName), value);
}

AttributeRow &AttributeTable::getRow(const AttributeKey &key)
{
    auto* row = getRowPtr(key);
    if (row == 0)
    {
        throw std::out_of_range("Invalid row key");
    }
    return *row;
}

const AttributeRow& AttributeTable::getRow(const AttributeKey &key) const
{
    auto* row = getRowPtr(key);
    if (row == 0)
    {
        throw std::out_of_range("Invalid row key");
    }
    return *row;
}

AttributeRow *AttributeTable::getRowPtr(const AttributeKey &key)
{
    auto iter = m_rows.find(key);
    if (iter == m_rows.end())
    {
        return 0;
    }
    return iter->second.get();
}

const AttributeRow *AttributeTable::getRowPtr(const AttributeKey &key) const
{
    auto iter = m_rows.find(key);
    if (iter == m_rows.end())
    {
        return 0;
    }
    return iter->second.get();
}

AttributeRow &AttributeTable::addRow(const AttributeKey &key)
{
    auto iter = m_rows.find(key);
    if (iter != m_rows.end())
    {
        throw new std::invalid_argument("Duplicate key");
    }
    auto res = m_rows.insert(std::make_pair(key, std::unique_ptr<AttributeRowImpl>(new AttributeRowImpl(*this))));
    return *res.first->second;
}

void AttributeTable::removeRow(const AttributeKey &key)
{
    auto iter = m_rows.find(key);
    if (iter == m_rows.end())
    {
        throw new std::invalid_argument("Row does not exist");
    }
    m_rows.erase(iter);
}

AttributeColumn &AttributeTable::getColumn(size_t index)
{
    if(index == size_t(-1)) {
        return m_keyColumn;
    }
    checkColumnIndex(index);
    return m_columns[index];
}

size_t AttributeTable::insertOrResetColumn(const std::string &columnName, const std::string &formula)
{
    auto iter = m_columnMapping.find(columnName);
    if (iter == m_columnMapping.end())
    {
        return addColumnInternal(columnName, formula);
    }

    // it exists - we need to reset it
    m_columns[iter->second].m_stats = AttributeColumnStats();
    m_columns[iter->second].setLock(false);
    for (auto& row : m_rows)
    {
        row.second->setValue(iter->second, -1.0f);
    }
    return iter->second;
}

size_t AttributeTable::insertOrResetLockedColumn(const std::string &columnName, const std::string &formula)
{
    size_t index = insertOrResetColumn(columnName, formula);
    m_columns[index].setLock(true);
    return index;
}

size_t AttributeTable::getOrInsertColumn(const std::string &columnName, const std::string &formula)
{
    auto iter = m_columnMapping.find(columnName);
    if ( iter != m_columnMapping.end())
    {
        return iter->second;
    }
    return addColumnInternal(columnName, formula);
}

size_t AttributeTable::getOrInsertLockedColumn(const std::string &columnName, const std::string &formula)
{
    size_t index = getOrInsertColumn(columnName, formula);
    m_columns[index].setLock(true);
    return index;
}

void AttributeTable::removeColumn(size_t colIndex)
{
    checkColumnIndex(colIndex);
    const std::string& name = m_columns[colIndex].getName();
    auto iter = m_columnMapping.find(name);
    m_columnMapping.erase(iter);
    for (auto& elem : m_columnMapping)
    {
        if (elem.second > colIndex)
        {
            elem.second--;
        }
    }
    m_columns.erase(m_columns.begin()+colIndex);
    for (auto& row : m_rows)
    {
        row.second->removeColumn(colIndex);
    }
}

void AttributeTable::renameColumn(const std::string &oldName, const std::string &newName)
{
    auto iter = m_columnMapping.find(oldName);
    if (iter == m_columnMapping.end())
    {
        throw std::out_of_range("Invalid column name");
    }

    size_t colIndex = iter->second;
    m_columns[colIndex].setName(newName);
    m_columnMapping.erase(iter);
    m_columnMapping[newName] = colIndex;

}

void AttributeTable::deselectAllRows()
{
    for (auto& row : m_rows)
    {
        row.second->setSelection(false);
    }
}

void AttributeTable::setDisplayParamsForAllAttributes(const DisplayParams &params)
{
    for (auto& col: m_columns)
    {
        col.setDisplayParams(params);
    }

}

void AttributeTable::read(std::istream &stream, LayerManager &layerManager)
{
    layerManager.read(stream);
    int colcount;
    stream.read((char *)&colcount, sizeof(colcount));
    std::map<size_t, AttributeColumnImpl> tmp;
    for (int j = 0; j < colcount; j++) {
        AttributeColumnImpl col("");
        tmp[col.read(stream)] = col;
    }

    for (auto & c : tmp)
    {
        m_columnMapping[c.second.getName()] = m_columns.size();
        m_columns.push_back(c.second);
    }

    int rowcount, rowkey;
    stream.read((char *)&rowcount, sizeof(rowcount));
    for (int i = 0; i < rowcount; i++) {
        stream.read((char *)&rowkey, sizeof(rowkey));
        auto row = std::unique_ptr<AttributeRowImpl>(new AttributeRowImpl(*this));
        row->read(stream);
        m_rows.insert(std::make_pair(AttributeKey(rowkey),std::move(row)));
    }

    // ref column display params
    stream.read((char *)&m_displayParams,sizeof(DisplayParams));
}

void AttributeTable::write(std::ostream &stream, const LayerManager &layerManager)
{
    layerManager.write(stream);
    int colCount = (int)m_columns.size();
    stream.write((char *)&colCount, sizeof(int));

    // TODO: For binary compatibility write the columns in alphabetical order
    // but the physical columns in the order inserted

    std::vector<size_t> indices(m_columns.size());
    std::iota(indices.begin(), indices.end(), static_cast<size_t>(0));

    std::sort(indices.begin(), indices.end(),
        [&](size_t a, size_t b) {
        return m_columns[a].getName() < m_columns[b].getName();
    });

    for (int idx: indices) {
        m_columns[idx].write(stream, m_columnMapping[m_columns[idx].getName()]);
    }

    int rowcount = (int)m_rows.size();
    stream.write((char *)&rowcount, sizeof(int));
    for ( auto &kvp : m_rows)
    {
        kvp.first.write(stream);
        kvp.second->write(stream);
    }
    stream.write((const char *)&m_displayParams, sizeof(DisplayParams));
}

void AttributeTable::clear() {
    m_rows.clear();
    m_columns.clear();
    m_columnMapping.clear();
}

size_t AttributeTable::getColumnIndex(const std::string &name) const
{
    auto iter = m_columnMapping.find(name);
    if (iter == m_columnMapping.end())
    {
        std::stringstream message;
        message << "Unknown column name " << name;
        throw std::out_of_range(message.str());
    }
    return iter->second;

}

// TODO: Compatibility. Method to retreive a column's index
// if the set of columns was sorted
size_t AttributeTable::getColumnSortedIndex(size_t index) const
{
    if(index == size_t(-1) || index == size_t(-2)) return index;
    if(index >= m_columns.size()) return -1;

    return std::distance(m_columnMapping.begin(), m_columnMapping.find(getColumnName(index)));
}


const AttributeColumn &AttributeTable::getColumn(size_t index) const
{
    if(index == size_t(-1)) {
        return m_keyColumn;
    }
    checkColumnIndex(index);
    return m_columns[index];
}

const std::string &AttributeTable::getColumnName(size_t index) const
{
    return getColumn(index).getName();
}

size_t AttributeTable::getNumColumns() const
{
    return m_columns.size();
}

bool AttributeTable::hasColumn(const std::string &name) const
{
    auto iter = m_columnMapping.find(name);
    return(iter != m_columnMapping.end());
}

void AttributeTable::checkColumnIndex(size_t index) const
{
    if (index >= m_columns.size())
    {
        throw std::out_of_range("ColumnIndex out of range");
    }
}

size_t AttributeTable::addColumnInternal(const std::string &name, const std::string &formula)
{
    size_t colIndex = m_columns.size();
    m_columns.push_back(AttributeColumnImpl(name, formula));
    m_columnMapping[name] = colIndex;
    for (auto& elem : m_rows)
    {
        elem.second->addColumn();
    }
    return colIndex;
}
