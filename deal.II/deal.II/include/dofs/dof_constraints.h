//----------------------------  dof_constraints.h  ---------------------------
//    $Id$
//    Version: $Name$
//
//    Copyright (C) 1998, 1999, 2000, 2001, 2002, 2003 by the deal.II authors
//
//    This file is subject to QPL and may not be  distributed
//    without copyright and license information. Please refer
//    to the file deal.II/doc/license.html for the  text  and
//    further information on this license.
//
//----------------------------  dof_constraints.h  ---------------------------
#ifndef __deal2__dof_constraints_h
#define __deal2__dof_constraints_h


#include <base/config.h>
#include <vector>
#include <utility>
#include <base/exceptions.h>
#include <base/subscriptor.h>

class SparsityPattern;
class CompressedSparsityPattern;
class BlockSparsityPattern;
class CompressedBlockSparsityPattern;
template <typename number> class SparseMatrix;
template <typename number> class BlockSparseMatrix;
class BlockIndices;


/**
 * This class implements linear homogeneous constraints on degrees of
 * freedom. In particular, it handles constraints of the form
 * $x_{i1} = \sum_{j=2}^M a_{i_j} x_{i_j}$. Each "line" in objects of this
 * class corresponds to one such constraint, with the number of the
 * line being $i1$, and the entries in this line being pairs
 * $(i_j,a_{i_j})$. Note that the constraints are linear in the $x_i$,
 * and that there is not constant (non-homogeneous) term in the
 * constraint. However, this is exactly the form we need for hanging
 * node and certain other constraints, where we need to constrain one
 * degree of freedom in terms of others. The name of the class stems
 * from the fact that these constraints can be represented in matrix
 * form as $X x = 0$, and this object then describes the matrix $X$.
 *
 * The matrix is organized in lines (rows), but only those lines are stored
 * where constraints are present.
 *
 * Constraint matrices are used to handle hanging nodes and other constrained
 * degrees of freedom. When building the global system matrix and the right
 * hand sides, you normally build them without taking care of the constraints,
 * purely on a topological base, i.e. by a loop over cells. In order to do
 * actual calculations, you have to 'condense' these matrices: eliminate
 * constrained degrees of freedom and distribute the appropriate values to
 * the unconstrained dofs. This changes the sparsity pattern of the sparse
 * matrices used in finite element calculations und is thus a quite expensive
 * operation.
 *
 * Condensation is done in four steps: first the large matrix sparsity pattern
 * is created (e.g. using @ref{DoFHandler}@p{::create_sparsity_pattern}), then the
 * sparsity pattern of the condensed matrix is made out of the large sparsity
 * pattern and the constraints. After that the global matrix is assembled and
 * finally condensed. To do these steps, you have (at least) two possibilities:
 * @begin{itemize}
 * @item Use two different sparsity patterns and two different matrices: you
 *   may eliminate the lines and rows connected with a constraint and create
 *   a totally new sparsity pattern and a new system matrix. This has the
 *   advantage that the resulting system of equations is free from artifacts
 *   of the condensation process and is therefore faster in the solution process
 *   since no unnecessary multiplications occur (see below). However, there are
 *   two major drawbacks: keeping two matrices at the same time can be quite
 *   unacceptable in many cases, since these matrices may be several 10 or even
 *   100 MB large. Secondly, the condensation process is quite expensive, since
 *   @em{all} entries of the matrix have to be copied, not only those which are
 *   subject to constraints.
 *
 * @item Use only one sparsity pattern and one matrix: doing it this way, the
 *   condense functions add nonzero entries to the sparsity pattern of the
 *   large matrix (with constrained nodes in it) where the condensation process
 *   of the matrix will create additional nonzero elements. In the condensation
 *   process itself, lines and rows subject to constraints are distributed to
 *   the lines and rows of unconstrained nodes. The constrained lines remain in
 *   place, however, unlike in the first possibility described above. In order
 *   not to disturb the solution process, these lines and rows are filled with
 *   zeros and identity on the main diagonal; the appropriate value in the right
 *   hand sides is set to zero. This way, the constrained node will always get
 *   the value zero upon solution of the equation system and will not couple to
 *   other nodes any more.
 *
 *   This method has the advantage that only one matrix and sparsity pattern is
 *   needed thus using less memory. Additionally, the condensation process is
 *   less expensive, since not all but only constrained values in the matrix
 *   have to be copied. On the other hand, the solution process will take a bit
 *   longer, since matrix vector multiplications will incur multiplications
 *   with zeroes in the lines subject to constraints. Additionally, the vector
 *   size is larger than in the first possibility, resulting in more memory
 *   consumption for those iterative solution methods using a larger number of
 *   auxiliary vectors (e.g. methods using explicit orthogonalization
 *   procedures).
 * @end{itemize}
 *
 * Usually, the second way is chosen since memory consumption upon construction
 * of a second matrix rules out the first possibility.
 *
 * This class provides two sets of @p{condense} functions: those taking two
 * arguments refer to the first possibility above, those taking only one do
 * their job in-place and refer to the second possibility.
 *
 * Condensing vectors works exactly as described above for matrices.
 *
 * After solving the condensed system of equations, the solution vector has to
 * be redistributed. This is done by the two @p{distribute} function, one working
 * with two vectors, one working in-place. The operation of distribution undoes
 * the condensation process in some sense, but it should be noted that it is not
 * the inverse operation.
 *
 * @author Wolfgang Bangerth, 1998
 */
