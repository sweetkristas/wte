#include <boost/algorithm/string.hpp>
#include <boost/regex.hpp>
#include <boost/filesystem.hpp>

#include "asserts.hpp"
#include "filesystem.hpp"
#include "foreach.hpp"
#include "unit_test.hpp"


namespace sys {

bool is_directory(const std::string& dname)
{
	boost::filesystem::path p(dname);
	return boost::filesystem::exists(p) && boost::filesystem::is_directory(p);
}

bool file_exists(const std::string& fname)
{
	boost::filesystem::path p(fname);
	return boost::filesystem::exists(p) && boost::filesystem::is_regular_file(p);
}

void get_files_in_dir(const std::string& dir,
                      std::vector<std::string>* files,
                      std::vector<std::string>* dirs,
                      FILE_NAME_MODE mode)
{
	boost::filesystem::path p(dir);
	std::vector<boost::filesystem::path> fds;
	if(is_directory(dir)) {
		std::copy(boost::filesystem::directory_iterator(p), 
			boost::filesystem::directory_iterator(), 
			std::back_inserter(fds));
		std::sort(fds.begin(), fds.end());
		for(std::vector<boost::filesystem::path>::iterator it = fds.begin(); it != fds.end(); it++) {
			if(boost::filesystem::is_directory(*it)) {
				if(dirs) {
					if(mode == ENTIRE_FILE_PATH) {
						dirs->push_back(it->string());
					} else {
						dirs->push_back(it->filename().string());
					}
				}
			} else {
				if(files) {
					if(mode == ENTIRE_FILE_PATH) {
						files->push_back(it->string());
					} else {
						files->push_back(it->filename().string());
					}
				}
			}
		}
	}
}

void get_unique_filenames_under_dir(const std::string& dir,
                                    std::map<std::string, std::string>* file_map,
									const std::string& prefix)
{
	if(dir.size() > 1024) {
		return;
	}

	std::vector<std::string> files;
	std::vector<std::string> dirs;
	get_files_in_dir(dir, &files, &dirs);
	for(std::vector<std::string>::const_iterator i = files.begin();
	    i != files.end(); ++i) {
		(*file_map)[prefix + *i] = dir + "/" + *i;
	}

	for(std::vector<std::string>::const_iterator i = dirs.begin();
	    i != dirs.end(); ++i) {
		get_unique_filenames_under_dir(dir + "/" + *i, file_map, prefix);
	}
}

std::string get_dir(const std::string& dir)
{
	boost::filesystem::path p(dir);
	if(boost::filesystem::exists(p)) {
		if(boost::filesystem::is_directory(p)) {
			return dir;
		}
		// exists, but not directory.
		return std::string();
	}
	// Try creating it.
	if(boost::filesystem::create_directories(p)) {
		return dir;
	}
	return std::string();
}

std::string read_file(const std::string& name)
{
	std::string fname = find_file(name);
	std::ifstream file(fname.c_str(),std::ios_base::binary);
	std::stringstream ss;
	ss << file.rdbuf();
	return ss.str();
}

void write_file(const std::string& fname, const std::string& data)
{
	bool absolute_path = fname[0] == '/' ? true : false;
	//Try to ensure that the dir the file is in exists.
	std::vector<std::string> components;
	boost::split(components, fname, std::bind2nd(std::equal_to<char>(), '/'));
	boost::filesystem::path p(fname);
	ASSERT_LOG(get_dir(p.parent_path().string()) != "", "Couldn't create directory: " << p.parent_path().string());

	//Write the file.
	std::ofstream file(fname.c_str(),std::ios_base::binary);
	file << data;
}

std::string find_file(const std::string& name)
{
	if(boost::filesystem::exists(boost::filesystem::path(name))) {
		return name;
	}
	return std::string();
}

int64_t file_mod_time(const std::string& fname)
{
	return boost::filesystem::last_write_time(boost::filesystem::path(fname));
}

void move_file(const std::string& from, const std::string& to)
{
	boost::filesystem::rename(boost::filesystem::path(from), boost::filesystem::path(to));
}

void remove_file(const std::string& fname)
{
	boost::filesystem::remove(boost::filesystem::path(fname));
}

void copy_file(const std::string& from, const std::string& to)
{
	boost::filesystem::copy_file(boost::filesystem::path(from), boost::filesystem::path(to));
}

bool is_path_absolute(const std::string& path)
{
	static const std::string re_absolute_path = "^(?:(?:(?:[A-Za-z]:)?(?:\\\\|/))|\\\\\\\\|/).*";
	return boost::regex_match(path, boost::regex(re_absolute_path));
}

std::string make_conformal_path(const std::string& path)
{
	std::string new_path(path);
	std::replace(new_path.begin(), new_path.end(),'\\','/');
	new_path = boost::regex_replace(new_path, boost::regex("//"), "/",  boost::match_default | boost::format_all);
	if(new_path[new_path.length()-1] == '/') {
		new_path.erase(new_path.length()-1);
	}
	return new_path;
}

std::string del_substring_front(const std::string& target, const std::string& common)
{
	if(boost::iequals(target.substr(0, common.length()), common)) {
	//if(target.find(common) == 0) {
		return target.substr(common.length());
	}
	return target;
}

std::string normalise_path(const std::string& path)
{
	if(is_path_absolute(path)) { 
		return path;
	}
	std::vector<std::string> cur_path;
	std::string norm_path;
	boost::split(cur_path, path, std::bind2nd(std::equal_to<char>(), '/'));
	foreach(const std::string& s, cur_path) {
		if(s != ".") {
			norm_path += s + "/";
		}
	}
	return norm_path;
}

std::string compute_relative_path(const std::string& source, const std::string& target)
{
	//std::cerr << "compute_relative_path(a): " << source << " : " << target << std::endl;
	std::string common_part = normalise_path(source);
	std::string back;
	if(common_part.length() > 1 && common_part[common_part.length()-1] == '/') {
		common_part.erase(common_part.length()-1);
	}
	while(boost::iequals(del_substring_front(target, common_part), target)) {
		size_t offs = common_part.rfind('/');
		//std::cerr << "compute_relative_path(b2): " << back << " : " << common_part << std::endl;
		if(common_part.length() > 1 && offs != std::string::npos) {
			common_part.erase(offs);
			back = "../" + back;
		} else {
			break;
		}
	}
	common_part = del_substring_front(target, common_part);
	if(common_part.length() == 1) {
		common_part = common_part.substr(1);
		if(back.empty() == false) {
			back.erase(back.length()-1);
		}
	} else if(common_part.length() > 1 && common_part[0] == '/') {
		common_part = common_part.substr(1);
	} else {
		if(back.empty() == false) {
			back.erase(back.length()-1);
		}
	}
	//std::cerr << "compute_relative_path(b): " << back << " : " << common_part << std::endl;
	return back + common_part;
}

}

UNIT_TEST(absolute_path_test1) {
	CHECK_EQ(sys::is_path_absolute("images"), false);
	CHECK_EQ(sys::is_path_absolute("images/"), false);
	CHECK_EQ(sys::is_path_absolute("./images"), false);
	CHECK_EQ(sys::is_path_absolute("/home"), true);
	CHECK_EQ(sys::is_path_absolute("/home/worker"), true);
	CHECK_EQ(sys::is_path_absolute("c:\\home"), true);
	CHECK_EQ(sys::is_path_absolute("c:\\"), true);
	CHECK_EQ(sys::is_path_absolute("\\"), true);
	CHECK_EQ(sys::is_path_absolute("\\home"), true);
	CHECK_EQ(sys::is_path_absolute("\\\\.\\"), true);
	CHECK_EQ(sys::is_path_absolute("\\\\unc\\test"), true);
	CHECK_EQ(sys::is_path_absolute("c:/home"), true);
	CHECK_EQ(sys::is_path_absolute("c:/"), true);
}
