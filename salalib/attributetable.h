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
#include <string>
#include <map>
#include <vector>
#include <memory>
#include <sstream>
#include <iterator>
#include <algorithm>
#include <salalib/displayparams.h>
#include <salalib/mgraph_consts.h>


///
/// Interface to an attribute row
///
class AttributeRow : public LayerAware
{
public:
    virtual float getValue(const std::string &column) const = 0;
    virtual float getValue(size_t index) const = 0;
    virtual float getNormalisedValue(size_t index) const = 0;
    virtual AttributeRow& setValue(const std::string &column, float value ) = 0;
    virtual AttributeRow& setValue(size_t index, float value) = 0;
    virtual AttributeRow& incrValue(size_t index, float value = 1.0f) = 0;
    virtual AttributeRow& incrValue(const std::string &colName, float value = 1.0f) = 0;
    virtual AttributeRow& setSelection(bool selected) = 0;
    virtual bool isSelected() const = 0;

    virtual ~AttributeRow(){}
};


///
/// Container for attribute column stats - really just POD to pass them around
///
struct AttributeColumnStats
{
    AttributeColumnStats( double minimum, double maximum, double tot, double vTot, double vMin, double vMax ): min(minimum), max(maximum), total(tot), visibleTotal(vTot), visibleMin(vMin), visibleMax(vMax)
    {}

    AttributeColumnStats() : AttributeColumnStats(-1.0, -1.0, -1.0, -1.0, -1.0, -1.0)
    {
    }

    double min;
    double max;
    double total;
    double visibleTotal;
    double visibleMin;
    double visibleMax;
};


///
/// Interface to an attribute column
///
class AttributeColumn
{
public:
    virtual const std::string& getName() const  = 0;
    virtual bool isLocked() const = 0;
    virtual void setLock(bool lock) = 0;
    virtual bool isHidden() const = 0;
    virtual void setHidden(bool hidden) = 0;
    virtual void setDisplayParams(const DisplayParams& params ) = 0;
    virtual const DisplayParams& getDisplayParams() const = 0;

    virtual const std::string& getFormula() const = 0;
    virtual void setFormula(std::string newFormula) = 0;

    virtual const AttributeColumnStats& getStats() const = 0;

    // stats are mutable - we need to be able to update them all the time,
    // even when not allowed to modify the column settings
    virtual void updateStats(float val, float oldVal = 0.0f) const = 0;

    virtual ~AttributeColumn(){}

};


///
/// Interface to an Attribute Column Manager
/// This handles the mapping from column name to index and actually storing the column implementations
/// Implemented by AttributeTable
///
class AttributeColumnManager
{
public:
    virtual size_t getNumColumns() const = 0;
    virtual size_t getColumnIndex(const std::string& name) const = 0;
    virtual const AttributeColumn& getColumn(size_t index) const = 0;
    virtual const std::string& getColumnName(size_t index) const = 0;
};



// Implementation of AttributeColumn

class AttributeColumnImpl: public AttributeColumn, AttributeColumnStats
{
    // AttributeColumn interface
public:
    AttributeColumnImpl(const std::string &name, const std::string &formula = std::string()) : m_name(name), m_locked(false), m_hidden(false), m_formula(formula)
    {
    }

    AttributeColumnImpl() : m_locked(false), m_hidden(false)
    {}
    virtual const std::string &getName() const;
    virtual bool isLocked() const;
    virtual void setLock(bool lock);
    virtual bool isHidden() const;
    virtual void setHidden(bool hidden);
    virtual const std::string &getFormula() const;
    virtual void setFormula(std::string newFormula);
    virtual const AttributeColumnStats& getStats() const;
    virtual void setDisplayParams(const DisplayParams &params){ m_displayParams = params; }
    virtual const DisplayParams &getDisplayParams() const{return m_displayParams;}

    virtual void updateStats(float val, float oldVal = 0.0f) const;

public:
    // stats are mutable - we need to be able to update them all the time,
    // even when not allowed to modify the column settings
    mutable AttributeColumnStats m_stats;

    void setName(const std::string &name);
    // returns the physical column for comaptibility with the old attribute table
    size_t read(std::istream &stream);
    void write(std::ostream& stream, int physicalCol);

private:
    std::string m_name;
    bool m_locked;
    bool m_hidden;
    std::string m_formula;
    DisplayParams m_displayParams;
};

// Implementation of AttributeColumn that actually links to the keys of the table

class KeyColumn : public AttributeColumnImpl
{
public:
    KeyColumn() : AttributeColumnImpl(), m_name("Ref")
    {}
private:
    std::string m_name;
};


// Implementation of AttributeRow
class AttributeRowImpl : public AttributeRow
{
public:
    AttributeRowImpl(const AttributeColumnManager& colManager) : m_data(colManager.getNumColumns(), -1.0), m_colManager(colManager), m_selected(false)
    {
        m_layerKey = 1;
    }

