/**
 * This API assumes that the file\s accessed have been parsed by
 * the parser with no errors.
 * Use of this API on a file with possible or confirmed errors is UB
 * ----------------
 * Written in C++17
*/

#pragma once
#include <string>
#include <fstream>

#include <type_traits>
#include <vector>
#include <array>

#define SIMPLEDATA_API_VERSION "3.0"

bool stob(const std::string& string)
{
    if (string == "true") return true;
    else return false;
}

namespace simpledata
{
    struct errorcodes
    {
        int code;
        std::string desc;
    };

    const errorcodes errstr_data[] = {
        {1, "Invalid File"},
        {2, "Identifier Not Found"},
        {3, "Invalid Type"},
        {4, "Invalid Template Argument"}
    };

    class api
    {
        private:
            int err;
            std::ifstream src;
            std::string file_name;

            static void remove_leading(std::string& string)
            {
                std::string buffer;

                // Find the position that the leading whitespace ends
                int start;
                for (start = 0; start < string.size() && string[start] == ' '; start++);

                // Copy everything to the buffer, starting from the end of the whitespace
                for (int i = start; i < string.size(); i++) buffer += string[i];
                string = buffer;
            }

            static void remove_trailing(std::string& string)
            {
                bool trailing = true;
                std::string buffer = string;

                for (int i = string.size() - 1; i >= 0; i--)
                {
                    if (string[i] != ' ') trailing = false;
                    if (!trailing) buffer[i] = string[i];
                }
                string = buffer;
            }

            // Used in fetch()
            static std::string decomment(const std::string string, const std::string type = "other")
            {
                std::string buffer;

                // Character we want to look for - brackets only for readability
                char lookfor = (type == "string") ? '\"' : '#';

                int i = (type == "string") ? 1 : 0;
                if (i == 1) buffer += string[0];

                for (; i < string.size(); i++)
                {
                    if (string[i] == lookfor)
                    {
                        if (lookfor != '#') lookfor = '#';
                        else break;
                    }
                    buffer += string[i];
                }

                simpledata::api::remove_trailing(buffer);
                return buffer;
            }

            // Used in update()
            static std::string preserve_comment(const std::string string)
            {
                std::string comment = "";
                bool found_comment = false;

                char lookfor = (string[0] == '\'' || string[0] == '\"') ? string[0] : '#';
                for (int i = (lookfor == string[0]) ? 1 : 0; i < string.size(); i++)
                {
                    if (string[i] == lookfor)
                    {
                        if (lookfor == '#') found_comment = true;
                        else lookfor = '#';
                    }
                    if (found_comment) comment += string[i];
                }

                return comment;
            }

            // Returns the characters outside of the allotted area - used in update()
            static std::string outside_substr(const char first, const char last, const std::string string)
            {
                int start_index = 0;
                char lookfor = first;
                bool first_found = false, last_found = false, print = true;

                std::string buffer = "";
                for (int i = 0; i < string.size(); i++)
                {
                    if (string[i] == first) first_found = true;
                    else if (string[i] == last) last_found = true;

                    if (!first_found || last_found) buffer += string[i];
                }

                return buffer;
            }

        public:
            std::string errstr()
            {
                for (int i = 0; i < sizeof(simpledata::errstr_data) / sizeof(simpledata::errstr_data[0]); i++)
                {
                    if (err == simpledata::errstr_data[i].code) return simpledata::errstr_data[i].desc;
                }
                return "Not An Error";
            }

            static std::string errfind(const int errcode)
            {
                for (int i = 0; i < sizeof(simpledata::errstr_data) / sizeof(simpledata::errstr_data[0]); i++)
                {
                    if (errcode == simpledata::errstr_data[i].code) return simpledata::errstr_data[i].desc;
                }
                return "Not An Error";
            }

            void open(const std::string file)
            {
                file_name = file;
            }

