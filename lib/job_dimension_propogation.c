#include "job.h"
#include "graph.h"

static bool flow_job_populate_outbound_dimensions_for_edge(Context *c, struct flow_job * job, struct flow_graph *g, int32_t outbound_edge_id){


    struct flow_edge * edge = &g->edges[outbound_edge_id];

    if (!flow_node_validate_inputs(c,g, edge->from)){
        CONTEXT_error_return(c);
    }
    if (!flow_node_populate_dimensions_to_edge(c,g,edge->from, outbound_edge_id)){
        CONTEXT_error_return(c);
    }
    return true;
}

bool flow_edge_has_dimensions(Context *c,struct flow_graph *g, int32_t edge_id){
    struct flow_edge * edge = &g->edges[edge_id];
    return edge->from_width > 0;
}
bool flow_node_input_edges_have_dimensions(Context *c,struct flow_graph *g, int32_t node_id){
    int32_t i;
    for (i = 0; i < g->next_edge_id; i++) {
        if (g->edges[i].type != flow_edgetype_null && g->edges[i].to == node_id) {
            if (!flow_edge_has_dimensions(c, g, i)) {
                return false;
            }
        }
    }
    return true;
}

static bool edge_visitor_populate_outbound_dimensions(Context *c, struct flow_job *job, struct flow_graph **graph_ref,
                                                      int32_t edge_id, bool *quit, bool *skip_outbound_paths,
                                                      void *custom_data){
    //Only populate if empty
    if (!flow_edge_has_dimensions(c,*graph_ref,edge_id)) {
        if (!flow_job_populate_outbound_dimensions_for_edge(c, job, *graph_ref, edge_id)){
            CONTEXT_error_return(c);
        }
        if (!flow_edge_has_dimensions(c,*graph_ref,edge_id)) {
            //We couldn't populate this edge, so we sure can't populate others in this direction.
            // Stop this branch of recursion
            *skip_outbound_paths = true;
        }else{
            flow_job_notify_graph_changed(c,job, *graph_ref);
        }
    }

    return true;
}

bool flow_job_populate_dimensions_where_certain(Context *c, struct flow_job * job, struct flow_graph **graph_ref ){
    //TODO: would be good to verify graph is acyclic.
    if (!flow_graph_walk(c, job, graph_ref, NULL, edge_visitor_populate_outbound_dimensions, NULL)){
        CONTEXT_error_return(c);
    }
    return true;
}