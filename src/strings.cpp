
#include <boost/algorithm/string/replace.hpp>

namespace Element {
static void escape_regex (std::string& input)
{
    boost::replace_all (input, "\\", "\\\\");
    boost::replace_all (input, "^", "\\^");
    boost::replace_all (input, ".", "\\.");
    boost::replace_all (input, "$", "\\$");
    boost::replace_all (input, "|", "\\|");
    boost::replace_all (input, "(", "\\(");
    boost::replace_all (input, ")", "\\)");
    boost::replace_all (input, "{", "\\{");
    boost::replace_all (input, "{", "\\}");
    boost::replace_all (input, "[", "\\[");
    boost::replace_all (input, "]", "\\]");
    boost::replace_all (input, "*", "\\*");
    boost::replace_all (input, "+", "\\+");
    boost::replace_all (input, "?", "\\?");
    boost::replace_all (input, "/", "\\/");
}

static void transform_wildcard (std::string& input)
{
    boost::replace_all (input, "\\?", ".");
    boost::replace_all (input, "\\*", ".*");
}

std::string wildcard_to_regex (const std::string& wildcard)
{
    auto input = wildcard;
    escape_regex (input);
    transform_wildcard (input);
    return input;
}

} // namespace Element