            // Constexpr makes conversions go weird, so we won't rely as much on template functions (std::is_convertible) for them
            template <typename T>
            int fetch(const std::string identifier, T& value_dest, const std::string type)
            {
                if (err != 1)
                {
                    std::ifstream copy(file_name);

                    bool found = false;
                    while (!copy.eof())
                    {
                        int id_end = 0;
                        std::string line;

                        std::getline(copy, line);
                        if (line != "" && line != "\n" && line[0] != '#')
                        {
                            std::string id_buffer, val_buffer;

                            // Identifier
                            for (; line[id_end] != ':'; id_end++) id_buffer += line[id_end];

                            simpledata::api::remove_leading(id_buffer);
                            simpledata::api::remove_trailing(id_buffer);

                            if (identifier != id_buffer) continue;
                            else found = true;

                            // Value
                            for (int i = id_end + 2; i < line.size(); i++) val_buffer += line[i];

                            simpledata::api::remove_leading(val_buffer);
                            simpledata::api::remove_trailing(val_buffer);

                            try
                            {
                                if constexpr (std::is_convertible<T, std::string>::value)
                                {
                                    if (type == "string")
                                    {
                                        std::string str_buffer, decommented = simpledata::api::decomment(val_buffer, "string");

                                        for (int i = 1; i < decommented.size() && decommented[i] != '\"'; i++) str_buffer += decommented[i];
                                        value_dest = str_buffer;
                                    }
                                    else
                                    {
                                        err = 4;

                                        copy.close();
                                        return err;
                                    }
                                }
                                else if constexpr (std::is_convertible<T, std::vector<std::string> >::value)
                                {
                                    if (type == "array")
                                    {
                                        std::string buffer_str = "";

                                        // Boolean flags to smother false positives
                                        bool str = false, ch = false;
                                        for (int i = 1; i < val_buffer.size(); i++)
                                        {
                                            if (val_buffer[i] != ',')
                                            {
                                                if (val_buffer[i] == '\"')
                                                {
                                                    if (str) str = false;
                                                    else str = true;
                                                }
                                                else if (val_buffer[i] == '\'')
                                                {
                                                    if (ch) ch = false;
                                                    else ch = true;
                                                }
                                                if (val_buffer[i] == ']' && !str && !ch)
                                                {
                                                    simpledata::api::remove_leading(buffer_str);
                                                    simpledata::api::remove_trailing(buffer_str);

                                                    // Removing single quotes and double quotes
                                                    if (buffer_str[0] == '\"' || buffer_str[0] == '\'')
                                                    {
                                                        std::string decommented = simpledata::api::decomment(buffer_str), b = "";
                                                        for (int j = 1; j < decommented.size() - 1; j++) b += decommented[j];
                                                        buffer_str = b;
                                                    }
                                                    value_dest.push_back(buffer_str);
                                                    break;
                                                }
                                                buffer_str += val_buffer[i];
                                            }
                                            else
                                            {
                                                simpledata::api::remove_leading(buffer_str);
                                                simpledata::api::remove_trailing(buffer_str);

                                                // Removing single and double quotes if present
                                                if (buffer_str[0] == '\"' || buffer_str[0] == '\'')
                                                {
                                                    std::string decommented = simpledata::api::decomment(buffer_str), b = "";
                                                    for (int j = 1; j < decommented.size() - 1; j++) b += decommented[j];
                                                    buffer_str = b;
                                                }  
                                                value_dest.push_back(buffer_str);
                                                buffer_str = "";
                                            }
                                        }
                                    }
                                    else
                                    {
                                        err = 4;

                                        copy.close();
                                        return err;
                                    }
                                }
                                else
                                {
                                    switch(type[0])
                                    {
                                        case 'i':
                                            value_dest = stoi(simpledata::api::decomment(val_buffer));
                                        break;
                                        case 'b':
                                            value_dest = stob(simpledata::api::decomment(val_buffer));
                                        break;
                                        case 'f':
                                            value_dest = stof(simpledata::api::decomment(val_buffer));
                                        break;
                                        case 'c':
                                            value_dest = val_buffer[1];
                                        break;
                                        default:
                                            err = 3;

                                            copy.close();
                                            return err;
                                        break;
                                    }
                                }
                            }
                            catch (std::invalid_argument e)
                            {
                                copy.close();
                                err = 3;

                                return err;
                            }
                            break;
                        }
                    }
                    if (!found) err = 2;
                    else err = 0;

                    copy.close();
                    return err;
                }

                // Just to keep the warnings away, will never actually execute
                return 0;
            }

