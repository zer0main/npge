/*
 * bloomrepeats, Find genomic repeats, using Bloom filter based prefiltration
 * Copyright (C) 2012 Boris Nagaev
 *
 * See the LICENSE file for terms of use.
 */

#ifndef BR_FILE_READER_HPP_
#define BR_FILE_READER_HPP_

#include <string>
#include <vector>

namespace bloomrepeats {

/** Base class for file readers */
class FileReader {
public:
    /** Files list */
    typedef std::vector<std::string> Files;

    /** Get files list */
    const std::vector<std::string>& files() const {
        return files_;
    }

    /** Set files list */
    void set_files(const std::vector<std::string>& files) {
        files_ = files;
    }

    /** Set file (list of one file) */
    void set_input_file(const std::string& file);

private:
    std::vector<std::string> files_;
    // FIXME rename to input_files
    // FIXME rename FileWritter:file to output_file
};

}

#endif

