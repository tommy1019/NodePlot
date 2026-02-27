#include "nodeplot.h"

#include <fcntl.h>
#include <string_view>
#include <sys/mman.h>
#include <unistd.h>

#include <filesystem>

template <>
ErrorOr<NodeOutput> node_output(EvaluatedNodeGraph* graph, const CSVImportNode& node, OutputId id) {
    auto source_path = TRY_OR(graph->get_input(node.i_source_path), return ERR("Could not get 'Source Path' input"));
    auto has_headers = TRY_OR(graph->get_input(node.i_has_headers), return ERR("Could not get 'Has Headers' input"));

    source_path = graph->node_graph.file_path().parent_path() / source_path;

    std::print("Loading: {}\n", source_path.string());

    if (!std::filesystem::exists(source_path)) {
        std::print("Trying to load: {}\n", source_path.c_str());
        return ERR("Input CSV file does not exist");
    }

    if (std::filesystem::is_directory(source_path)) {
        std::print("Trying to load: {}\n", source_path.c_str());
        return ERR("Input CSV file is a directory");
    }

    auto file_size = std::filesystem::file_size(source_path);

    int fd = open(source_path.c_str(), O_RDWR);
    if (fd < 0)
        return ERR("Failed to open CSV input file");

    char* map = static_cast<char*>(mmap(NULL, file_size, PROT_READ, MAP_SHARED, fd, 0));
    if (map == MAP_FAILED) {
        close(fd);
        return ERR("Failed to map CSV input file");
    }

    Table res;
    res.mapped_file = std::shared_ptr<MappedFile>(new MappedFile{.data = map, .file_size = file_size});
    res.mapped_file->full_string_view = std::string_view(res.mapped_file->data, res.mapped_file->file_size);

    auto extract_csv_elements = [](std::string_view line) {
        std::vector<std::string_view> res;

        while (true) {
            while (line.size() > 0 && line[0] == ' ')
                line = line.substr(1);

            auto comma_pos = line.find_first_of(',');
            if (comma_pos == std::string::npos) {
                res.push_back(line);
                return res;
            }

            // TODO: Escaping commas
            res.push_back(line.substr(0, comma_pos));
            line = line.substr(comma_pos + 1);
        }
    };
    auto extract_line = [](std::string_view& text) {
        auto newline_pos = text.find('\n');

        if (newline_pos == std::string::npos) {
            auto res = text;
            text = {};
            return res;
        }

        auto str = text.substr(0, newline_pos);
        if (str.ends_with('\r'))
            str = str.substr(0, str.length() - 1);

        if (text.size() == newline_pos)
            text = {};
        else
            text = text.substr(newline_pos + 1);

        return str;
    };

    auto cur = res.mapped_file->full_string_view;
    if (has_headers) {
        auto extracted_names = extract_csv_elements(extract_line(cur));
        for (auto& s : extracted_names)
            res.column_names.emplace_back(s);
    }

    std::vector<std::vector<std::string_view>> extracted_values;

    size_t rows_loaded = 0;

    while (cur.size() > 0) {
        auto l = extract_csv_elements(extract_line(cur));

        if (l.size() > extracted_values.size()) {
            extracted_values.resize(l.size(), std::vector<std::string_view>(rows_loaded, std::string_view{}));
        }

        for (size_t i = 0; i < l.size(); i++) {
            extracted_values[i].push_back(l[i]);
        }
        for (size_t i = l.size(); i < extracted_values.size(); i++) {
            extracted_values[i].emplace_back();
        }

        rows_loaded++;
    }

    for (size_t i = res.column_names.size(); i < extracted_values.size(); i++)
        res.column_names.push_back(std::to_string(i));

    for (size_t i = 0; i < res.column_names.size(); i++)
        res.columns.emplace(res.column_names[i],
                            Column{
                                .name = res.column_names[i],
                                .values = std::move(extracted_values[i]),
                                .mapped_file = res.mapped_file,
                            });

    printf("Loaded CSV file with %zu columns and %zu rows\n", res.columns.size(), rows_loaded);

    return res;
}