class ConstraintMatrix : public Subscriptor
{
  public:
				     /**
				      * Constructor
				      */
    ConstraintMatrix ();


				     /**
				      * Add a new line to the
				      * matrix. If the line already
				      * exists, then the function
				      * simply returns.
				      */
    void add_line (const unsigned int line);

				     /**
				      * Add an entry to a given
				      * line. The list of lines is
				      * searched from the back to the
				      * front, so clever programming
				      * would add a new line (which is
				      * pushed to the back) and
				      * immediately afterwards fill
				      * the entries of that line. This
				      * way, no expensive searching is
				      * needed.
				      *
				      * If an entry with the same
				      * indices as the one this
				      * function call denotes already
				      * exists, then this function
				      * simply returns provided that
				      * the value of the entry is the
				      * same. Thus, it does no harm to
				      * enter a constraint twice.
				      */
    void add_entry (const unsigned int line,
		    const unsigned int column,
		    const double value);

				     /**
				      * Add a whole series of entries,
				      * denoted by pairs of column
				      * indices and values, to a line
				      * of constraints. This function
				      * is equivalent to calling the
				      * preceeding function more
				      * several times, but is faster.
				      */
    void add_entries (const unsigned int                                  line,
		      const std::vector<std::pair<unsigned int,double> > &col_val_pairs);

				     /**
				      * Close the filling of
				      * entries. Since the lines of a
				      * matrix of this type are
				      * usually filled in an arbitrary
				      * order and since we do not want
				      * to use associative constainers
				      * to store the lines, we need to
				      * sort the lines and within the
				      * lines the columns before usage
				      * of the matrix.  This is done
				      * through this function.
				      *
				      * Also, zero entries are
				      * discarded, since they are not
				      * needed.
				      *
				      * After closing, no more entries
				      * are accepted. If the object
				      * was already closed, then this
				      * function returns immediately.
				      */
    void close ();

				     /**
				      * Merge the constraints
				      * represented by the object
				      * given as argument into the
				      * constraints represented by
				      * this object. Both objects may
				      * or may not be closed (by
				      * having their function
				      * @p{close} called before), if
				      * this object was closed before,
				      * then it will be closed
				      * afterwards as well. Note,
				      * however, that if the other
				      * argument is closed, then
				      * merging may be significantly
				      * faster.
				      *
				      * Note that the constraints in
				      * each of the two objects (the
				      * old one represented by this
				      * object and the argument) may
				      * not refer to the same degree
				      * of freedom, i.e. a degree of
				      * freedom that is constrained in
				      * one object may not be
				      * constrained in the second. If
				      * this is nevertheless the case,
				      * an exception is thrown.
				      *
				      * However, the following is
				      * possible: if DoF @p{x} is
				      * constrained to dofs @p{x_i}
				      * for some set of indices @p{i},
				      * then the DoFs @p{x_i} may be
				      * further constrained by the
				      * constraints object given as
				      * argument, although not to
				      * other DoFs that are
				      * constrained in either of the
				      * two objects. Note that it is
				      * not possible that the DoFs
				      * @p{x_i} are constrained within
				      * the present object.
				      *
				      * Because of simplicity of
				      * implementation, and also to
				      * avoid cycles, this operation
				      * is not symmetric: degrees of
				      * freedom that are constrained
				      * in the given argument object
				      * may not be constrained to DoFs
				      * that are themselves
				      * constrained within the present
				      * object.
				      *
				      * The aim of these merging
				      * operations is that if, for
				      * example, you have hanging
				      * nodes that are constrained to
				      * the degrees of freedom
				      * adjacent to them, you cannot
				      * originally, i.e. within one
				      * object, constrain these
				      * adjacent nodes
				      * further. However, that may be
				      * desirable in some cases, for
				      * example if they belong to a
				      * symmetry boundary for which
				      * the nodes on one side of the
				      * domain should have the same
				      * values as those on the other
				      * side. In that case, you would
				      * first construct a costraints
				      * object holding the hanging
				      * nodes constraints, and a
				      * second one that contains the
				      * constraints due to the
				      * symmetry boundary. You would
				      * then finally merge this second
				      * one into the first, possibly
				      * eliminating constraints of
				      * hanging nodes to adjacent
				      * boundary nodes by constraints
				      * to nodes at the opposite
				      * boundary.
				      */
    void merge (const ConstraintMatrix &other_constraints);

