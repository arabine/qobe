#include <iostream>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <regex>
#include <unordered_map>

namespace fs = std::filesystem;

static const char *defaultHeader = R"(
<h1>D8S EURL blog</h1>

)";

static std::string customHeader;

// Default CSS file 
static const char *defaultCssFile = R"(
body {
    font-family: Arial, sans-serif;
    line-height: 1.6;
    color: #333;
    background-color: #f9f9f9;
    margin: 0;
    padding: 0;
}

h1, h2, h3, h4, h5, h6 {
    color: #222;
    margin-top: 20px;
    margin-bottom: 10px;
}

h1 {
    font-size: 2em;
}

h2 {
    font-size: 1.75em;
}

h3 {
    font-size: 1.5em;
}

p {
    margin: 10px 0;
}

a {
    color: #007BFF;
    text-decoration: none;
}

a:hover {
    text-decoration: underline;
}

ul {
    margin: 10px 0;
    padding-left: 20px;
}

li {
    margin: 5px 0;
}

img {
    max-width: 100%;
    height: auto;
    margin: 10px 0;
    border: 1px solid #ccc;
    padding: 5px;
    background-color: #fff;
}

pre {
    background: #f4f4f4;
    padding: 10px;
    border: 1px solid #ddd;
    overflow-x: auto;
}

code {
    font-family: "Courier New", Courier, monospace;
    background: #f4f4f4;
    padding: 2px 4px;
    border-radius: 4px;
}

.container {
    max-width: 800px;
    margin: 20px auto;
    background: #fff;
    padding: 20px;
    box-shadow: 0 2px 5px rgba(0, 0, 0, 0.1);
    border-radius: 5px;
}


/* Table styling */
table {
    width: 100%;
    border-collapse: collapse;
    margin: 20px 0;
    font-size: 1em;
    font-family: 'Arial', sans-serif;
    min-width: 400px;
    box-shadow: 0 0 20px rgba(0, 0, 0, 0.15);
}

table thead tr {
    background-color: #009879;
    color: #ffffff;
    text-align: left;
    font-weight: bold;
}

table th,
table td {
    padding: 12px 15px;
}

table tbody tr {
    border-bottom: 1px solid #dddddd;
}

table tbody tr:nth-of-type(even) {
    background-color: #f3f3f3;
}

table tbody tr:last-of-type {
    border-bottom: 2px solid #009879;
}

table tbody tr.active-row {
    font-weight: bold;
    color: #009879;
}

)";

std::tuple<std::string, std::string, std::string> removeYAMLHeaderAndExtractTitle(const std::string& markdown) {
    std::istringstream stream(markdown);
    std::ostringstream result;
    std::string line;
    bool inYAMLHeader = false;
    std::string title, date;

    while (std::getline(stream, line)) {
        if (line == "---") {
            inYAMLHeader = !inYAMLHeader;
            if (!inYAMLHeader) {
                result << "<!-- YAML Header removed -->\n";
            }
            continue;
        }
        if (inYAMLHeader) {
            std::regex titleRegex("^title:\\s*(.*)$");
            std::regex dateRegex("^date:\\s*(\\d{4}-\\d{2}-\\d{2})$");
            std::smatch match;
            if (std::regex_match(line, match, titleRegex)) {
                title = match[1].str();
            } else if (std::regex_match(line, match, dateRegex)) {
                date = match[1].str();
            }
        } else {
            result << line << "\n";
        }
    }

    return {result.str(), title, date};
}


// Function to split a string by a delimiter
std::vector<std::string> split(const std::string& str, char delimiter) {
    std::vector<std::string> tokens;
    std::string token;
    std::istringstream tokenStream(str);
    while (std::getline(tokenStream, token, delimiter)) {
        tokens.push_back(token);
    }
    return tokens;
}
/*
// Function to trim whitespace from a string
std::string trim(const std::string& str) {
    size_t first = str.find_first_not_of(' ');
    if (std::string::npos == first) {
        return str;
    }
    size_t last = str.find_last_not_of(' ');
    return str.substr(first, (last - first + 1));
}*/

