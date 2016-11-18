
#include "components/graph_panel.h"

#include <math.h>

#include "ui.h"
#include "libs/types.h"
#include "libs/imgui/imgui.h"
#include "libs/mathlib.h"
#include "libpapaya.h"

const Vec2 node_sz = Vec2(36, 36);

void init_graph_panel(GraphPanel* g) {
    g->scroll_pos = Vec2(0,0);
    g->node_properties_panel_height = 200.0f;
    g->width = 300.0f;
    g->cur_node = 0;
    g->dragged_slot = 0;
    g->displaced_slot = 0;
}

static Vec2 get_slot_pos(PapayaNode* node, PapayaSlot* slot)
{
    return Vec2(node->pos_x, node->pos_y)
         + Vec2(node_sz.x * slot->pos_x,
                node_sz.y * slot->pos_y);
}

static Vec2 get_output_slot_pos(PapayaNode* node)
{
    return Vec2(node->pos_x, node->pos_y) + Vec2(node_sz.x * 0.5f, 0);
}

static Vec2 get_input_slot_pos(PapayaNode* node)
{
    return get_output_slot_pos(node) + Vec2(0, node_sz.y);
}

static Vec2 find_link_snap(PapayaMemory* mem, Vec2 offset, int* node_idx)
{
    GraphPanel* g = &mem->graph_panel;
    Vec2 m = mem->mouse.pos;

    for (int i = 0; i < mem->doc->num_nodes; i++) {
        PapayaNode* n = &mem->doc->nodes[i];
        Vec2 p = Vec2(n->pos_x, n->pos_y) + offset;
        if (m.x >= p.x &&
            m.y >= p.y &&
            m.x <= p.x + node_sz.x &&
            m.y <= p.y + node_sz.y) {
            *node_idx = i;
            return offset + get_output_slot_pos(n)
                          + (g->dragged_slot->is_out ? Vec2(0, node_sz.y) : Vec2());
        }
    }

    *node_idx = -1;
    return m;
}

static void draw_link(Vec2 b, Vec2 t, ImDrawList* draw_list)
{
    draw_list->AddBezierCurve(b,
                              b + Vec2(0,-10),
                              t + Vec2(0,+10),
                              t,
                              ImColor(220, 163,89, 150),
                              4.0f); // Link thickness
}

static void draw_nodes(PapayaMemory* mem)
{
    GraphPanel* g = &mem->graph_panel;
    Vec2 offset = ImGui::GetCursorScreenPos() + g->scroll_pos;
    ImDrawList* draw_list = ImGui::GetWindowDrawList();
    int hovered_node = -1;

    for (int i = 0; i < mem->doc->num_nodes; i++) {
        PapayaNode* n = &mem->doc->nodes[i];
        Vec2 pos = offset + Vec2(n->pos_x - 1, n->pos_y - 1);

        ImGui::PushID(i);

        // Slots
        for (int j = 0; j < n->num_slots; j++) {
            PapayaSlot* s = &n->slots[j];
            Vec2 pos = offset + Vec2(n->pos_x, n->pos_y)
                              + Vec2(node_sz.x * s->pos_x,
                                     node_sz.y * s->pos_y);
            float rad = 4.0f;
            Vec2 dia = Vec2(2.0f * rad, 2.0f * rad);
            ImGui::PushID(j);
            ImGui::SetCursorScreenPos(pos - (dia * 0.5f));
            ImGui::InvisibleButton("slot", dia);
            draw_list->AddCircleFilled(pos, rad, (ImGui::IsItemHovered() ?
                                                  ImColor(255,150,150,255) :
                                                  ImColor(150,150,150,150)));

            if (ImGui::IsItemActive() && ImGui::IsMouseDragging(0)) {
                if (!s->to[0] || s->is_out) {
                    g->dragged_slot = s;
                } else {
                    g->displaced_slot = s;
                    g->dragged_slot = s->to[0];
                }
            }
            ImGui::PopID();
        }

        draw_list->ChannelsSetCurrent(0);
        ImGui::SetCursorScreenPos(pos);
        ImGui::InvisibleButton("node", node_sz);
        if (ImGui::IsItemHovered()) {
            hovered_node = i;
        }

        if (ImGui::IsItemActive()) {
            if (g->cur_node != i) {
                g->cur_node = i;
                core::update_canvas(mem);
            }
            if (ImGui::IsMouseDragging(0)) {
                n->pos_x += ImGui::GetIO().MouseDelta.x;
                n->pos_y += ImGui::GetIO().MouseDelta.y;
            }
        }

#if 0
        // Node debug info
        {
            ImGui::SetCursorScreenPos(pos + Vec2(node_sz.x + 5,0));
            int in = -1;
            int out = -1;
            if (n->in) {
                in = n->in - mem->doc->nodes;
            }
            if (n->out) {
                out = n->out - mem->doc->nodes;
            }
            // ImGui::Text("in: %d\nout: %d", in, out);
            ImGui::Text("%s", n->name);
        }
#endif

        // Thumbnail
        draw_list->ChannelsSetCurrent(1);
        ImGui::SetCursorScreenPos(pos);
        ImGui::BeginGroup(); // Lock horizontal position
        {
            ImColor c = (hovered_node == i ||
                         mem->graph_panel.cur_node == i) ?
                ImColor(220,163,89, 150) : ImColor(60,60,60);
            // TODO: Optimization: Use mipmaps here.
            // TODO: Unstretch aspect ratio
            ImGui::Image(0, node_sz, Vec2(0,0), Vec2(1,1),
                         ImVec4(1,1,1,1), c);
        }
        ImGui::EndGroup();

        // Incoming links
        draw_list->ChannelsSetCurrent(0);

        for (int j = 0; j < n->num_slots; j++) {

            PapayaSlot* t = &n->slots[j];
            if (t->is_out) {
                continue;
            }

            PapayaSlot* b = t->to[0];
            if (!b) {
                continue;
            }

            if (g->dragged_slot &&
                !g->dragged_slot->is_out &&
                g->dragged_slot->node == n) {
                // This link is being dragged. Skip the normal drawing and
                // handle this in the link interaction code
                continue;
            }

            if (g->displaced_slot &&
                g->displaced_slot->node == n) {
                continue;
            } 

            draw_link(offset + get_slot_pos(b->node, b),
                      offset + get_slot_pos(n, t),
                      draw_list);
        }

        ImGui::PopID();
    }

    // Link interaction
    if (g->dragged_slot) {

        PapayaNode* d = g->dragged_slot->node;

        int s = -1; // Index of node snapped to. -1 if not snapped to any node.
        if (g->dragged_slot->is_out) {
            Vec2 b = offset + get_output_slot_pos(d);
            Vec2 t = find_link_snap(mem, offset, &s);
            draw_link(b, t, draw_list);
        } else {
            Vec2 b = find_link_snap(mem, offset, &s);
            Vec2 t = offset + get_input_slot_pos(d);
            draw_link(b, t, draw_list);
        }

        if (ImGui::IsMouseReleased(0)) {

            // TODO: Handle this inside libpapaya
            if (s != -1) {
                PapayaSlot* out = &d->slots[1];
                PapayaSlot* in  = &mem->doc->nodes[s].slots[0];

                if (g->displaced_slot) {
                    papaya_disconnect(out, g->displaced_slot);
                } else if (!g->dragged_slot->is_out) {
                    in = &d->slots[0];
                    out  = &mem->doc->nodes[s].slots[1];
                }
                papaya_connect(out, in);
                core::update_canvas(mem);
            } else if (g->displaced_slot) {
                // Disconnection
                papaya_disconnect(g->displaced_slot->to[0], g->displaced_slot);
            }
            g->dragged_slot = 0;
            g->displaced_slot = 0;
        }

    }
}

