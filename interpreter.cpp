#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <map>

// Define a Node structure for representing parsed content
struct Node {
    std::string type;               // Element type (e.g., "page", "header")
    std::string value;              // Text content (e.g., "Hello World!")
    std::vector<Node> children;     // Nested elements
};

// Function to read a file into a string
std::string read_file(const std::string& filepath) {
    std::ifstream file(filepath);
    if (!file) {
        throw std::runtime_error("Could not open file: " + filepath);
    }
    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

// Function to escape HTML special characters
std::string escape_html(const std::string& input) {
    std::string output;
    for (char c : input) {
        switch (c) {
            case '&': output += "&amp;"; break;
            case '"': output += "&quot;"; break;
            case '<': output += "&lt;"; break;
            case '>': output += "&gt;"; break;
            default: output += c; break;
        }
    }
    return output;
}

// Function to tokenize .wbb input
std::vector<std::string> tokenize(const std::string& source) {
    std::vector<std::string> tokens;
    std::string token;
    bool inside_string = false;

    for (size_t i = 0; i < source.size(); ++i) {
        char ch = source[i];

        if (ch == '"' && !inside_string) {
            inside_string = true;
            token += ch; // Include the starting quote
        } else if (ch == '"' && inside_string) {
            inside_string = false;
            token += ch; // Include the closing quote
            tokens.push_back(token);
            token.clear();
        } else if (inside_string) {
            token += ch; // Add characters inside the string to the token
        } else if (ch == '<' || ch == '>' || ch == '{' || ch == '}') {
            // Handle special cases for custom syntax like <{>, </{>, etc.
            if (!token.empty()) {
                tokens.push_back(token);
                token.clear();
            }
            if (ch == '<' || ch == '>' || ch == '{' || ch == '}') {
                tokens.push_back(std::string(1, ch)); // Treat single characters as individual tokens
            }
        } else if (std::isspace(ch)) {
            if (!token.empty()) {
                tokens.push_back(token);
                token.clear();
            }
        } else {
            token += ch;
        }
    }
    
    if (!token.empty()) {
        tokens.push_back(token);
    }

    return tokens;
}

// Function to parse tokens into a tree structure
Node parse(std::vector<std::string>& tokens, size_t& index) {
    Node node;

    if (tokens[index] == "page") {
        node.type = "page";
        node.value = tokens[++index];  // Title of the page
        ++index; // Skip "{"
        while (tokens[index] != "}") {
            node.children.push_back(parse(tokens, index));
        }
        ++index; // Skip "}"
    } else if (tokens[index] == "header" || tokens[index] == "footer") {
        node.type = tokens[index++];
        ++index; // Skip "{"
        while (tokens[index] != "}") {
            node.children.push_back(parse(tokens, index));
        }
        ++index; // Skip "}"
    } else {
        node.type = tokens[index++];  // Tag name (e.g., h1, p)

        if (tokens[index][0] == '"') {
            node.value = tokens[index++];  // If the next token is a string, assign it as the value
        }

        if (tokens[index] == "{") {
            ++index; // Skip "{"
            while (tokens[index] != "}") {
                node.children.push_back(parse(tokens, index));
            }
            ++index; // Skip "}"
        }
    }

    return node;
}

// Function to generate formatted HTML
std::string generate_html(const Node& node, int indent_level = 0) {
    std::string html;
    std::string indent(indent_level, ' '); // Create indentation string

    // Check for "page" node
    if (node.type == "page") {
        html += indent + "<html>\n";
        html += indent + "  <head><title>" + escape_html(node.value) + "</title></head>\n";
        html += indent + "  <body>\n";
        for (const auto& child : node.children) {
            html += generate_html(child, indent_level + 4); // Increase indentation for nested elements
        }
        html += indent + "  </body>\n";
        html += indent + "</html>\n";
    } else {
        // Standard tag (like h1, p, etc.)
        html += indent + "<" + node.type + ">";
        if (!node.value.empty()) {
            html += escape_html(node.value);
        }
        for (const auto& child : node.children) {
            html += generate_html(child, indent_level + 2); // Increase indentation for nested elements
        }
        html += "</" + node.type + ">\n";
    }
    
    return html;
}

int main(int argc, char* argv[]) {
    if (argc != 2) {
        std::cerr << "Usage: " << argv[0] << " <input.wbb>" << std::endl;
        return 1;
    }

    std::string filepath = argv[1];
    try {
        // Read the .wbb file
        std::string source = read_file(filepath);

        // Tokenize the input
        auto tokens = tokenize(source);

        // Parse the tokens into a tree structure
        size_t index = 0;
        Node root = parse(tokens, index);

        // Generate formatted HTML
        std::string html = generate_html(root);

        // Write the HTML to a file
        std::string output_path = filepath.substr(0, filepath.find_last_of('.')) + ".html";
        std::ofstream output_file(output_path);
        if (!output_file) {
            throw std::runtime_error("Could not create output file: " + output_path);
        }
        output_file << html;
        std::cout << "Generated HTML file: " << output_path << std::endl;

    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
