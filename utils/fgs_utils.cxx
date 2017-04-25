/*\
 * fgs_utils.cxx
 *
 * Copyright (c) 2017 - Geoff R. McLane
 * Licence: GNU GPL version 2
 *
\*/

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>   // for unix struct stat, ...
#ifndef _WIN32
#include <unistd.h> // for getcwd(), ...
#include <string.h> // for strlen(), ...
#endif
#include "fgs_utils.hxx"

static const char *module = "fgs_utils";

///////////////////////////////////////////////////////////////////////
// utilities
void ensure_win_sep(std::string &path)
{
    std::string::size_type pos;
    std::string s = "/";
    std::string n = "\\";
    while ((pos = path.find(s)) != std::string::npos) {
        path.replace(pos, 1, n);
    }
}

void ensure_unix_sep(std::string &path)
{
    std::string::size_type pos;
    std::string n = "/";
    std::string s = "\\";
    while ((pos = path.find(s)) != std::string::npos) {
        path.replace(pos, 1, n);
    }
}

void ensure_native_sep(std::string &path)
{
#ifdef WIN32
    ensure_win_sep(path);
#else
    ensure_unix_sep(path);
#endif
}

vSTG PathSplit(std::string &path)
{
    std::string tmp = path;
    std::string s = PATH_SEP;
    vSTG result;
    int pos;
    bool done = false;

    result.clear();
    ensure_native_sep(tmp);
    while (!done) {
        pos = (int)tmp.find(s);
        if (pos >= 0) {
            result.push_back(tmp.substr(0, pos));
            tmp = tmp.substr(pos + 1);
        }
        else {
            if (!tmp.empty())
                result.push_back(tmp);
            done = true;
        }
    }
    return result;
}

vSTG FileSplit(std::string &file)
{
    vSTG vs;
    std::string::size_type pos = file.rfind(".");
    if ((pos != std::string::npos) && (pos > 0)) {
        std::string s = file.substr(0, pos);
        vs.push_back(s);
        s = file.substr(pos);
        vs.push_back(s);
    }
    else {
        vs.push_back(file);
    }
    return vs;
}

void fix_relative_path(std::string &path)
{
    vSTG vs = PathSplit(path);
    size_t ii, max = vs.size();
    std::string npath, tmp;
    vSTG n;
    for (ii = 0; ii < max; ii++) {
        tmp = vs[ii];
        if (tmp == ".")
            continue;
        if (tmp == "..") {
            if (n.size()) {
                n.pop_back();
                continue;
            }
            return;
        }
        n.push_back(tmp);
    }
    ii = n.size();
    if (ii && (ii != max)) {
        max = ii;
        for (ii = 0; ii < max; ii++) {
            tmp = n[ii];
            if (npath.size())
                npath += PATH_SEP;
            npath += tmp;
        }
        path = npath;
    }
}

std::string get_path_only(std::string &file)
{
    std::string path;
    vSTG vs = PathSplit(file);
    size_t ii, max = vs.size();
    if (max)
        max--;
    if (max) {
        for (ii = 0; ii < max; ii++) {
            if (path.size() || ii)
                path += PATH_SEP;
            path += vs[ii];
        }
    }
    else {
        char *pcwd = getcwd(NULL, 0);
        if (pcwd)
            path = pcwd;
    }
    return path;
}

std::string get_file_only(std::string &path)
{
    std::string file;
    vSTG vs = PathSplit(path);
    size_t max = vs.size();
    if (max) {
        file = vs[max - 1];
    }
    return file;
}

#ifdef _MSC_VER
#define M_IS_DIR _S_IFDIR
#else // !_MSC_VER
#define M_IS_DIR S_IFDIR
#endif

static	struct stat _s_buf;
int is_file_or_directory(const char * path) // 1 = file, 2 = dir, 0 = neither
{
    if (!path)
        return 0;
    if (stat(path, &_s_buf) == 0)
    {
        if (_s_buf.st_mode & M_IS_DIR)
            return 2;
        else
            return 1;
    }
    return 0;
}
size_t get_last_file_size() { return _s_buf.st_size; }

int find_extension(std::string &file, const char *ext)
{
    if (!ext)
        return 0;
    size_t len = strlen(ext);
    if (!len)
        return 0;
    size_t pos = file.find(ext);
    if (pos == file.size() - len)
        return 1;
    return 0;
}

std::string get_extension(std::string path)
{
    std::string file = get_file_only(path); // actually only gets LAST path component
    std::string ext;
    size_t max;
    if (file.size()) {
        vSTG vs = FileSplit(file);  // split name on last '.', if one exists
        max = vs.size(); // get count
        if (max > 1)    // will be 1 if no '.', else
            ext = vs[1];    // get the file extension
    }
    else {
        max = 0;
    }
    return ext;
}

void add_path_sep(std::string &path)
{
    size_t len = path.size();
    if (len) {
        len--;
        int c = path[len];
        if ((c != '\\') && (c != '/'))
            path += PATH_SEP;
    }
}

////////////////////////////////////////////////////////////////////


// eof = fgs_utils.cxx
