#include "node_registry.h"

extern void register_output();

extern void register_binary_operation();
extern void register_column_select();
extern void register_create_plot();
extern void register_csv_import();
extern void register_filter_table();
extern void register_function();
extern void register_sampled_property_extract();
extern void register_series_create();
extern void register_value();

namespace NodePlot {

std::map<NodeTypeId, Node> NodeRegistry::node_map = {};

void NodeRegistry::init() {
    register_binary_operation();
    register_column_select();
    register_create_plot();
    register_csv_import();
    register_filter_table();
    register_function();
    register_sampled_property_extract();
    register_series_create();
    register_value();
    register_output();
}

void NodeRegistry::NodeRegistry::register_node(NodeTypeId node_id, Node node) { NodeRegistry::node_map.insert({node_id, node}); }

} // namespace NodePlot