				     /**
				      * Shift all entries of this
				      * matrix down @p{offset} rows
				      * and over @p{offset} columns.
				      *
				      * This function is useful if you
				      * are building block matrices,
				      * where all blocks are built by
				      * the same @p{DoFHandler}
				      * object, i.e. the matrix size
				      * is larger than the number of
				      * degrees of freedom. Since
				      * several matrix rows and
				      * columns correspond to the same
				      * degrees of freedom, you'd
				      * generate several constraint
				      * objects, then shift them, and
				      * finally @p{merge} them
				      * together again.
				      */
    void shift (const unsigned int offset);
    
				     /**
				      * Clear all entries of this matrix. Reset
				      * the flag determining whether new entries
				      * are accepted or not.
				      *
				      * This function may be called also on
				      * objects which are empty or already
				      * cleared.
				      */
    void clear ();

				     /**
				      * Return number of constraints stored in
				      * this matrix.
				      */
    unsigned int n_constraints () const;

				     /**
				      * Return whether the degree of
				      * freedom with number @p{index} is
				      * a constrained one.
				      *
				      * Note that if @p{close} was
				      * called before, then this
				      * function is significantly
				      * faster, since then the
				      * constrained degrees of freedom
				      * are sorted and we can do a
				      * binary search, while before
				      * @p{close} was called, we have to
				      * perform a linear search
				      * through all entries.
				      */
    bool is_constrained (const unsigned int index) const;

				     /**
				      * Return whether the dof is
				      * constrained, and whether it is
				      * constrained to only one other
				      * degree of freedom with weight
				      * one. The function therefore
				      * returns whether the degree of
				      * freedom would simply be
				      * eliminated in favor of exactly
				      * one other degree of freedom.
				      *
				      * The function returns @p{false}
				      * if either the degree of
				      * freedom is not constrained at
				      * all, or if it is constrained
				      * to more than one other degree
				      * of freedom, or if it is
				      * constrained to only one degree
				      * of freedom but with a weight
				      * different from one.
				      */
    bool is_identity_constrained (const unsigned int index) const;
    
				     /**
				      * Return the maximum number of
				      * other dofs that one dof is
				      * constrained to. For example,
				      * in 2d a hanging node is
				      * constrained only to its two
				      * neighbors, so the returned
				      * value would be @p{2}. However,
				      * for higher order elements
				      * and/or higher dimensions, or
				      * other types of constraints,
				      * this number is no more
				      * obvious.
				      *
				      * The name indicates that within
				      * the system matrix, references
				      * to a constrained node are
				      * indirected to the nodes it is
				      * constrained to.
				      */
    unsigned int max_constraint_indirections () const;

				     /**
				      * Condense a given sparsity
				      * pattern. This function assumes
				      * the uncondensed matrix struct
				      * to be compressed and the one
				      * to be filled to be empty. The
				      * condensed structure is
				      * compressed afterwards.
				      *
				      * The constraint matrix object
				      * must be closed to call this
				      * function.
				      */
    void condense (const SparsityPattern &uncondensed,
		   SparsityPattern       &condensed) const;


				     /**
				      * This function does much the
				      * same as the above one, except
				      * that it condenses the matrix
				      * struct 'in-place'. It does not
				      * remove nonzero entries from
				      * the matrix but adds those
				      * needed for the process of
				      * distribution of the
				      * constrained degrees of
				      * freedom.
				      *
				      * Since this function adds new
				      * nonzero entries to the
				      * sparsity pattern, the argument
				      * must not be
				      * compressed. However the
				      * constraint matrix must be
				      * closed.  The matrix struct is
				      * compressed at the end of the
				      * function.
				      */
    void condense (SparsityPattern &sparsity) const;