            template <typename T>
            int update(const std::string identifier, const T new_val, const std::string type)
            {
                if (err != 1)
                {
                    std::ifstream copy(file_name);
                    std::ofstream out(".simpdat_buf.simpdat");

                    // Only used to determine the function's return value
                    bool found = false;

                    while (!copy.eof())
                    {
                        std::string line;
                        std::getline(copy, line);

                        if (line != "" && line != "\n" && line[0] != '#')
                        {
                            std::string current_identifier;

                            // Parse the identifier for the line
                            for (int i = 0; line[i] != ':'; i++) current_identifier += line[i];

                            if (current_identifier == identifier)
                            {
                                std::string value = outside_substr(line[0], ':', line);
                                if constexpr (std::is_convertible<T, bool>::value)
                                {
                                    if (type == "bool" || type == "boolean")
                                    {
                                        if (new_val) out << identifier << ": true";
                                        else out << identifier << ": false";
                                    }
                                }
                                else if constexpr (std::is_convertible<T, std::vector<std::string> >::value)
                                {
                                    if (type == "array")
                                    {
                                        out << identifier << ": [";
                                        for (int i = 0; i < new_val.size(); i++)
                                        {
                                            std::string output = "";
                                            if (new_val[i].size() == 1 && !(new_val[i] >= "0" && new_val[i] <= "9"))
                                            {
                                                output = '\'' + new_val[i] + '\'';
                                            }
                                            else
                                            {
                                                int numerals = 0;
                                                for (int j = 0; j < new_val[i].size(); j++)
                                                {
                                                    if ((new_val[i][j] >= '0' && new_val[i][j] <= '9') || new_val[i][j] == '.') numerals++;
                                                }
                                                if (numerals == new_val[i].size() || new_val[i] == "true" || new_val[i] == "false")
                                                {
                                                    output = new_val[i];
                                                }
                                                else
                                                {
                                                    output = '\"' + new_val[i] + '\"';
                                                }
                                            }

                                            out << output;
                                            if (i != new_val.size() - 1) out << ", ";
                                        }
                                        out << ']';
                                    }
                                    else
                                    {
                                        err = 4;
                                        return err;
                                    }
                                }
                                else if (type == "char")
                                {
                                    out << identifier << ": \'" << new_val << "\'";
                                }
                                else if (type == "string")
                                {
                                    out << identifier << ": \"" << new_val << "\"";
                                }
                                else if (type == "int" || type == "integer" || type == "float")
                                {
                                    out << identifier << ": " << new_val;
                                }
                                else
                                {
                                    copy.close();
                                    out.close();

                                    // Delete ".simpdat_buf.simpdat" file
                                    #if defined(__unix) || defined(unix__) || defined(__linux) || (defined(__APPLE__) && defined(__MACH__))
                                        system("rm .simpdat_buf.simpdat");
                                    #elif defined(__WIN32)
                                        system("del /f .simpdat_buf.simpdat");
                                    #endif

                                    return 3;
                                }

                                out << " " << simpledata::api::preserve_comment(value);
                                found = true;
                            }
                            else
                            {
                                out << line;
                            }
                        }
                        else
                        {
                            out << line;
                        }
                        if (!copy.eof()) out << "\n";
                    }
                    copy.close();
                    out.close();

                    std::rename(".simpdat_buf.simpdat", file_name.c_str());

                    if (!found) err = 2;
                    else err = 0;

                    return err;
                }
                return err;
            }

