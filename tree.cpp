#include <iostream>
#include <vector>
#include <string>
#include <algorithm>
#include <cstring>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>
#include <getopt.h>
#include <cstdlib>
#include <sstream>

struct TreeOptions {
    bool show_hidden;
    bool dir_only;
    bool full_path;
    int max_depth;
    std::string start_path;
    
    TreeOptions() : show_hidden(false), dir_only(false), full_path(false), max_depth(-1), start_path(".") {}
};

void print_help() {
    std::cout << "Usage: tree [OPTIONS] [DIRECTORY]\n";
    std::cout << "Display directory contents in a tree-like format.\n\n";
    std::cout << "Options:\n";
    std::cout << "  -a, --all         Show hidden files (those starting with '.')\n";
    std::cout << "  -d, --dir-only    List directories only\n";
    std::cout << "  -f, --full-path   Print the full path prefix for each file\n";
    std::cout << "  -L, --level N     Descend only N levels deep\n";
    std::cout << "  -h, --help        Display this help message\n";
}

bool is_directory(const std::string& path) {
    struct stat statbuf;
    if (stat(path.c_str(), &statbuf) != 0)
        return false;
    return S_ISDIR(statbuf.st_mode);
}


bool is_symlink(const std::string& path) {
    struct stat statbuf;
    if (lstat(path.c_str(), &statbuf) != 0)
        return false;
    return S_ISLNK(statbuf.st_mode);
}


std::string read_symlink(const std::string& path) {
    char buf[1024];
    ssize_t len = readlink(path.c_str(), buf, sizeof(buf)-1);
    if (len == -1)
        return "";
    buf[len] = '\0';
    return std::string(buf);
}


bool is_dir_entry(const std::string& base, const std::string& name) {
    std::string full_path = base + "/" + name;
    return is_directory(full_path);
}


bool compare_entries(const std::string& a, const std::string& b, const std::string& base) {
    bool a_is_dir = is_dir_entry(base, a);
    bool b_is_dir = is_dir_entry(base, b);
    if (a_is_dir != b_is_dir) {
        return a_is_dir > b_is_dir;
    }
    return a < b;
}


std::vector<std::string> get_directory_entries(const std::string& path, bool show_hidden) {
    std::vector<std::string> entries;
    DIR* dir = opendir(path.c_str());
    if (dir == NULL) {
        return entries;
    }

    struct dirent* entry;
    while ((entry = readdir(dir)) != NULL) {
        std::string name = entry->d_name;
        if (name == "." || name == "..")
            continue;
        if (!show_hidden && name[0] == '.')
            continue;
        entries.push_back(name);
    }
    closedir(dir);

    for (size_t i = 0; i < entries.size(); i++) {
        for (size_t j = i + 1; j < entries.size(); j++) {
            if (compare_entries(entries[j], entries[i], path)) {
                std::swap(entries[i], entries[j]);
            }
        }
    }

    return entries;
}

std::string get_tree_prefix(const std::vector<bool>& is_last, bool is_dir) {
    std::string prefix;
    for (size_t i = 0; i < is_last.size() - 1; ++i) {
        prefix += is_last[i] ? "    " : "│   ";
    }
    prefix += is_last.back() ? "└── " : "├── ";
    
    if (is_dir) {
        prefix = "\033[1;34m" + prefix + "\033[0m"; // Blue for directories
    }
    
    return prefix;
}

void print_tree(const std::string& path, TreeOptions& options, 
                std::vector<bool> is_last, int depth) {
    if (options.max_depth != -1 && depth > options.max_depth) {
        return;
    }

    std::vector<std::string> entries = get_directory_entries(path, options.show_hidden);
    
    for (size_t i = 0; i < entries.size(); ++i) {
        const std::string& entry = entries[i];
        std::string full_path = path + "/" + entry;
        bool entry_is_dir = is_directory(full_path);
        
        if (options.dir_only && !entry_is_dir) {
            continue;
        }
        
        if (!is_last.empty()) {
            is_last.back() = (i == entries.size() - 1);
        } else {
            is_last.push_back(i == entries.size() - 1);
        }
        
        std::string display_name;
        if (options.full_path) {
            display_name = full_path;
        } else {
            display_name = entry;
        }
        
        if (is_symlink(full_path)) {
            std::string target = read_symlink(full_path);
            display_name += " -> " + target;
        }
        
        std::cout << get_tree_prefix(is_last, entry_is_dir) << display_name << "\n";
        
        if (entry_is_dir && !is_symlink(full_path)) {
            is_last.push_back(true); // Will be updated in the recursive call
            print_tree(full_path, options, is_last, depth + 1);
            is_last.pop_back();
        }
    }
}

void print_tree_wrapper(const std::string& path, TreeOptions& options) {
    std::vector<bool> is_last;
    print_tree(path, options, is_last, 0);
}

int string_to_int(const std::string& s) {
    std::istringstream iss(s);
    int result;
    iss >> result;
    return result;
}

int main(int argc, char* argv[]) {
    TreeOptions options;
    
    static struct option long_options[] = {
        {"all", no_argument, 0, 'a'},
        {"dir-only", no_argument, 0, 'd'},
        {"full-path", no_argument, 0, 'f'},
        {"level", required_argument, 0, 'L'},
        {"help", no_argument, 0, 'h'},
        {0, 0, 0, 0}
    };
    
    int c;
    while ((c = getopt_long(argc, argv, "adfL:h", long_options, NULL)) != -1) {
        switch (c) {
            case 'a':
                options.show_hidden = true;
                break;
            case 'd':
                options.dir_only = true;
                break;
            case 'f':
                options.full_path = true;
                break;
            case 'L':
                options.max_depth = string_to_int(optarg);
                break;
            case 'h':
                print_help();
                return 0;
            case '?':
                std::cerr << "Invalid option. Use -h for help.\n";
                return 1;
        }
    }
    
    // Get the starting directory
    if (optind < argc) {
        options.start_path = argv[optind];
    }
    
    // Check if the path exists
    if (access(options.start_path.c_str(), F_OK) != 0) {
        std::cerr << "Error: Path '" << options.start_path << "' does not exist.\n";
        return 1;
    }
    
    // Print the tree
    std::cout << options.start_path << "\n";
    print_tree_wrapper(options.start_path, options);
    
    return 0;
}