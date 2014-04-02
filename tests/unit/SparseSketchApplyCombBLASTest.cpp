/**
 *  This test ensures that the sketch application (for CombBLAS matrices) is
 *  done correctly (on-the-fly matrix multiplication in the code is compared
 *  to true matrix multiplication).
 *  This test builds on the following assumptions:
 *
 *      - CombBLAS PSpGEMM returns the correct result, and
 *      - the random numbers in row_idx and row_value (see
 *        hash_transform_data_t) are drawn from the promised distributions.
 */


#include <vector>

#include <boost/mpi.hpp>
#include <boost/test/minimal.hpp>

#include "../../utility/distributions.hpp"
#include "../../base/context.hpp"
#include "../../sketch/hash_transform.hpp"

#include "../../base/sparse_matrix.hpp"

typedef FullyDistVec<size_t, double> mpi_vector_t;
typedef SpDCCols<size_t, double> col_t;
typedef SpParMat<size_t, double, col_t> DistMatrixType;
typedef PlusTimesSRing<double, double> PTDD;
typedef skylark::base::sparse_matrix_t<double> LocalMatrixType;


template < typename InputMatrixType,
           typename OutputMatrixType = InputMatrixType >
struct Dummy_t : public skylark::sketch::hash_transform_t<
    InputMatrixType, OutputMatrixType,
    boost::random::uniform_int_distribution,
    skylark::utility::rademacher_distribution_t > {

    typedef skylark::sketch::hash_transform_t<
        InputMatrixType, OutputMatrixType,
        boost::random::uniform_int_distribution,
        skylark::utility::rademacher_distribution_t >
            hash_t;

    Dummy_t(int N, int S, skylark::base::context_t& context)
        : skylark::sketch::hash_transform_t<InputMatrixType, OutputMatrixType,
          boost::random::uniform_int_distribution,
          skylark::utility::rademacher_distribution_t>(N, S, context)
    {}

    std::vector<size_t> getRowIdx() { return hash_t::row_idx; }
    std::vector<double> getRowValues() { return hash_t::row_value; }
};


template<typename sketch_t>
void compute_sketch_matrix(sketch_t sketch, const DistMatrixType &A,
                           DistMatrixType &result) {

    std::vector<size_t> row_idx = sketch.getRowIdx();
    std::vector<double> row_val = sketch.getRowValues();

    // PI generated by random number gen
    size_t sketch_size = row_val.size();
    mpi_vector_t cols(sketch_size);
    mpi_vector_t rows(sketch_size);
    mpi_vector_t vals(sketch_size);

    for(size_t i = 0; i < sketch_size; ++i) {
        cols.SetElement(i, i);
        rows.SetElement(i, row_idx[i]);
        vals.SetElement(i, row_val[i]);
    }

    result = DistMatrixType(result.getnrow(), result.getncol(),
                            rows, cols, vals);
}


