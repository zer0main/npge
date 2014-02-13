/*
 * bloomrepeats, Find genomic repeats, using Bloom filter based prefiltration
 * Copyright (C) 2013 Boris Nagaev
 *
 * See the LICENSE file for terms of use.
 */

#include "MakePrePangenome.hpp"
#include "AnchorFinder.hpp"
#include "FragmentsExpander.hpp"
#include "Filter.hpp"
#include "OverlapsResolver2.hpp"
#include "Align.hpp"
#include "Rest.hpp"
#include "Connector.hpp"

namespace bloomrepeats {

MakePrePangenome::MakePrePangenome() {
    add(new AnchorFinder);
    add(new Connector);
    add(new FragmentsExpander, "--max-overlap:=200");
    add(new Filter);
    add(new OverlapsResolver2, "target=target other=target");
    add(new Filter);
    add(new FragmentsExpander, "--max-overlap:=200");
    add(new OverlapsResolver2, "target=target other=target");
    add(new Align);
    add(new Rest, "other=target");
}

}

