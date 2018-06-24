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


namespace dXreimpl
{
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
        size_t read(std::istream &stream, int version);
        void write(std::ostream& stream, int physicalCol);

    private:
        std::string m_name;
        bool m_locked;
        bool m_hidden;
        std::string m_formula;
        DisplayParams m_displayParams;
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

        void read(std::istream &stream, int version);
        void write(std::ostream &stream);

    private:
        std::vector<float> m_data;
        const AttributeColumnManager& m_colManager;
        bool m_selected;

        void checkIndex(size_t index) const;

    };

    ///
    /// \brief Small struct to make a serialised pixel ref distinguishable from an int
    /// PixelRefs are serialised into an int (2 bytes x, 2 bytes y) for historic reason. This seems dangerous
    /// and confusing as these are by no means inices, but look the same to the compiler and the reader.
    /// This struct should disambiguate this...
    ///
    struct SerialisedPixelRef
    {
        SerialisedPixelRef(int val) : value(val)
        {}
        int value;

        bool operator < (const SerialisedPixelRef& other ) const
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
    /// Templated on the RowKeyType so we can switch that out easily, I am not to happy with the current one...
    ///
    template<typename RowKeyType> class AttributeTable : public AttributeColumnManager
    {
        // AttributeTable "interface" - the actual table handling
    public:
        AttributeTable(){}
        AttributeTable(const AttributeTable& ) = delete;
        AttributeTable& operator =(const AttributeTable&) = delete;

        ///
        /// \brief Get a row reference
        /// \param key of the row
        /// \return reference to row, throws if key not found
        ///
        AttributeRow& getRow(const RowKeyType& key );

        ///
        /// \brief Get a row const reference
        /// \param key of the row
        /// \return const reference to row, throws if key not found
        ///
        const AttributeRow& getRow(const RowKeyType& key) const;

        ///
        /// \brief Get a row pointer
        /// \param key of the row
        /// \return pointer to row, null if key not found
        ///
        AttributeRow* getRowPtr(const RowKeyType& key);

        ///
        /// \brief Get a row const pointer
        /// \param key of the row
        /// \return const pointer to row, null if key not found
        ///
        const AttributeRow* getRowPtr(const RowKeyType& key)const;
        AttributeRow &addRow(const RowKeyType& key);
        AttributeColumn& getColumn(size_t index);
        size_t insertOrResetColumn(const std::string& columnName, const std::string &formula = std::string());
        size_t insertOrResetLockedColumn(const std::string& columnName, const std::string &formula = std::string());
        size_t getOrInsertColumn(const std::string& columnName, const std::string &formula = std::string());
        size_t getOrInsertLockedColumn(const std::string& columnName, const std::string &formula = std::string());
        void removeRow(const RowKeyType& key);
        void removeColumn(size_t colIndex);
        void renameColumn(const std::string& oldName, const std::string& newName);
        size_t getNumRows() const { return m_rows.size(); }
        void deselectAllRows();
        const DisplayParams& getDisplayParams() const { return m_displayParams; }
        void setDisplayParams(const DisplayParams& params){m_displayParams = params;}
        void setDisplayParamsForAllAttributes(const DisplayParams& params);
        void read(std::istream &stream, LayerManager &layerManager, int version);
        void write(std::ostream &stream, const LayerManager &layerManager);

   // interface AttributeColumnManager
    public:
        virtual size_t getColumnIndex(const std::string& name) const;
        virtual const AttributeColumn& getColumn(size_t index) const;
        virtual const std::string& getColumnName(size_t index) const;
        virtual size_t getNumColumns() const;

    private:
        typedef std::map<RowKeyType, std::unique_ptr<AttributeRowImpl>> StorageType;
        StorageType m_rows;
        std::map<std::string, size_t> m_columnMapping;
        std::vector<AttributeColumnImpl> m_columns;
        int64_t m_visibleLayers;
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
            virtual const RowKeyType& getKey() const = 0;
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


            const RowKeyType& getKey() const
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
            template<typename other_type> iterator(const const_iterator_impl<other_type>& other) : const_iterator_impl<typename StorageType::iterator>::m_item(other.m_item)
            {}
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
    };

// Below here is the implementation of AttributeTable methods - needs to be header only as it is templated.

    template<class RowKeyType>
    AttributeRow &AttributeTable<RowKeyType>::getRow(const RowKeyType &key)
    {
        auto* row = getRowPtr(key);
        if (row == 0)
        {
            throw std::out_of_range("Invalid row key");
        }
        return *row;
    }

    template<class RowKeyType>
    const AttributeRow& AttributeTable<RowKeyType>::getRow(const RowKeyType &key) const
    {
        auto* row = getRowPtr(key);
        if (row == 0)
        {
            throw std::out_of_range("Invalid row key");
        }
        return *row;
    }

    template<typename RowKeyType>
    AttributeRow *AttributeTable<RowKeyType>::getRowPtr(const RowKeyType &key)
    {
        auto iter = m_rows.find(key);
        if (iter == m_rows.end())
        {
            return 0;
        }
        return iter->second.get();
    }

    template<typename RowKeyType>
    const AttributeRow *AttributeTable<RowKeyType>::getRowPtr(const RowKeyType &key) const
    {
        auto iter = m_rows.find(key);
        if (iter == m_rows.end())
        {
            return 0;
        }
        return iter->second.get();
    }

    template<class RowKeyType>
    AttributeRow &AttributeTable<RowKeyType>::addRow(const RowKeyType &key)
    {
        auto iter = m_rows.find(key);
        if (iter != m_rows.end())
        {
            throw new std::invalid_argument("Duplicate key");
        }
        auto res = m_rows.insert(std::make_pair(key, std::unique_ptr<AttributeRowImpl>(new AttributeRowImpl(*this))));
        return *res.first->second;
    }

    template<typename RowKeyType>
    AttributeColumn &AttributeTable<RowKeyType>::getColumn(size_t index)
    {
        checkColumnIndex(index);
        return m_columns[index];
    }

    template<class RowKeyType>
    size_t AttributeTable<RowKeyType>::insertOrResetColumn(const std::string &columnName, const std::string &formula)
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

    template<class RowKeyType>
    size_t AttributeTable<RowKeyType>::insertOrResetLockedColumn(const std::string &columnName, const std::string &formula)
    {
        size_t index = insertOrResetColumn(columnName, formula);
        m_columns[index].setLock(true);
        return index;
    }

    template<class RowKeyType>
    size_t AttributeTable<RowKeyType>::getOrInsertColumn(const std::string &columnName, const std::string &formula)
    {
        auto iter = m_columnMapping.find(columnName);
        if ( iter != m_columnMapping.end())
        {
            return iter->second;
        }
        return addColumnInternal(columnName, formula);
    }

    template<class RowKeyType>
    size_t AttributeTable<RowKeyType>::getOrInsertLockedColumn(const std::string &columnName, const std::string &formula)
    {
        size_t index = getOrInsertColumn(columnName, formula);
        m_columns[index].setLock(true);
        return index;
    }

    template<class RowKeyType>
    void AttributeTable<RowKeyType>::removeColumn(size_t colIndex)
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

    template<class RowKeyType>
    void AttributeTable<RowKeyType>::renameColumn(const std::string &oldName, const std::string &newName)
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

    template<typename RowKeyType>
    void AttributeTable<RowKeyType>::deselectAllRows()
    {
        for (auto& row : m_rows)
        {
            row.second->setSelection(false);
        }
    }

    template<typename RowKeyType>
    void AttributeTable<RowKeyType>::setDisplayParamsForAllAttributes(const DisplayParams &params)
    {
        for (auto& col: m_columns)
        {
            col.setDisplayParams(params);
        }

    }

    template<typename RowKeyType>
    void AttributeTable<RowKeyType>::read(std::istream &stream, LayerManager &layerManager, int version)
    {
        layerManager.read(stream);
        int colcount;
        stream.read((char *)&colcount, sizeof(colcount));
        std::map<size_t, AttributeColumnImpl> tmp;
        for (int j = 0; j < colcount; j++) {
            AttributeColumnImpl col("");
            tmp[col.read(stream, METAGRAPH_VERSION)] = col;
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
           row->read(stream, METAGRAPH_VERSION);
           m_rows.insert(std::make_pair(rowkey,std::move(row)));
        }

       // ref column display params
       stream.read((char *)&m_displayParams,sizeof(DisplayParams));
    }

    template<typename RowKeyType>
    void AttributeTable<RowKeyType>::write(std::ostream &stream, const LayerManager &layerManager)
    {
        layerManager.write(stream);
        int colCount = (int)m_columns.size();
        stream.write((char *)&colCount, sizeof(int));
        for (size_t i = 0; i < m_columns.size(); ++i)
        {
            m_columns[i].write(stream, (int)i);
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

    template<class RowKeyType>
    size_t AttributeTable<RowKeyType>::getColumnIndex(const std::string &name) const
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

    template<class RowKeyType>
    const AttributeColumn &AttributeTable<RowKeyType>::getColumn(size_t index) const
    {
        checkColumnIndex(index);
        return m_columns[index];
    }

    template<class RowKeyType>
    const std::string &AttributeTable<RowKeyType>::getColumnName(size_t index) const
    {
        checkColumnIndex(index);
        return m_columns[index].getName();
    }

    template<class RowKeyType>
    size_t AttributeTable<RowKeyType>::getNumColumns() const
    {
        return m_columns.size();
    }

    template<class RowKeyType>
    void AttributeTable<RowKeyType>::checkColumnIndex(size_t index) const
    {
        if (index >= m_columns.size())
        {
            throw std::out_of_range("ColumnIndex out of range");
        }
    }

    template<class RowKeyType>
    size_t AttributeTable<RowKeyType>::addColumnInternal(const std::string &name, const std::string &formula)
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
}

