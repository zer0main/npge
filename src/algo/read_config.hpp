/*
 * NPG-explorer, Nucleotide PanGenome explorer
 * Copyright (C) 2014 Boris Nagaev
 *
 * See the LICENSE file for terms of use.
 */

#ifndef NPGE_READ_OPTS_HPP_
#define NPGE_READ_OPTS_HPP_

#include "global.hpp"

namespace npge {

/** Update opt value from environment variable.
Return whether option was successfully updated.
If new value can not be converted to type of target option,
throws.
*/
bool read_env(Meta* meta, const std::string& name);

/** Update all opts from environment variables */
void read_all_env(Meta* meta);

/** Update all opts from config file passed by name */
void read_config_file(Meta* meta, const std::string& cfg);

}

#endif

