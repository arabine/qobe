#include <iostream>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <regex>


namespace fs = std::filesystem;


// =========================================================================
// HEADER OPTIONAL CUSTOMISATION
// =========================================================================
static const char *defaultHeader = R"(
<header>
    <h1>D8S EURL Blog</h1>
</header>

)";

static std::string customHeader;


// =========================================================================
// FOOTER OPTIONAL CUSTOMISATION
// =========================================================================
static const char *defaultFooter = R"(
<footer>
    <p>&copy; 2025 D8S EURL - Tous droits réservés.</p>
</footer>


)";

static std::string customFooter;

// =========================================================================
// ASIDE OPTIONAL CUSTOMISATION
// =========================================================================
static const char *defaultAside = R"(
<aside class="sidebar">
    <h2>Projets</h2>
    <div class="projects-grid">
        <div class="project">
            <a href="https://example.com/projet1" target="_blank">
                <img src="https://placehold.co/600x400" alt="Projet 1">
                <div class="overlay">Projet 1</div>
            </a>
        </div>
        <div class="project">
            <a href="https://example.com/projet2" target="_blank">
                <img src="https://placehold.co/600x400" alt="Projet 2">
                <div class="overlay">Projet 2</div>
            </a>
        </div>
        <div class="project">
            <a href="https://example.com/projet3" target="_blank">
                <img src="https://placehold.co/600x400" alt="Projet 3">
                <div class="overlay">Projet 3</div>
            </a>
        </div>
    </div>
</aside>

)";

static std::string customAside;


// Default CSS file 
static const char *defaultCssFile = R"(
/* Général */
body {
    font-family: Arial, sans-serif;
    margin: 0;
    padding: 0;
}

/* Conteneurs principaux */
.main-container {
    display: flex;
    max-width: 1200px;
    margin: auto;
    padding: 20px;
}

.content-container {
    flex: 3;
}

.sidebar {
    flex: 1;
    margin-left: 20px;
}

/* Header & Footer */
header, footer {
    text-align: center;
    padding: 10px;
    background: #ddd;
}

/* Liens */
a {
    text-decoration: none;
}

/* Liste d'articles */
.blog-list {
    padding: 0;
    list-style: none;
}

.blog-list li {
    display: flex;
    align-items: center;
    padding: 5px 0;
    border-bottom: 1px solid #ccc;
}

/* Pagination */
.pagination {
    text-align: center;
    margin-top: 10px;
}

.pagination-button {
    padding: 5px 10px;
    border: 1px solid #000;
    background: #eee;
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


void generateIndexPage(const std::string& outputDir, const std::vector<std::tuple<std::string, std::string, std::string>>& articles)
{
        fs::path pagePath = fs::path(outputDir) / "index.html";
        std::ofstream indexFile(pagePath);

        indexFile << R"(<!DOCTYPE html>
        <html lang="en">
        <head>
            <meta charset="UTF-8" />
            <meta name="viewport" content="width=device-width, initial-scale=1.0" />
            <title>Blog Index</title>
            <link rel="stylesheet" href="style.css" />
        </head>
        <body>)";

        indexFile << customHeader;

        indexFile << R"(

        <main class="main-container">
            <section class="content-container">

                <h2>Blog Index</h2>
                <ul id="blog-list" class="blog-list">
        )";


        for (size_t i = 0; i < articles.size(); i++) {
            const auto& [articleTitle, articleLink, articleDate] = articles[i];
            indexFile << R"(<li>
                <span class="blog-date">[
            )" << articleDate << "]</span> <a href=\"" << articleLink << "\"  class=\"blog-link\">" << articleTitle << "</a></li>\n";
        }

        indexFile << R"(
            <!-- Pagination -->
                    <div class="pagination">
                        <button id="prev-button" class="pagination-button" disabled>Précédent</button>
                        <span id="page-info"></span>
                        <button id="next-button" class="pagination-button">Suivant</button>
                    </div>
                </section>


            )";

        indexFile << customAside;

        indexFile << R"(
            </main>
        )";

        indexFile << customFooter;

        indexFile << R"(
            </body>
            </html>
        )";

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

void getAsideIfExists(const fs::path& inputDir) {
    fs::path asideFilePath = inputDir / "aside.html";
    std::ifstream asideFile(asideFilePath);
    if (asideFile.is_open()) {
        customAside = std::string((std::istreambuf_iterator<char>(asideFile)), std::istreambuf_iterator<char>());
        asideFile.close();
    } else {
        customAside = std::string(defaultAside);
        std::cerr << "Aside file not found, using default" <<  std::endl;
    }
}


void getFooterIfExists(const fs::path& inputDir) {
    fs::path footerFilePath = inputDir / "footer.html";
    std::ifstream footerFile(footerFilePath);
    if (footerFile.is_open()) {
        customFooter = std::string((std::istreambuf_iterator<char>(footerFile)), std::istreambuf_iterator<char>());
        footerFile.close();
    } else {
        customFooter = std::string(defaultFooter);
        std::cerr << "Footer file not found, using default" <<  std::endl;
    }
}


void copy_directory(const fs::path& source, const fs::path& destination) {
    if (!fs::exists(source) || !fs::is_directory(source)) {
        std::cerr << "Erreur : Le répertoire source n'existe pas ou n'est pas un dossier.\n";
        return;
    }

    // Parcourir récursivement le répertoire source
    for (const auto& entry : fs::recursive_directory_iterator(source)) {
        fs::path dest_path = destination / fs::relative(entry.path(), source);

        try {
            if (fs::is_directory(entry.status())) {
                // Créer les sous-dossiers dans la destination
                fs::create_directories(dest_path);
            } else if (fs::is_regular_file(entry.status())) {
                // Copier le fichier
                fs::copy_file(entry.path(), dest_path, fs::copy_options::overwrite_existing);
            }
        } catch (const fs::filesystem_error& e) {
            std::cerr << "Erreur lors de la copie de " << entry.path() << ": " << e.what() << '\n';
        }
    }
}

int main(int argc, char* argv[]) {
    if (argc < 3) {
        std::cerr << "Usage: " << argv[0] << " <input_directory> <output_directory> [optional]<custom_directory>" << std::endl;
        return 1;
    }

    std::string inputDir = argv[1];
    std::string outputDir = argv[2];
    std::string optionalDir;

    if (argc == 4) {
        optionalDir = argv[3];
    }

    if (!fs::exists(outputDir)) {
        fs::create_directories(outputDir);
    }

    getHeaderIfExists(optionalDir);

    getAsideIfExists(optionalDir);

    getFooterIfExists(optionalDir);

    // We generate a default CSS file, overrided by a custom CSS if exists
    generateDefaultCss(outputDir);

    generateBlog(inputDir, outputDir);


    if (optionalDir.size() > 0) {
        copy_directory(optionalDir, outputDir);
    }

    std::cout << "Blog generated successfully in " << outputDir << std::endl;
    return 0;
}