				     /**
				      * Same function as above, but
				      * condenses square block sparsity
				      * patterns.
				      */
    void condense (BlockSparsityPattern &sparsity) const;

				     /**
				      * Same function as above, but
				      * condenses square compressed
				      * sparsity patterns.
				      */
    void condense (CompressedSparsityPattern &sparsity) const;

				     /**
				      * Same function as above, but
				      * condenses square compressed
				      * sparsity patterns.
				      */
    void condense (CompressedBlockSparsityPattern &sparsity) const;
    
				     /**
				      * Condense a given matrix. The associated
				      * matrix struct should be condensed and
				      * compressed. It is the user's
				      * responsibility to guarantee that all
				      * entries in the @p{condensed} matrix be
				      * zero!
				      *
				      * The constraint matrix object must be
				      * closed to call this function.
				      */
    template<typename number>
    void condense (const SparseMatrix<number> &uncondensed,
		   SparseMatrix<number>       &condensed) const;

				     /**
				      * This function does much the same as
				      * the above one, except that it condenses
				      * the matrix 'in-place'. See the general
				      * documentation of this class for more
				      * detailed information.
				      */
    template<typename number>
    void condense (SparseMatrix<number> &matrix) const;

				     /**
				      * Same function as above, but
				      * condenses square block sparse
				      * matrices.
				      */
    template <typename number>
    void condense (BlockSparseMatrix<number> &matrix) const;
    
				     /**
				      * Condense the given vector @p{uncondensed}
				      * into @p{condensed}. It is the user's
				      * responsibility to guarantee that all
				      * entries of @p{condensed} be zero!
				      *
				      * The @p{VectorType} may be a
				      * @ref{Vector}@p{<float>},
				      * @ref{Vector}@p{<double>},
				      * @ref{BlockVector}@p{<...>}, or any
				      * other type having the same
				      * interface.
				      */
    template <class VectorType>
    void condense (const VectorType &uncondensed,
		   VectorType       &condensed) const;

				     /**
				      * Condense the given vector
				      * in-place. The @p{VectorType} may
				      * be a @ref{Vector}@p{<float>},
				      * @ref{Vector}@p{<double>},
				      * @ref{BlockVector}@p{<...>}, or any
				      * other type having the same
				      * interface.
				      */
    template <class VectorType>
    void condense (VectorType &vec) const;

				     /**
				      * Re-distribute the elements of
				      * the vector @p{condensed} to
				      * @p{uncondensed}. It is the
				      * user's responsibility to
				      * guarantee that all entries of
				      * @p{uncondensed} be zero!
				      *
				      * This function undoes the
				      * action of @p{condense} somehow,
				      * but it should be noted that it
				      * is not the inverse of
				      * @p{condense}.
				      *
				      * The @p{VectorType} may be a
				      * @ref{Vector}@p{<float>},
				      * @ref{Vector}@p{<double>},
				      * @ref{BlockVector}@p{<...>}, or any
				      * other type having the same
				      * interface.
				      */
    template <class VectorType>
    void distribute (const VectorType &condensed,
		     VectorType       &uncondensed) const;

				     /**
				      * Re-distribute the elements of
				      * the vector in-place. The
				      * @p{VectorType} may be a
				      * @ref{Vector}@p{<float>},
				      * @ref{Vector}@p{<double>},
				      * @ref{BlockVector}@p{<...>}, or any
				      * other type having the same
				      * interface.
				      */
    template <class VectorType>
    void distribute (VectorType &vec) const;
    
				     /**
				      * Delete hanging nodes in a
				      * vector.  Sets all hanging node
				      * values to zero. The
				      * @p{VectorType} may be a
				      * @ref{Vector}@p{<float>},
				      * @ref{Vector}@p{<double>},
				      * @ref{BlockVector}@p{<...>}, or any
				      * other type having the same
				      * interface.
				      */
    template <class VectorType>
    void set_zero (VectorType &vec) const;


				     /**
				      * Print the constraint lines. Mainly for
				      * debugging purposes.
				      *
				      * This function writes out all entries
				      * in the constraint matrix lines with
				      * their value in the form
				      * @p{row col : value}. Unconstrained lines
				      * containing only one identity entry are
				      * not stored in this object and are not
				      * printed.
				      */
    void print (std::ostream &) const;