// trim from end of string (right)
inline std::string& rtrim(std::string& s)
{
    s.erase(s.find_last_not_of(' ') + 1);
    return s;
}

// trim from beginning of string (left)
inline std::string& ltrim(std::string& s)
{
    s.erase(0, s.find_first_not_of(' '));
    return s;
}

// trim from both ends of string (right then left)
inline std::string& trim(std::string& s)
{
    return ltrim(rtrim(s));
}

std::string markdownToHTML(const std::string& markdown) {
    std::string html;
    std::istringstream stream(markdown);
    std::string line;
    std::regex codeBlockRegex("```(\\w+)?");
    std::regex headingRegex("^(#{1,6})\\s+(.*)$");
    std::regex boldRegex("\\*\\*(.*?)\\*\\*");
    std::regex italicRegex("\\*(.*?)\\*");
    std::regex underlineRegex("__(.*?)__");
    std::regex imageRegex("!\\[(.*?)\\]\\((.*?)\\)");
    std::regex bulletPointRegex("^(\\s*[-*])\\s+(.*)$");
    std::regex tableStartRegex(R"(^\s*\|.*\|\s*$)");
    std::smatch match;
    bool inCodeBlock = false;
    bool inList = false;
    
    // table states
    bool inTable = false;
    int tableRow = 0;
    std::vector<std::string> rowAlignemnts;

    std::string language;

    while (std::getline(stream, line)) {
        if (inCodeBlock) {

            if (std::regex_match(line, match, codeBlockRegex)) {
                html += "</code></pre>\n";
                inCodeBlock = false;
            } else {
                html += line + "\n";
            }
            

        } else if (std::regex_match(line, match, tableStartRegex)) {
  
            if (!inTable) {
                html += "<table>\n";
                inTable = true;
                tableRow = 0;
            }

            line = trim(line);

            std::vector<std::string> row = split(line, '|');
            for (auto& cell : row) {
                cell = trim(cell);
            }

            if (tableRow == 0) {
                html += "<thead>\n<tr>\n";
                for (const auto& cell : row) {
                    html += "<th>" + cell + "</th>";
                }
                html += "\n</tr>\n</thead>\n<tbody>\n";

            } else if (tableRow == 1) {
            
                // Parse markdown dashes and column alignment
                rowAlignemnts.clear();
                for (const auto& cell : row) {
                    std::string alignment = "left";
                    bool f = cell.front() && ':';
                    auto b = cell.back() && ':';
                    if (f && b) {
                        alignment = "center";
                    }
                    else if (b) {
                        alignment = "right";
                    } else {
                        alignment = "left";
                    }
                    rowAlignemnts.push_back(alignment);
                }


            } else {
                html += "<tr>\n";
                int i = 0;

                if (row.size() != rowAlignemnts.size()) {
                    std::cerr << "Error: Table alignment error, " << rowAlignemnts.size() << row.size() << line << std::endl;
                    // Fix the alignment
                    rowAlignemnts.clear();
                    for (const auto& cell : row) {
                        rowAlignemnts.push_back("left");
                    }
                }
                

                for (const auto& cell : row) {
                    html += "<td style=\"text-align:" + rowAlignemnts[i++] + "\">" + cell + "</td>";
                }
                html += "\n</tr>\n";
            }
            tableRow++;


        } else if (std::regex_match(line, match, codeBlockRegex)) {
            if (!inCodeBlock) {
                inCodeBlock = true;
                language = match[1].str();
                html += "<pre><code class=\"language-" + language + "\">\n";
            }
        } else if (std::regex_match(line, match, headingRegex)) {
            if (inList) {
                html += "</ul>\n";
                inList = false;
            }
            int level = match[1].str().size();
            html += "<h" + std::to_string(level) + ">" + match[2].str() + "</h" + std::to_string(level) + ">\n";
        } else if (std::regex_search(line, match, imageRegex)) {
            if (inList) {
                html += "</ul>\n";
                inList = false;
            }
            std::string altText = match[1].str();
            std::string imagePath = match[2].str();
            html += "<img src=\"" + imagePath + "\" alt=\"" + altText + "\">\n";
        } else if (std::regex_match(line, match, bulletPointRegex)) {
            if (!inList) {
                html += "<ul>\n";
                inList = true;
            }
            html += "<li>" + match[2].str() + "</li>\n";

        } else {
            if (inList) {
                html += "</ul>\n";
                inList = false;
            }

            if (inTable) {
                html += "</tbody>\n</table>\n";
                inTable = false;
            }

            // Apply formatting (bold, italic, underline)
            line = std::regex_replace(line, boldRegex, "<b>$1</b>");
            line = std::regex_replace(line, italicRegex, "<i>$1</i>");
            line = std::regex_replace(line, underlineRegex, "<u>$1</u>");

            // Wrap in paragraph
            html += "<p>" + line + "</p>\n";
        }
    }

    return html;
}