    // AttributeRow interface
public:
    virtual float getValue(const std::string &column) const;
    virtual float getValue(size_t index) const;
    virtual float getNormalisedValue(size_t index) const;
    virtual AttributeRow& setValue(const std::string &column, float value);
    virtual AttributeRow& setValue(size_t index, float value);
    virtual AttributeRow& incrValue(const std::string &column, float value);
    virtual AttributeRow& incrValue(size_t index, float value);
    virtual AttributeRow& setSelection(bool selected);
    virtual bool isSelected() const;

    void addColumn();
    void removeColumn(size_t index);

    void read(std::istream &stream);
    void write(std::ostream &stream);

private:
    std::vector<float> m_data;
    const AttributeColumnManager& m_colManager;
    bool m_selected;

    void checkIndex(size_t index) const;

};

///
/// \brief Small struct to make an attribute key distinguishable from an int
/// PixelRefs are serialised into an int (2 bytes x, 2 bytes y) for historic reason. This seems dangerous
/// and confusing as these are by no means indices, but look the same to the compiler and the reader.
/// This struct should disambiguate this...
///
struct AttributeKey
{
    explicit AttributeKey(int val) : value(val)
    {}
    int value;

    bool operator < (const AttributeKey& other ) const
    {
        return value < other.value;
    }

    void write(std::ostream &stream) const
    {
        stream.write((char *)&value, sizeof(int));
    }

    void read(std::istream &stream)
    {
        stream.read((char *)&value, sizeof(int));
    }
};

///
/// AttributeTable
///
class AttributeTable : public AttributeColumnManager
{
    // AttributeTable "interface" - the actual table handling
public:
    AttributeTable(){}
    virtual ~AttributeTable(){}
    AttributeTable(AttributeTable&&) = default;
    AttributeTable& operator =(AttributeTable&&) = default;
    AttributeTable(const AttributeTable& ) = delete;
    AttributeTable& operator =(const AttributeTable&) = delete;

    ///
    /// \brief Get a row reference
    /// \param key of the row
    /// \return reference to row, throws if key not found
    ///
    AttributeRow& getRow(const AttributeKey& key );

    ///
    /// \brief Get a row const reference
    /// \param key of the row
    /// \return const reference to row, throws if key not found
    ///
    const AttributeRow& getRow(const AttributeKey& key) const;

    ///
    /// \brief Get a row pointer
    /// \param key of the row
    /// \return pointer to row, null if key not found
    ///
    AttributeRow* getRowPtr(const AttributeKey& key);

    ///
    /// \brief Get a row const pointer
    /// \param key of the row
    /// \return const pointer to row, null if key not found
    ///
    const AttributeRow* getRowPtr(const AttributeKey& key)const;
    AttributeRow &addRow(const AttributeKey& key);
    AttributeColumn& getColumn(size_t index);
    size_t insertOrResetColumn(const std::string& columnName, const std::string &formula = std::string());
    size_t insertOrResetLockedColumn(const std::string& columnName, const std::string &formula = std::string());
    size_t getOrInsertColumn(const std::string& columnName, const std::string &formula = std::string());
    size_t getOrInsertLockedColumn(const std::string& columnName, const std::string &formula = std::string());
    void removeRow(const AttributeKey& key);
    void removeColumn(size_t colIndex);
    void renameColumn(const std::string& oldName, const std::string& newName);
    size_t getNumRows() const { return m_rows.size(); }
    void deselectAllRows();
    const DisplayParams& getDisplayParams() const { return m_displayParams; }
    void setDisplayParams(const DisplayParams& params){m_displayParams = params;}
    void setDisplayParamsForAllAttributes(const DisplayParams& params);
    void read(std::istream &stream, LayerManager &layerManager);
    void write(std::ostream &stream, const LayerManager &layerManager);
    void clear();
    float getSelAvg(size_t columnIndex) {
        float selTotal = 0;
        int selNum = 0;
        for(auto& pair: m_rows) {
            if(pair.second->isSelected()) {
                selTotal += pair.second->getValue(columnIndex);
                selNum++;
            }
        }
        if(selNum == 0) {
            return(-1);
        }
        return(selTotal/selNum);
    }

// interface AttributeColumnManager
public:
    virtual size_t getColumnIndex(const std::string& name) const;
    virtual const AttributeColumn& getColumn(size_t index) const;
    virtual const std::string& getColumnName(size_t index) const;
    virtual size_t getNumColumns() const;
    virtual bool hasColumn(const std::string &name) const;