void draw_graph_panel(PapayaMemory* mem)
{
    GraphPanel* g = &mem->graph_panel;

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, Vec2(5, 5));
    ImGui::PushStyleVar(ImGuiStyleVar_ItemInnerSpacing, Vec2(5, 2));
    ImGui::PushStyleColor(ImGuiCol_WindowBg, mem->colors[PapayaCol_Clear]);

    float x = mem->window.width - 36 - g->width; // Panel X
    float y = 55; // Panel Y
    float w = g->width; // Panel width
    float h = g->node_properties_panel_height; // Panel height

    // -------------------------------------------------------------------------
#if 1
    ImGui::SetNextWindowPos(Vec2(x, y));
    ImGui::SetNextWindowSize(Vec2(w, h));

    ImGui::Begin("Node properties", 0, mem->window.default_imgui_flags);

    ImGui::Checkbox(mem->doc->nodes[g->cur_node].name,
                    (bool*)&mem->doc->nodes[g->cur_node].is_active);
    ImGui::Text("Node properties controls go here");

    ImGui::End();
#endif
    // -------------------------------------------------------------------------

    y += h;
    h = mem->window.height - 58.0f - h;
    ImGui::SetNextWindowPos(Vec2(x, y));
    ImGui::SetNextWindowSize(Vec2(w, h));

    ImGui::Begin("Nodes", 0, mem->window.default_imgui_flags);

    ImGui::BeginGroup();

    const Vec2 NODE_WINDOW_PADDING(2.0f, 2.0f);

    // Create our child canvas
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, Vec2(1,1));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, Vec2(0,0));
    ImGui::PushStyleColor(ImGuiCol_ChildWindowBg, ImColor(60,60,70,200));
    ImGui::BeginChild("scrolling_region", Vec2(0,0), true, ImGuiWindowFlags_NoScrollbar|ImGuiWindowFlags_NoMove);
    ImGui::PushItemWidth(120.0f);

    ImDrawList* draw_list = ImGui::GetWindowDrawList();
    draw_list->ChannelsSplit(2);

    // Draw the grid
    {
        ImU32 col = ImColor(200,200,200,40);
        float grid_sz = 50.0f;
        Vec2 win_pos = ImGui::GetCursorScreenPos();
        Vec2 win_sz = ImGui::GetWindowSize();

        // Vertical lines
        float x = fmodf(g->scroll_pos.x, grid_sz);
        while (x < win_sz.x) {
            draw_list->AddLine(win_pos + Vec2(x, 0),
                               win_pos + Vec2(x, win_sz.y), col);
            x += grid_sz;
        }

        // Horizontal lines
        float y = fmodf(g->scroll_pos.y, grid_sz);
        while (y < win_sz.y) {
            draw_list->AddLine(win_pos + Vec2(0, y),
                               win_pos + Vec2(win_sz.x, y), col);
            y += grid_sz;
        };
    }

    // Display nodes
    draw_nodes(mem);
    draw_list->ChannelsMerge();

    // Scrolling
    if (ImGui::IsWindowHovered() && !ImGui::IsAnyItemActive() && ImGui::IsMouseDragging(2, 0.0f))
        mem->graph_panel.scroll_pos += ImGui::GetIO().MouseDelta;

    ImGui::PopItemWidth();
    ImGui::EndChild();
    ImGui::PopStyleColor();
    ImGui::PopStyleVar(2);
    ImGui::EndGroup();

    ImGui::End();
    ImGui::PopStyleVar(2);
    ImGui::PopStyleColor();
}