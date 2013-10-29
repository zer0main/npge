/*
 * bloomrepeats, Find genomic repeats, using Bloom filter based prefiltration
 * Copyright (C) 2012 Boris Nagaev
 *
 * See the LICENSE file for terms of use.
 */

#include <iostream>
#include <exception>
#include <boost/foreach.hpp>

#include "process.hpp"
#include "po.hpp"
#include "string_arguments.hpp"
#include "name_to_stream.hpp"

namespace bloomrepeats {

void print_processor_tree(Processor* processor, int indent = 0) {
    const int SPACES_IN_TAB = 4;
    std::string tab(SPACES_IN_TAB * indent, ' ');
    *name_to_ostream(":cerr") << tab << processor->key();
    *name_to_ostream(":cerr") << ": " << processor->name() << "\n";
    BOOST_FOREACH (Processor* child, processor->children()) {
        print_processor_tree(child, indent + 1);
    }
}

int process(int argc, char** argv,
            Processor* processor,
            const std::string& name,
            const std::string& positional) {
    po::options_description desc(name);
    add_general_options(desc);
    processor->add_options(desc);
    po::positional_options_description pod;
    if (!positional.empty()) {
        pod.add(positional.c_str(), -1);
    }
    po::variables_map vm;
    int error = read_options(argc, argv, vm, desc, pod);
    if (error) {
        return error;
    }
    if (vm.count("debug")) {
        processor->apply_options(vm);
    } else {
        try {
            processor->apply_options(vm);
        } catch (std::exception& e) {
            std::cerr << argv[0];
            std::cerr << ": error while applying options" << std::endl;
            std::cerr << "  " << e.what() << std::endl;
            return 255;
        } catch (...) {
            std::cerr << argv[0];
            std::cerr << ": error while applying options" << std::endl;
            std::cerr << "  Unknown error" << std::endl;
            return 255;
        }
    }
    std::vector<std::string> errors = processor->options_errors();
    if (!errors.empty()) {
        std::cerr << argv[0];
        std::cerr << ": error while validating options" << std::endl;
        BOOST_FOREACH (const std::string& message, errors) {
            std::cerr << message << std::endl;
        }
        return 255;
    }
    std::vector<std::string> warnings = processor->options_warnings();
    if (!warnings.empty()) {
        std::cerr << argv[0];
        std::cerr << ": warnings while validating options" << std::endl;
        BOOST_FOREACH (const std::string& message, warnings) {
            std::cerr << message << std::endl;
        }
    }
    if (!processor->block_set()) {
        processor->set_empty_block_set();
    }
    if (vm.count("tree")) {
        print_processor_tree(processor);
        return 0;
    }
    if (vm.count("debug")) {
        processor->run();
    } else {
        try {
            processor->run();
        } catch (std::exception& e) {
            std::cerr << argv[0] << ": algorithm error" << std::endl;
            std::cerr << "  " << e.what() << std::endl;
            return 255;
        } catch (...) {
            std::cerr << argv[0];
            std::cerr << ": error while applying options" << std::endl;
            std::cerr << "  Unknown error" << std::endl;
            return 255;
        }
    }
    return 0;
}

void copy_processor_options(Processor& dest, const Processor& source) {
    StringToArgv dummy;
    po::options_description desc;
    source.add_options(desc);
    po::variables_map vm;
    po::store(po::command_line_parser(dummy.argc(), dummy.argv()).options(desc)
              .allow_unregistered().run(), vm);
    // po::notify(vm); // to pass required options check
    dest.apply_options(vm);
    dest.set_timing(source.timing());
}

}

