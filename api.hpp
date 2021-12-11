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

#define SIMPLEDATA_API_VERSION "1.0"

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
        {3, "Invalid Type"}
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

            // Constexpr makes conversions go weird, so we won't rely as much on template functions (std::is_convertible) for that
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

                            // No need to parse the value if the identifier isn't the one being searched for
                            if (identifier != id_buffer) continue;
                            else found = true;

                            // Value
                            for (int i = id_end + 2; i < line.size(); i++) val_buffer += line[i];

                            simpledata::api::remove_leading(val_buffer);
                            simpledata::api::remove_trailing(val_buffer);

                            try
                            {
                                if (type == "int")
                                {
                                    value_dest = stoi(val_buffer);
                                }
                                else if (type == "bool")
                                {
                                    value_dest = stob(val_buffer);
                                }
                                else if (type == "float")
                                {
                                    value_dest = stof(val_buffer);
                                }
                                else if (type == "char")
                                {
                                    value_dest = val_buffer[1];
                                }
                                else if constexpr (std::is_convertible<T, std::string>::value)
                                {
                                    if (type == "string")
                                    {
                                        std::string str_buffer;

                                        for (int i = 1; i < val_buffer.size() && val_buffer[i] != '"'; i++) str_buffer += val_buffer[i];
                                        value_dest = str_buffer;
                                    }
                                }
                                else
                                {
                                    err = 3;
                                    copy.close();

                                    return err;
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
                                if (type == "bool" || type == "boolean")
                                {
                                    if (new_val) out << identifier << ": true";
                                    else out << identifier << ": false";
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
                        std::string current_identifier, value_buffer;

                        int id_end = 0;
                        for (; id_end < line.size() && line[id_end] != ':'; id_end++) current_identifier += line[id_end];

                        simpledata::api::remove_leading(current_identifier);
                        simpledata::api::remove_trailing(current_identifier);

                        if (identifier != current_identifier) continue;
                        else found = true;

                        // getting the value
                        for (int i = id_end + 1; i < line.size(); i++) value_buffer += line[i];

                        simpledata::api::remove_leading(value_buffer);
                        simpledata::api::remove_trailing(value_buffer);

                        try
                        {
                            if (type == "int")
                            {
                                value_dest = stoi(value_buffer);
                            }
                            else if (type == "bool")
                            {
                                value_dest = stob(value_buffer);
                            }
                            else if (type == "float")
                            {
                                value_dest = stof(value_buffer);
                            }
                            else if (type == "char")
                            {
                                value_dest = value_buffer[1];
                            }
                            else if constexpr (std::is_convertible<T, std::string>::value)
                            {
                                if (type == "string")
                                {
                                    std::string str_buffer;

                                    for (int i = 1; i < value_buffer.size() && value_buffer[i] != '"'; i++) str_buffer += value_buffer[i];
                                    value_dest = str_buffer;
                                }
                            }
                            else
                            {
                                file.close();
                                return 3;
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
                            if (type == "bool" || type == "boolean")
                            {
                                if (new_val) out << identifier << ": true";
                                else out << identifier << ": false";
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
