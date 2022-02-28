
#pragma once

#include <algorithm>
#include <filesystem>
#include <regex>

namespace element {

extern std::string wildcard_to_regex (const std::string& wildcard);

class SearchPath {
public:
    SearchPath() = default;
    ~SearchPath() = default;

    SearchPath (const SearchPath& o) { operator= (o); }
    SearchPath (SearchPath&& o) { operator= (o); }

    SearchPath& operator= (const SearchPath& o)
    {
        std::copy (o.paths.begin(), o.paths.end(), this->paths.begin());
        return *this;
    }

    SearchPath& operator= (SearchPath&& o)
    {
        this->paths = std::move (o.paths);
        return *this;
    }

    template <typename Tx>
    void add (Tx&& path)
    {
        paths.push_back (path);
        paths.back().make_preferred();
    }

    void clear() noexcept
    {
        paths.clear();
    }

    std::vector<std::filesystem::path> find_folders (bool recursive, const std::string& wildcard = "*") const
    {
        auto regex = wildcard.size() > 0 && wildcard != "*"
                         ? wildcard_to_regex (wildcard)
                         : "";
        return recursive ? std::move (find_folders_regex<std::filesystem::recursive_directory_iterator> (regex))
                         : std::move (find_folders_regex<std::filesystem::directory_iterator> (regex));
    }

    auto begin() const noexcept { return paths.begin(); }
    auto end() const noexcept { return paths.end(); }

private:
    std::vector<std::filesystem::path> paths;

    template <class Iter>
    std::vector<std::filesystem::path> find_folders_regex (const std::string& pattern) const
    {
        namespace fs = std::filesystem;
        std::vector<std::filesystem::path> results;
        std::function<bool (const std::filesystem::directory_entry& entry)> match;

        try {
            std::regex reg (pattern);
            if (pattern == "*" || pattern.empty())
                match = [] (const std::filesystem::directory_entry&) -> bool { return true; };
            else
                match = [=, &reg] (const std::filesystem::directory_entry& entry) -> bool {
                    return std::regex_match (entry.path().filename().string(), reg);
                };

            for (const auto& dir : paths) {
                std::filesystem::path path (dir);
                if (std::filesystem::exists (path) && std::filesystem::is_directory (path)) {
                    for (auto const& entry : Iter (path)) {
                        if (entry.is_directory() && match (entry))
                            results.push_back (entry.path());
                    }
                }
            }
        } catch (const std::regex_error& e) {
            // noop
        }

        return results;
    }
};

} // namespace element