    // TODO: Compatibility. Very inefficient method to retreive a column's index
    // if the set of columns was sorted
    size_t getColumnSortedIndex(size_t index) const;

private:
    typedef std::map<AttributeKey, std::unique_ptr<AttributeRowImpl>> StorageType;
    StorageType m_rows;
    std::map<std::string, size_t> m_columnMapping;
    std::vector<AttributeColumnImpl> m_columns;
    KeyColumn m_keyColumn;
    DisplayParams m_displayParams;

private:
    void checkColumnIndex(size_t index) const;
    size_t addColumnInternal(const std::string &name, const std::string &formula);

// warning - here be dragons!
// This is the implementation of stl style iterators on attribute table, allowing efficient
// iteration of rows without resorting to log(n) access via the map


public:
    ///
    /// \brief The iterator_item class
    /// The value of an iterator over the table - we want to hide the actual storage details and just
    /// return references to rows and keys.
    class iterator_item
    {
    public:
        virtual const AttributeKey& getKey() const = 0;
        virtual const AttributeRow& getRow() const = 0;
        virtual AttributeRow& getRow() = 0;
        virtual ~iterator_item(){}
    };
private:
    // implementation of the iterator_item, templated on iterator type to allow const and non-const iterator
    template <typename iterator_type>
    class iterator_item_impl : public iterator_item
    {
    public:
        iterator_item_impl( const iterator_type & iter) : m_iter(iter)
        {}
        template<typename other_type> iterator_item_impl(const iterator_item_impl<other_type>& other) : m_iter(other.m_iter)
        {}

        template<typename other_type> iterator_item_impl<iterator_type>& operator = (const iterator_item_impl<other_type>& other)
        {
            m_iter = other.m_iter;
            return *this;
        }


        const AttributeKey& getKey() const
        {
            return m_iter->first;
        }

        const AttributeRow& getRow() const
        {
            return *m_iter->second;
        }

       AttributeRow& getRow()
       {
           return *m_iter->second;
       }

        void forward() const
        {
            ++m_iter;
        }

        void back() const
        {
            --m_iter;
        }

        template<typename other_type> bool operator == (const iterator_item_impl<other_type> &other) const
        {
            return m_iter == other.m_iter;
        }
    public:
        mutable iterator_type m_iter;
    };


    // iterator implementation - templated on iterator type for const/non-const
    template <typename iterator_type>
    class const_iterator_impl : public std::iterator<std::bidirectional_iterator_tag, iterator_item>
    {
        template<typename other_type> friend class const_iterator_impl;
    public:
        const_iterator_impl( const iterator_type& iter) : m_item(iter)
        {}
        template<typename other_type> const_iterator_impl(const const_iterator_impl<other_type>& other) : m_item(other.m_item)
        {}
        template<typename other_type> const_iterator_impl& operator =(const const_iterator_impl<other_type> &other)
        {
            m_item = other.m_item;
            return *this;
        }

        const_iterator_impl& operator++() {m_item.forward();return *this;}
        const_iterator_impl operator++(int) {const_iterator_impl<iterator_type> tmp(*this); operator++(); return tmp;}
        const_iterator_impl& operator--() {m_item.back();return *this;}
        const_iterator_impl operator--(int) {const_iterator_impl<iterator_type> tmp(*this); operator--(); return tmp;}
        template<typename other_type> bool operator==(const const_iterator_impl<other_type>& rhs) const {return m_item == rhs.m_item;}
        template<typename other_type> bool operator!=(const const_iterator_impl<other_type>& rhs) const {return !(m_item==rhs.m_item);}
        const iterator_item& operator*() const {return m_item;}
        const iterator_item* operator->() const {return &m_item;}

    protected:
        iterator_item_impl<iterator_type> m_item;
    };

public:
    // const iterator is just a typedef on the impl
    typedef const_iterator_impl<typename StorageType::const_iterator> const_iterator;

    // non const iterator needs some extra methods
    class iterator : public const_iterator_impl<typename StorageType::iterator>
    {
    public:
        iterator(const typename StorageType::iterator& iter) : const_iterator_impl<typename StorageType::iterator>(iter)
        {}
        template<typename other_type> iterator(const const_iterator_impl<other_type>& other) : const_iterator_impl<StorageType::iterator>(other.item){
           // m_item = other.m_item;
        }
        template<typename other_type> iterator& operator =(const const_iterator_impl<other_type> &other)
        {
            const_iterator_impl<typename StorageType::iterator>::m_item = other.m_item;
            return *this;
        }
        iterator_item& operator*() {return const_iterator_impl<typename StorageType::iterator>::m_item;}
        iterator_item* operator->() {return &(const_iterator_impl<typename StorageType::iterator>::m_item);}
    };

    // stl style iteration methods
    const_iterator begin() const
    {
        return const_iterator(m_rows.begin());
    }

    iterator begin()
    {
        return iterator(m_rows.begin());
    }

    const_iterator end() const
    {
        return const_iterator(m_rows.end());
    }

    iterator end()
    {
        return iterator(m_rows.end());
    }

    iterator find(AttributeKey key)
    {
        return iterator(m_rows.find(key));
    }

    std::pair<const AttributeKey, std::unique_ptr<AttributeRowImpl>>& back()
    {
        return *m_rows.rbegin();
    }
};