void generateIndexPage(const std::string& outputDir, const std::vector<std::tuple<std::string, std::string, std::string>>& articles) {
    size_t articlesPerPage = 10;
    size_t totalPages = (articles.size() + articlesPerPage - 1) / articlesPerPage;

    for (size_t page = 0; page < totalPages; ++page) {
        std::string pageFileName = (page == 0) ? "index.html" : ("index" + std::to_string(page + 1) + ".html");
        fs::path pagePath = fs::path(outputDir) / pageFileName;
        std::ofstream indexFile(pagePath);

        indexFile << "<!DOCTYPE html>\n<html lang=\"en\">\n<head>\n"
                  << "<meta charset=\"UTF-8\">\n<meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">\n"
                  << "<title>Blog Index - Page " << (page + 1) << "</title>\n"
                  << "<link rel=\"stylesheet\" href=\"style.css\">\n"
                  << "</head>\n<body>\n";
                  
        indexFile << customHeader << "<div style=\"padding: 10px;\">\n";

        indexFile << "<h2>Blog Index - Page " << (page + 1) << "</h2>\n<ul>\n";

        for (size_t i = page * articlesPerPage; i < std::min((page + 1) * articlesPerPage, articles.size()); ++i) {
            const auto& [articleTitle, articleLink, articleDate] = articles[i];
            indexFile << "<li>[" << articleDate << "] <a href=\"" << articleLink << "\">" << articleTitle << "</a></li>\n";
        }

        indexFile << "</ul>\n";

        if (page > 0) {
            indexFile << "<a href=\"index" << (page == 1 ? "" : std::to_string(page)) << ".html\">Previous</a>\n";
        }
        if (page + 1 < totalPages) {
            indexFile << "<a href=\"index" << (page + 2) << ".html\">Next</a>\n";
        }

        indexFile << "</div></body>\n</html>\n";
    }
}

