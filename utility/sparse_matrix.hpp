#ifndef SPARSE_MATRIX_HPP
#define SPARSE_MATRIX_HPP

namespace skylark { namespace utility {

/**
 *  This implements a very crude sparse matrix container only intended to hold
 *  local sparse matrices.
 *  Implementing a container rather than using e.g. CombBLAS SpMat, enables to
 *  user to have (local) sparse matrix support without the additional
 *  dependencies.
 */
template<typename IndexType, typename ValueType>
struct sparse_matrix_t {

    typedef IndexType index_type;
    typedef ValueType value_type;

    typedef typename std::vector<index_type>::const_iterator const_ind_itr_t;
    typedef std::pair<const_ind_itr_t, const_ind_itr_t> const_ind_itr_range_t;

    typedef typename std::vector<value_type>::const_iterator const_val_itr_t;
    typedef std::pair<const_val_itr_t, const_val_itr_t> const_val_itr_range_t;

    typedef tuple<index_type, index_type, value_type> coord_tuple_t;
    typedef std::vector<coord_tuple_t> coords_t;

    sparse_matrix_t()
        : _dirty(false)
    {}

    ~sparse_matrix_t() {}

    // expose size to allocate arrays for detach
    void Size(int *n_indptr, int *n_indices) const {

        *n_indptr  = _indptr.size();
        *n_indices = _indices.size();
    }

    // expose dirty flag to CAPI
    bool needsUpdate() const { return _dirty; }

    // expose to the CAPI layer assume memory has been allocated
    void Detach(int32_t *indptr, int32_t *indices, double *values) const {

        for(size_t i = 0; i < _indptr.size(); ++i)
            indptr[i] = static_cast<int32_t>(_indptr[i]);

        for(size_t i = 0; i < _indices.size(); ++i)
            indices[i] = static_cast<int32_t>(_indices[i]);

        for(size_t i = 0; i < _values.size(); ++i)
            values[i] = static_cast<double>(_values[i]);
    }

    // attach data from CAPI
    void Attach(int *indptr, int *indices, double *values,
                int n_indptr, int n_ind) {

        //XXX: we could use pointer for indptr array, indicies and values
        //     array only determined later by the nnz.
        _indptr.assign(indptr, indptr + n_indptr);
        _indices.assign(indices, indices + n_ind);
        _values.assign(values, values + n_ind);

        //XXX: assume this is only called from the python layer
        _dirty = false;
    }


    // attaching a coordinate structure facilitates going from distributed
    // input to local output.
    void Attach(coords_t coords) {

        index_type n_indptr = _indptr.size();

        _indptr.clear();
        _indices.clear();
        _values.clear();

        sort(coords.begin(), coords.end(), &sparse_matrix_t::_sort_coords);

        index_type indptr_idx  = 0;
        index_type row_counter = 0;
        for(size_t i = 0; i < coords.size(); ++i) {
            index_type cur_row = boost::get<0>(coords[i]);
            index_type cur_col = boost::get<1>(coords[i]);
            value_type cur_val = boost::get<2>(coords[i]);

            for(; indptr_idx < cur_row; ++indptr_idx)
                _indptr.push_back(row_counter);

            // sum duplicates
            while(i + 1 < coords.size() &&
                  cur_row == boost::get<0>(coords[i + 1]) &&
                  cur_col == boost::get<1>(coords[i + 1])) {

                cur_val += boost::get<2>(coords[i + 1]);
                i++;
            }

            _indices.push_back(cur_col);
            _values.push_back(cur_val);
            row_counter++;
        }

        for(; indptr_idx < n_indptr; ++indptr_idx)
            _indptr.push_back(row_counter);

        _dirty = !_dirty;
    }

    const_ind_itr_range_t indptr_itr() const {
        return std::make_pair(_indptr.begin(), _indptr.end());
    }

    const_ind_itr_range_t indices_itr() const {
        return std::make_pair(_indices.begin(), _indices.end());
    }

    const_val_itr_range_t values_itr() const {
        return std::make_pair(_values.begin(), _values.end());
    }


private:
    std::vector<index_type> _indptr;
    std::vector<index_type> _indices;
    std::vector<value_type> _values;

    bool _dirty;

    static bool _sort_coords(coord_tuple_t lhs, coord_tuple_t rhs) {
        if(boost::get<0>(lhs) != boost::get<0>(rhs))
            return boost::get<0>(lhs) < boost::get<0>(rhs);
        else
            return boost::get<1>(lhs) < boost::get<1>(rhs);
    }
};

}
}

#endif
