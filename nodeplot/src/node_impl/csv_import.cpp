#include "nodeplot.h"

#include <fcntl.h>
#include <string_view>
#include <sys/mman.h>
#include <unistd.h>

#include <filesystem>

using namespace NodePlot;

void register_csv_import() {
    NodeRegistry::register_node("csv_import",
                                Node{
                                    .type_id = "csv_import",
                                    .display_name = "CSV Import",
                                    .inputs = [](NodePlotFile*, EvaluatedNodeGraph*, NodeId) -> std::vector<std::pair<InputId, Node::Input>> {
                                        return {
                                            {"filename", Node::Input{.id = "filename", .display_name = "Filename", .valid_data_types = {DataType::STRING}, .attributes = {InputAttribute::FilePath{}}}},
                                            {"has_headers", Node::Input{.id = "has_headers", .display_name = "Has Headers", .valid_data_types = {DataType::BOOLEAN}, .default_value = {true}}},
                                        };
                                    },
                                    .outputs = [](NodePlotFile*, EvaluatedNodeGraph*, NodeId) -> std::vector<std::pair<OutputId, Node::Output>> {
                                        return {
                                            {"table", Node::Output{.id = "table", .display_name = "Table", .valid_data_types = {DataType::TABLE}}},
                                        };
                                    },
                                    .evalulate = [](NodePlotFile* npf, EvaluatedNodeGraph* eng, NodeId node_id, NodeOutputCache& cache) -> ErrorOr<void> {
                                        std::filesystem::path filename = TRY(eng->get_input_value<std::filesystem::path>(npf, node_id, "filename"));
                                        bool has_headers = TRY(eng->get_input_value<bool>(npf, node_id, "has_headers"));

                                        filename = npf->path.parent_path() / filename;

                                        std::print("Loading: {}\n", filename.string());

                                        if (!std::filesystem::exists(filename)) {
                                            return ERR("Input CSV file does not exist");
                                        }

                                        if (std::filesystem::is_directory(filename)) {
                                            return ERR("Input CSV file is a directory");
                                        }

                                        auto file_size = std::filesystem::file_size(filename);

                                        int fd = open(filename.c_str(), O_RDWR);
                                        if (fd < 0)
                                            return ERR("Failed to open CSV input file");

                                        char* map = static_cast<char*>(mmap(NULL, file_size, PROT_READ, MAP_SHARED, fd, 0));
                                        if (map == MAP_FAILED) {
                                            close(fd);
                                            return ERR("Failed to map CSV input file");
                                        }

                                        auto mapped_file = std::shared_ptr<MappedFile>(new MappedFile{.data = map, .file_size = file_size});

                                        Table res;
                                        auto full_string_view = std::string_view(mapped_file->data, mapped_file->file_size);

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

                                        auto cur = full_string_view;
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
                                                                Column{Column::CSVImported{
                                                                    .mapped_file = mapped_file,
                                                                    .values = std::move(extracted_values[i]),
                                                                }});

                                        printf("Loaded CSV file with %zu columns and %zu rows\n", res.columns.size(), rows_loaded);

                                        cache.computed_outputs["table"] = res;

                                        return {};
                                    },
                                });
}