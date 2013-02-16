/*
 * bloomrepeats, Find genomic repeats, using Bloom filter based prefiltration
 * Copyright (C) 2012 Boris Nagaev
 *
 * See the LICENSE file for terms of use.
 */

#ifndef BR_BOUNDARIES_HPP_
#define BR_BOUNDARIES_HPP_

#include <vector>

namespace bloomrepeats {

/** Vector of positions.
Elements of the vector may be Fragment::min_pos() or Fragment::max_pos() + 1.
*/
typedef std::vector<size_t> Boundaries;

/** Return average value of the vector */
size_t avg_element(const Boundaries& boundaries);

/** Return value of the nearest element to the position.
The vector must be sorted in ascending.
*/
size_t nearest_element(const Boundaries& boundaries, size_t pos);

/** Sort the vector and merge too close elements together.
\param boundaries List of boundaries.
\param min_distance min distance between input boundaries,
    which guarantees that they will not be merged into one boundary.
\param length Length of sequence (boundaries are coordinates in the sequence).

If distance between a boundary and first/last nucleotide
is less than min_distance, then the boundary is moved to first/last nucleotide.
*/
void select_boundaries(Boundaries& boundaries, int min_distance, size_t length);

/** Shortcut for std::lower_bound */
Boundaries::const_iterator lower_bound(const Boundaries& b, size_t pos);

/** Shortcut for std::upper_bound */
Boundaries::const_iterator upper_bound(const Boundaries& b, size_t pos);

/** Return if boundaries includes pos */
bool has_element(const Boundaries& b, size_t pos);

}

#endif

