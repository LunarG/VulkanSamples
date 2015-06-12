/*
 * Vulkan Samples Kit
 *
 * Copyright (C) 2015 LunarG, Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */

/*
VULKAN_SAMPLE_DESCRIPTION
samples utility functions
*/

#include "util.hpp"

using namespace std;

string get_file_name(const string& s) {

    char sep = '/';

#ifdef _WIN32
    sep = '\\';
#endif

    //cout << "in get_file_name\n";
    size_t i = s.rfind(sep, s.length());
    if (i != string::npos) {
        return(s.substr(i+1, s.length() - i));
    }

    return("");
}


std::string get_base_data_dir()
{
    return std::string(VULKAN_SAMPLES_BASE_DIR) + "/data/";
}

std::string get_data_dir( std::string filename )
{
    std::string basedir = get_base_data_dir();
    // get the base filename
    std::string fname = get_file_name( filename );

    // get the prefix of the base filename, i.e. the part before the dash
    stringstream stream(fname);
    std::string prefix;
    getline(stream, prefix, '-');
    std::string ddir = basedir + prefix;
    return ddir;
}