void generateBlog(const std::string& inputDir, const std::string& outputDir) {
    std::vector<std::tuple<std::string, std::string, std::string>> articles;

    for (const auto& entry : fs::directory_iterator(inputDir)) {
        if (entry.is_directory()) {
            fs::path indexPath = entry.path() / "index.md";
            if (fs::exists(indexPath)) {
                std::ifstream inputFile(indexPath);
                std::ostringstream buffer;
                buffer << inputFile.rdbuf();

                std::string markdownContent = buffer.str();
                auto [cleanedMarkdown, title, date] = removeYAMLHeaderAndExtractTitle(markdownContent);
                std::string htmlContent = markdownToHTML(cleanedMarkdown);

                std::string articleName = entry.path().filename().string();
                fs::path articleOutputDir = fs::path(outputDir) / articleName;

                if (!fs::exists(articleOutputDir)) {
                    fs::create_directories(articleOutputDir);
                }

                fs::path outputPath = articleOutputDir / "index.html";

                std::ofstream outputFile(outputPath);
                outputFile << "<!DOCTYPE html>\n<html lang=\"en\">\n<head>\n"
                           << "<meta charset=\"UTF-8\">\n<meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">\n"
                           << "<title>" << (title.empty() ? articleName : title) << "</title>\n"
                           << "<link rel=\"stylesheet\" href=\"https://cdnjs.cloudflare.com/ajax/libs/highlight.js/11.7.0/styles/default.min.css\">\n"
                           << "<script src=\"https://cdnjs.cloudflare.com/ajax/libs/highlight.js/11.7.0/highlight.min.js\"></script>\n"
                           << "<script>hljs.highlightAll();</script>\n"
                           << "<link rel=\"stylesheet\" href=\"../style.css\">\n"
                           << "</head>\n<body>\n";

                outputFile << customHeader << "\n";
                // Ajout du lien vers l'index
                outputFile << "<a href=\"../index.html\">&larr; Back to Index</a>\n";

                outputFile << "<div class=\"container\">\n";

                if (!title.empty()) {
                    outputFile << "<h1>" << title << "</h1>\n";
                }

                outputFile << htmlContent;

                outputFile << "</div>\n";
                outputFile << "</body>\n</html>\n";

                articles.emplace_back((title.empty() ? articleName : title), articleName + "/index.html", date);

                // Copy all other files from the article directory to the output directory only if they don't exist
                for (const auto& file : fs::directory_iterator(entry.path())) {
                    if (file.path() != indexPath) {
                        fs::path destPath = articleOutputDir / file.path().filename();
                        if (!fs::exists(destPath)) {
                            fs::copy(file.path(), destPath, fs::copy_options::skip_existing);
                        }
                    }
                }
            }
        }
    }

    // Sort articles by date descending
    std::sort(articles.begin(), articles.end(), [](const auto& a, const auto& b) {
        return std::get<2>(a) > std::get<2>(b); // Compare dates
    });


    generateIndexPage(outputDir, articles);
}

void generateDefaultCss(const fs::path& outputDir) {
    fs::path cssFilePath = outputDir / "style.css";
    std::ofstream cssFile(cssFilePath);
    if (cssFile.is_open()) {
        cssFile << defaultCssFile;
        cssFile.close();
    } else {
        std::cerr << "Unable to create CSS file at " << cssFilePath << std::endl;
    }
}


void getHeaderIfExists(const fs::path& inputDir) {
    fs::path headerFilePath = inputDir / "header.html";
    std::ifstream headerFile(headerFilePath);
    if (headerFile.is_open()) {
        customHeader = std::string((std::istreambuf_iterator<char>(headerFile)), std::istreambuf_iterator<char>());
        headerFile.close();
    } else {
        customHeader = std::string(defaultHeader);
        std::cerr << "Header file not found, using default" <<  std::endl;
    }
}

int main(int argc, char* argv[]) {
    if (argc != 3) {
        std::cerr << "Usage: " << argv[0] << " <input_directory> <output_directory>" << std::endl;
        return 1;
    }

    std::string inputDir = argv[1];
    std::string outputDir = argv[2];

    if (!fs::exists(outputDir)) {
        fs::create_directories(outputDir);
    }

    getHeaderIfExists(inputDir);

    generateBlog(inputDir, outputDir);

    fs::path cssPath = fs::path(inputDir) / "style.css";
    if (fs::exists(cssPath)) {
        fs::copy(cssPath, outputDir, fs::copy_options::overwrite_existing);
    } else {
        // Dump default css
        generateDefaultCss(outputDir);
    }

    std::cout << "Blog generated successfully in " << outputDir << std::endl;
    return 0;
}