            // Static versions of update() and fetch()
            // errfind() will have to be used to get the error code descriptions of these functions
            template <typename T>
            static int fetch(const std::string identifier, T& value_dest, const std::string filename, const std::string type)
            {
                std::ifstream file(filename);
                bool found = false;

                while (!file.eof())
                {
                    std::string line;
                    getline(file, line);

                    if (line != "" && line != "\n" && line[0] != '#')
                    {
                        std::string current_identifier, val_buffer;

                        int id_end = 0;
                        for (; id_end < line.size() && line[id_end] != ':'; id_end++) current_identifier += line[id_end];

                        simpledata::api::remove_leading(current_identifier);
                        simpledata::api::remove_trailing(current_identifier);

                        if (identifier != current_identifier) continue;
                        else found = true;

                        // getting the value
                        for (int i = id_end + 1; i < line.size(); i++) val_buffer += line[i];

                        simpledata::api::remove_leading(val_buffer);

                        try
                        {
                            if constexpr (std::is_convertible<T, std::string>::value)
                            {
                                if (type == "string")
                                {
                                    std::string str_buffer, decommented = simpledata::api::decomment(val_buffer, "string");

                                    for (int i = 1; i < decommented.size() && decommented[i] != '\"'; i++) str_buffer += decommented[i];
                                    value_dest = str_buffer;
                                }
                                else
                                {
                                    file.close();
                                    return 4;
                                }
                            }
                            else if constexpr (std::is_convertible<T, std::vector<std::string> >::value)
                            {
                                if (type == "array")
                                {
                                    std::string buffer_str = "";

                                    // Boolean flags to smother false positives
                                    bool str = false, ch = false;
                                    for (int i = 1; i < val_buffer.size(); i++)
                                    {
                                        if (val_buffer[i] != ',')
                                        {
                                            if (val_buffer[i] == '\"')
                                            {
                                                if (str) str = false;
                                                else str = true;
                                            }
                                            else if (val_buffer[i] == '\'')
                                            {
                                                if (ch) ch = false;
                                                else ch = true;
                                            }
                                            if (val_buffer[i] == ']' && !str && !ch)
                                            {
                                                simpledata::api::remove_leading(buffer_str);
                                                simpledata::api::remove_trailing(buffer_str);

                                                // Removing single and double quotes if present
                                                if (buffer_str[0] == '\'' || buffer_str[0] == '\"')
                                                {
                                                    std::string decommented = simpledata::api::decomment(buffer_str), b = "";
                                                    for (int j = 1; j < decommented.size() - 1; j++) b += decommented[j];
                                                    buffer_str = b;
                                                }
                                                value_dest.push_back(buffer_str);
                                                break;
                                            }

                                            buffer_str += val_buffer[i];
                                        }
                                        else
                                        {
                                            simpledata::api::remove_leading(buffer_str);
                                            simpledata::api::remove_trailing(buffer_str);

                                            if (buffer_str[0] == '\"' || buffer_str[0] == '\'')
                                            {
                                                std::string decommented = simpledata::api::decomment(buffer_str), b = "";
                                                for (int j = 1; j < decommented.size() - 1; j++) b += decommented[j];
                                                buffer_str = b;
                                            }

                                            value_dest.push_back(buffer_str);
                                            buffer_str = "";
                                        }
                                    }
                                }
                                else
                                {
                                    file.close();
                                    return 4;
                                }
                            }
                            else
                            {
                                switch(type[0])
                                {
                                    case 'i':
                                        value_dest = stoi(simpledata::api::decomment(val_buffer));
                                    break;
                                    case 'b':
                                        value_dest = stob(simpledata::api::decomment(val_buffer));
                                    break;
                                    case 'f':
                                        value_dest = stof(simpledata::api::decomment(val_buffer));
                                    break;
                                    case 'c':
                                        value_dest = val_buffer[1];
                                    break;
                                    default:
                                        file.close();
                                        return 3;
                                    break;
                                }
                            }
                        }
                        catch (std::invalid_argument e)
                        {
                            file.close();
                            return 3;
                        }
                        break;
                    }
                }
                file.close();

                if (!found) return 2;
                else return 0;
            }

