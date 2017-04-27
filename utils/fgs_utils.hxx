/*\
 * fgs_utils.hxx
 *
 * Copyright (c) 2017 - Geoff R. McLane
 * Licence: GNU GPL version 2
 *
\*/

#ifndef _FGS_UTILS_HXX_
#define _FGS_UTILS_HXX_
#include <vector>
#include <string>
#include <map>
#ifdef _WIN32
#include <direct.h> // for getcwd(), ..
#define getcwd _getcwd
#endif

#ifndef PATH_SEP
#ifdef WIN32
#define PATH_SEP "\\"
#else
#define PATH_SEP "/"
#endif
#endif

///////////////////////////////////////////////////////
//// some convenient 'typedefs'
typedef std::vector<std::string> vSTG;
typedef std::map<std::string, vSTG *>  mSTGvSTG;
typedef mSTGvSTG::iterator iSTGvSTG;
typedef std::vector<size_t> vOFF;

///////////////////////////////////////////////////////
//// services offered by library
extern std::string get_file_only(std::string &path);
extern void ensure_native_sep(std::string &path);
extern std::string get_path_only(std::string &file);
extern void add_path_sep(std::string &path);
extern vSTG PathSplit(std::string &path);
extern int find_extension(std::string &file, const char *ext);
extern std::string get_extension(std::string path);

//////////////////////////////////////////////////////////
extern int is_file_or_directory(const char * path); // 1 = file, 2 = dir, 0 = neither
extern size_t get_last_file_size();

#endif // #ifndef _FGS_UTILS_HXX_
// eof - fgs_utils.hxx