				     /**
				      * Determine an estimate for the
				      * memory consumption (in bytes)
				      * of this object.
				      */
    unsigned int memory_consumption () const;


				     /**
				      * Exception
				      */
    DeclException0 (ExcMatrixIsClosed);
				     /**
				      * Exception
				      */
    DeclException0 (ExcMatrixNotClosed);
				     /**
				      * Exception
				      */
    DeclException1 (ExcLineInexistant,
		    unsigned int,
		    << "The specified line " << arg1
		    << " does not exist.");
				     /**
				      * Exception
				      */
    DeclException0 (ExcMatrixNotSquare);
				     /**
				      * Exception
				      */
    DeclException0 (ExcWrongDimension);
				     /**
				      * Exception
				      */
    DeclException4 (ExcEntryAlreadyExists,
		    int, int, double, double,
		    << "The entry for the indices " << arg1 << " and "
		    << arg2 << " already exists, but the values "
		    << arg3 << " (old) and " << arg4 << " (new) differ.");
				     /**
				      * Exception
				      */
    DeclException2 (ExcDoFConstrainedToConstrainedDoF,
		    int, int,
		    << "You tried to constrain DoF " << arg1
		    << " to DoF " << arg2
		    << ", but that one is also constrained. This is not allowed!");
				     /**
				      * Exception.
				      */
    DeclException1 (ExcDoFIsConstrainedFromBothObjects,
		    int,
		    << "Degree of freedom " << arg1
		    << " is constrained from both object in a merge operation.");
				     /**
				      * Exception
				      */
    DeclException1 (ExcDoFIsConstrainedToConstrainedDoF,
		    int,
		    << "In the given argument a degree of freedom is constrained "
		    << "to another DoF with number " << arg1
		    << ", which however is constrained by this object. This is not"
		    << " allowed.");
    
  private:

				     /**
				      * This class represents one line of a
				      * constraint matrix.
				      */
    struct ConstraintLine
    {
					 /**
					  * Number of this line. Since only very
					  * few lines are stored, we can not
					  * assume a specific order and have
					  * to store the line number explicitly.
					  */
	unsigned int line;

					 /**
					  * Row numbers and values of the entries
					  * in this line.
					  *
					  * For the reason why we use a vector
					  * instead of a map and the consequences
					  * thereof, the same applies as what is
					  * said for @ref{ConstraintMatrix}@p{::lines}.
					  */
	std::vector<std::pair<unsigned int,double> > entries;

					 /**
					  * This operator is a bit
					  * weird and unintuitive: it
					  * compares the line numbers
					  * of two lines. We need this
					  * to sort the lines; in fact
					  * we could do this using a
					  * comparison predicate.
					  * However, this way, it is
					  * easier, albeit unintuitive
					  * since two lines really
					  * have no god-given order
					  * relation.
					  */
	bool operator < (const ConstraintLine &) const;

					 /**
					  * This operator is likewise
					  * weird: it checks whether
					  * the line indices of the
					  * two operands are equal,
					  * irrespective of the fact
					  * that the contents of the
					  * line may be different.
					  */
	bool operator == (const ConstraintLine &) const;

					 /**
					  * Determine an estimate for the
					  * memory consumption (in bytes)
					  * of this object.
					  */
	unsigned int memory_consumption () const;
    };

				     /**
				      * Store the lines of the matrix.
				      * Entries are usually
				      * appended in an arbitrary order and
				      * insertion into a vector is done best
				      * at the end, so the order is
				      * unspecified after all entries are
				      * inserted. Sorting of the entries takes
				      * place when calling the @p{close()} function.
				      *
				      * We could, instead of using a vector, use
				      * an associative array, like a map to
				      * store the lines. This, however, would
				      * mean a much more fractioned heap since it
				      * allocates many small objects, ans would
				      * additionally make usage of this matrix
				      * much slower.
				      */
    std::vector<ConstraintLine> lines;
	
				     /**
				      * Store whether the arrays are sorted.
				      * If so, no new entries can be added.
				      */
    bool sorted;

				     /**
				      * Return @p{true} if the weight
				      * of an entry (the second
				      * element of the pair) equals
				      * zero. This function is used to
				      * delete entries with zero
				      * weight.
				      */
    static bool check_zero_weight (const std::pair<unsigned int, double> &p);
};


#endif