            template <typename T>
            static int update(const std::string identifier, const T new_val, const std::string filename, const std::string type)
            {
                std::ifstream file(filename);
                std::ofstream out(".simpdat_buf.simpdat");
                bool found = false;

                while (!file.eof())
                {
                    std::string line;
                    getline(file, line);

                    if (line != "" && line != "\n" && line[0] != '#')
                    {
                        std::string current_identifier;

                        // Parse the identifier for the line
                        for (int i = 0; line[i] != ':'; i++) current_identifier += line[i];

                        if (current_identifier == identifier)
                        {
                            std::string value = outside_substr(line[0], ':', line);
                            if constexpr (std::is_convertible<T, bool>::value)
                            {
                                if (type == "bool" || type == "boolean")
                                {
                                    if (new_val) out << identifier << ": true";
                                    else out << identifier << ": false";
                                }
                            }
                            else if constexpr (std::is_convertible<T, std::vector<std::string> >::value)
                            {
                                if (type == "array")
                                {
                                    out << identifier << ": [";
                                    for (int i = 0; i < new_val.size(); i++)
                                    {
                                        std::string output = "";
                                        if (new_val[i].size() == 1 && !(new_val[i] >= "0" && new_val[i] <= "9"))
                                        {
                                            output = '\'' + new_val[i] + '\'';
                                        }
                                        else
                                        {
                                            int numerals = 0;
                                            for (int j = 0; j < new_val[i].size(); j++)
                                            {
                                                if ((new_val[i][j] >= '0' && new_val[i][j] <= '9') || new_val[i][j] == '.') numerals++;
                                            }
                                            if (numerals == new_val[i].size() || new_val[i] == "true" || new_val[i] == "false")
                                            {
                                                output = new_val[i];
                                            }
                                            else
                                            {
                                                output = '\"' + new_val[i] + '\"';
                                            }
                                        }

                                        out << output;
                                        if (i != new_val.size() - 1) out << ", ";
                                    }
                                    out << ']';
                                }
                                else
                                {
                                    return 4;
                                }
                            }
                            else if (type == "char")
                            {
                                out << identifier << ": \'" << new_val << "\'";
                            }
                            else if (type == "string")
                            {
                                out << identifier << ": \"" << new_val << "\"";
                            }
                            else if (type == "int" || type == "integer" || type == "float")
                            {
                                out << identifier << ": " << new_val;
                            }
                            else
                            {
                                file.close();
                                out.close();

                                // Delete ".simpdat_buf.simpdat" file
                                #if defined(__unix) || defined(unix__) || defined(__linux) || (defined(__APPLE__) && defined(__MACH__))
                                    system("rm .simpdat_buf.simpdat");
                                #elif defined(__WIN32)
                                    system("del /f .simpdat_buf.simpdat");
                                #endif

                                return 3;
                            }

                            out << " " << simpledata::api::preserve_comment(value);
                            found = true;
                        }
                        else
                        {
                            out << line;
                        }
                    }
                    else
                    {
                        out << line;
                    }
                    if (!file.eof()) out << "\n";
                }
                file.close();
                out.close();

                std::rename(".simpdat_buf.simpdat", filename.c_str());
                return found ? 0 : 2;
            }

            static std::string version() { return SIMPLEDATA_API_VERSION; }

            api(const std::string& filename = "")
            {
                if (filename != "") open(filename);
            }
    };
};