int test_main(int argc, char *argv[]) {

    //////////////////////////////////////////////////////////////////////////
    //[> Parameters <]

    //FIXME: use random sizes?
    const size_t n   = 10;
    const size_t m   = 5;
    const size_t n_s = 6;
    const size_t m_s = 3;

    //////////////////////////////////////////////////////////////////////////
    //[> Setup test <]
    namespace mpi = boost::mpi;
    mpi::environment env(argc, argv);
    mpi::communicator world;
    const size_t rank = world.rank();

    skylark::base::context_t context (0);

    double count = 1.0;

    const size_t matrix_full = n * m;
    mpi_vector_t colsf(matrix_full);
    mpi_vector_t rowsf(matrix_full);
    mpi_vector_t valsf(matrix_full);

    for(size_t i = 0; i < matrix_full; ++i) {
        colsf.SetElement(i, i % m);
        rowsf.SetElement(i, i / m);
        valsf.SetElement(i, count);
        count++;
    }

    DistMatrixType A(n, m, rowsf, colsf, valsf);


    //////////////////////////////////////////////////////////////////////////
    //[> Column wise application DistSparseMatrix -> DistSparseMatrix <]

    //[> 1. Create the sketching matrix <]
    Dummy_t<DistMatrixType, DistMatrixType> Sparse(n, n_s, context);

    //[> 2. Create space for the sketched matrix <]
    mpi_vector_t zero;
    DistMatrixType sketch_A(n_s, m, zero, zero, zero);

    //[> 3. Apply the transform <]
    Sparse.apply(A, sketch_A, skylark::sketch::columnwise_tag());

    //[> 4. Build structure to compare <]
    DistMatrixType pi_sketch(n_s, n, zero, zero, zero);
    compute_sketch_matrix(Sparse, A, pi_sketch);
    DistMatrixType expected_A = Mult_AnXBn_Synch<PTDD, double, col_t>(pi_sketch, A, false, false);
    if (!static_cast<bool>(expected_A == sketch_A))
        BOOST_FAIL("Result of colwise (dist -> dist) application not as expected");

    //////////////////////////////////////////////////////////////////////////
    //[> Column wise application DistSparseMatrix -> LocalSparseMatrix <]

    //Dummy_t<DistMatrixType, LocalMatrixType> LocalSparse(n, n_s, context);
    //LocalMatrixType local_sketch_A;
    //LocalSparse.apply(A, local_sketch_A, skylark::sketch::columnwise_tag());

    //int nrowptr = 0, nnz = 0;
    //local_sketch_A.get_size(&nrowptr, &nnz);
    //mpi_vector_t lcols(nnz);
    //mpi_vector_t lrows(nnz);
    //mpi_vector_t lvals(nnz);

    //size_t col = 0;
    //typename LocalMatrixType::const_ind_itr_range_t citr =
        //local_sketch_A.indptr_itr();
    //typename LocalMatrixType::const_ind_itr_range_t ritr =
        //local_sketch_A.indices_itr();
    //typename LocalMatrixType::const_val_itr_range_t vitr =
        //local_sketch_A.values_itr();

    //size_t counter = 0;
    //for(; citr.first + 1 != citr.second; citr.first++, ++col) {
        //for(size_t idx = 0; idx < (*(citr.first + 1) - *citr.first);
            //ritr.first++, vitr.first++, ++idx) {

            //lrows.SetElement(counter, *ritr.first);
            //lcols.SetElement(counter, col);
            //lvals.SetElement(counter, *vitr.first);
            //counter++;
        //}
    //}

    //DistMatrixType local_sketch_result(n_s, m, lrows, lcols, lvals);

    //DistMatrixType pi_sketch_l(n_s, n, zero, zero, zero);
    //compute_sketch_matrix(LocalSparse, A, pi_sketch_l);
    //DistMatrixType expected_A_l = Mult_AnXBn_Synch<PTDD, double, col_t>(pi_sketch_l, A, false, false);
    //if (!static_cast<bool>(expected_A_l == local_sketch_result))
        //BOOST_FAIL("Result of colwise (dist -> local) application not as expected");

    //////////////////////////////////////////////////////////////////////////
    //[> Row wise application DistSparseMatrix -> DistSparseMatrix <]

    //[> 1. Create the sketching matrix <]
    Dummy_t<DistMatrixType, DistMatrixType> Sparse_r(m, m_s, context);

    //[> 2. Create space for the sketched matrix <]
    DistMatrixType sketch_A_r(n, m_s, zero, zero, zero);

    //[> 3. Apply the transform <]
    Sparse_r.apply(A, sketch_A_r, skylark::sketch::rowwise_tag());

    //[> 4. Build structure to compare <]
    DistMatrixType pi_sketch_r(m_s, m, zero, zero, zero);
    compute_sketch_matrix(Sparse_r, A, pi_sketch_r);
    pi_sketch_r.Transpose();
    DistMatrixType expected_AR = PSpGEMM<PTDD>(A, pi_sketch_r);

    if (!static_cast<bool>(expected_AR == sketch_A_r))
        BOOST_FAIL("Result of rowwise (dist -> dist) application not as expected");

    return 0;
}
