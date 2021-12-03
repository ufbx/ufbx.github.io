#include "ufbx.h"
#include <emscripten/emscripten.h>
#include <stdio.h>
#include <string>
#include "json_output.h"
#include "external/sokol_app.h"
#include "external/sokol_gfx.h"
#include "external/sokol_gl.h"
#include "external/sokol_glue.h"
#include "external/umath.h"
#include "shader/copy.h"

const char *get_element_type_name(ufbx_element_type type)
{
    switch (type) {
	case UFBX_ELEMENT_UNKNOWN: return "unknown";
	case UFBX_ELEMENT_NODE: return "node";
	case UFBX_ELEMENT_MESH: return "mesh";
	case UFBX_ELEMENT_LIGHT: return "light";
	case UFBX_ELEMENT_CAMERA: return "camera";
	case UFBX_ELEMENT_BONE: return "bone";
	case UFBX_ELEMENT_EMPTY: return "empty";
	case UFBX_ELEMENT_LINE_CURVE: return "line_curve";
	case UFBX_ELEMENT_NURBS_CURVE: return "nurbs_curve";
	case UFBX_ELEMENT_PATCH_SURFACE: return "patch_surface";
	case UFBX_ELEMENT_NURBS_SURFACE: return "nurbs_surface";
	case UFBX_ELEMENT_NURBS_TRIM_SURFACE: return "nurbs_trim_surface";
	case UFBX_ELEMENT_NURBS_TRIM_BOUNDARY: return "nurbs_trim_boundary";
	case UFBX_ELEMENT_PROCEDURAL_GEOMETRY: return "procedural_geometry";
	case UFBX_ELEMENT_CAMERA_STEREO: return "camera_stereo";
	case UFBX_ELEMENT_CAMERA_SWITCHER: return "camera_switcher";
	case UFBX_ELEMENT_LOD_GROUP: return "lod_group";
	case UFBX_ELEMENT_SKIN_DEFORMER: return "skin_deformer";
	case UFBX_ELEMENT_SKIN_CLUSTER: return "skin_cluster";
	case UFBX_ELEMENT_BLEND_DEFORMER: return "blend_deformer";
	case UFBX_ELEMENT_BLEND_CHANNEL: return "blend_channel";
	case UFBX_ELEMENT_BLEND_SHAPE: return "blend_shape";
	case UFBX_ELEMENT_CACHE_DEFORMER: return "cache_deformer";
	case UFBX_ELEMENT_CACHE_FILE: return "cache_file";
	case UFBX_ELEMENT_MATERIAL: return "material";
	case UFBX_ELEMENT_TEXTURE: return "texture";
	case UFBX_ELEMENT_VIDEO: return "video";
	case UFBX_ELEMENT_SHADER: return "shader";
	case UFBX_ELEMENT_SHADER_BINDING: return "shader_binding";
	case UFBX_ELEMENT_ANIM_STACK: return "anim_stack";
	case UFBX_ELEMENT_ANIM_LAYER: return "anim_layer";
	case UFBX_ELEMENT_ANIM_VALUE: return "anim_value";
	case UFBX_ELEMENT_ANIM_CURVE: return "anim_curve";
	case UFBX_ELEMENT_DISPLAY_LAYER: return "display_layer";
	case UFBX_ELEMENT_SELECTION_SET: return "selection_set";
	case UFBX_ELEMENT_SELECTION_NODE: return "selection_node";
	case UFBX_ELEMENT_CHARACTER: return "character";
	case UFBX_ELEMENT_CONSTRAINT: return "constraint";
	case UFBX_ELEMENT_POSE: return "pose";
	case UFBX_ELEMENT_METADATA_OBJECT: return "metadata_object";
	case UFBX_NUM_ELEMENT_TYPES: return "";
    }
}

void jso_string(jso_stream *s, ufbx_string str)
{
    jso_string_len(s, str.data, str.length);
}

void jso_prop_string(jso_stream *s, const char *name, ufbx_string str)
{
    jso_prop_string_len(s, name, str.data, str.length);
}

void json_application(jso_stream *s, const ufbx_application *app)
{
    jso_object(s);
    jso_prop_string(s, "name", app->name);
    jso_prop_string(s, "vendor", app->vendor);
    jso_prop_string(s, "version", app->version);
    jso_end_object(s);
}

void json_ufbx_prop(jso_stream *s, const ufbx_prop *prop)
{
    jso_object(s);
    jso_prop_string(s, "name", prop->name);
    jso_end_object(s);
}

void json_element_unknown(jso_stream *s, const ufbx_unknown* elem)
{
}

void json_element_node(jso_stream *s, const ufbx_node* elem)
{
    jso_prop_array(s, "children");

    for (ufbx_element *e : elem->all_attribs) {
        jso_int(s, (int)e->element_id);
    }

    for (ufbx_node *node : elem->children) {
        jso_int(s, (int)node->element_id);
    }

    jso_end_array(s);
}

void json_element_mesh(jso_stream *s, const ufbx_mesh* elem)
{
}

void json_element_light(jso_stream *s, const ufbx_light* elem)
{
}

void json_element_camera(jso_stream *s, const ufbx_camera* elem)
{
}

void json_element_bone(jso_stream *s, const ufbx_bone* elem)
{
}

void json_element_empty(jso_stream *s, const ufbx_empty* elem)
{
}

void json_element_line_curve(jso_stream *s, const ufbx_line_curve* elem)
{
}

void json_element_nurbs_curve(jso_stream *s, const ufbx_nurbs_curve* elem)
{
}

void json_element_patch_surface(jso_stream *s, const ufbx_patch_surface* elem)
{
}

void json_element_nurbs_surface(jso_stream *s, const ufbx_nurbs_surface* elem)
{
}

void json_element_nurbs_trim_surface(jso_stream *s, const ufbx_nurbs_trim_surface* elem)
{
}

void json_element_nurbs_trim_boundary(jso_stream *s, const ufbx_nurbs_trim_boundary* elem)
{
}

void json_element_procedural_geometry(jso_stream *s, const ufbx_procedural_geometry* elem)
{
}

void json_element_camera_stereo(jso_stream *s, const ufbx_camera_stereo* elem)
{
}

void json_element_camera_switcher(jso_stream *s, const ufbx_camera_switcher* elem)
{
}

void json_element_lod_group(jso_stream *s, const ufbx_lod_group* elem)
{
}

void json_element_skin_deformer(jso_stream *s, const ufbx_skin_deformer* elem)
{
}

void json_element_skin_cluster(jso_stream *s, const ufbx_skin_cluster* elem)
{
}

void json_element_blend_deformer(jso_stream *s, const ufbx_blend_deformer* elem)
{
}

void json_element_blend_channel(jso_stream *s, const ufbx_blend_channel* elem)
{
}

void json_element_blend_shape(jso_stream *s, const ufbx_blend_shape* elem)
{
}

void json_element_cache_deformer(jso_stream *s, const ufbx_cache_deformer* elem)
{
}

void json_element_cache_file(jso_stream *s, const ufbx_cache_file* elem)
{
}

void json_element_material(jso_stream *s, const ufbx_material* elem)
{
}

void json_element_texture(jso_stream *s, const ufbx_texture* elem)
{
}

void json_element_video(jso_stream *s, const ufbx_video* elem)
{
}

void json_element_shader(jso_stream *s, const ufbx_shader* elem)
{
}

void json_element_shader_binding(jso_stream *s, const ufbx_shader_binding* elem)
{
}

void json_element_anim_stack(jso_stream *s, const ufbx_anim_stack* elem)
{
}

void json_element_anim_layer(jso_stream *s, const ufbx_anim_layer* elem)
{
}

void json_element_anim_value(jso_stream *s, const ufbx_anim_value* elem)
{
}

void json_element_anim_curve(jso_stream *s, const ufbx_anim_curve* elem)
{
}

void json_element_display_layer(jso_stream *s, const ufbx_display_layer* elem)
{
}

void json_element_selection_set(jso_stream *s, const ufbx_selection_set* elem)
{
}

void json_element_selection_node(jso_stream *s, const ufbx_selection_node* elem)
{
}

void json_element_character(jso_stream *s, const ufbx_character* elem)
{
}

void json_element_constraint(jso_stream *s, const ufbx_constraint* elem)
{
}

void json_element_pose(jso_stream *s, const ufbx_pose* elem)
{
}

void json_element_metadata_object(jso_stream *s, const ufbx_metadata_object* elem)
{
}

void json_element(jso_stream *s, const ufbx_element *elem)
{
    jso_object(s);
    jso_prop_int(s, "id", (int)elem->element_id);
    jso_prop_string(s, "name", elem->name);
    jso_prop_string(s, "type", get_element_type_name(elem->type));

    jso_prop_array(s, "props");
    for (size_t i = 0; i < elem->props.num_props; i++) {
        json_ufbx_prop(s, &elem->props.props[i]);
    }
    jso_end_array(s);

    switch (elem->type) {
	case UFBX_ELEMENT_UNKNOWN: json_element_unknown(s, (const ufbx_unknown*)elem); break;
	case UFBX_ELEMENT_NODE: json_element_node(s, (const ufbx_node*)elem); break;
	case UFBX_ELEMENT_MESH: json_element_mesh(s, (const ufbx_mesh*)elem); break;
	case UFBX_ELEMENT_LIGHT: json_element_light(s, (const ufbx_light*)elem); break;
	case UFBX_ELEMENT_CAMERA: json_element_camera(s, (const ufbx_camera*)elem); break;
	case UFBX_ELEMENT_BONE: json_element_bone(s, (const ufbx_bone*)elem); break;
	case UFBX_ELEMENT_EMPTY: json_element_empty(s, (const ufbx_empty*)elem); break;
	case UFBX_ELEMENT_LINE_CURVE: json_element_line_curve(s, (const ufbx_line_curve*)elem); break;
	case UFBX_ELEMENT_NURBS_CURVE: json_element_nurbs_curve(s, (const ufbx_nurbs_curve*)elem); break;
	case UFBX_ELEMENT_PATCH_SURFACE: json_element_patch_surface(s, (const ufbx_patch_surface*)elem); break;
	case UFBX_ELEMENT_NURBS_SURFACE: json_element_nurbs_surface(s, (const ufbx_nurbs_surface*)elem); break;
	case UFBX_ELEMENT_NURBS_TRIM_SURFACE: json_element_nurbs_trim_surface(s, (const ufbx_nurbs_trim_surface*)elem); break;
	case UFBX_ELEMENT_NURBS_TRIM_BOUNDARY: json_element_nurbs_trim_boundary(s, (const ufbx_nurbs_trim_boundary*)elem); break;
	case UFBX_ELEMENT_PROCEDURAL_GEOMETRY: json_element_procedural_geometry(s, (const ufbx_procedural_geometry*)elem); break;
	case UFBX_ELEMENT_CAMERA_STEREO: json_element_camera_stereo(s, (const ufbx_camera_stereo*)elem); break;
	case UFBX_ELEMENT_CAMERA_SWITCHER: json_element_camera_switcher(s, (const ufbx_camera_switcher*)elem); break;
	case UFBX_ELEMENT_LOD_GROUP: json_element_lod_group(s, (const ufbx_lod_group*)elem); break;
	case UFBX_ELEMENT_SKIN_DEFORMER: json_element_skin_deformer(s, (const ufbx_skin_deformer*)elem); break;
	case UFBX_ELEMENT_SKIN_CLUSTER: json_element_skin_cluster(s, (const ufbx_skin_cluster*)elem); break;
	case UFBX_ELEMENT_BLEND_DEFORMER: json_element_blend_deformer(s, (const ufbx_blend_deformer*)elem); break;
	case UFBX_ELEMENT_BLEND_CHANNEL: json_element_blend_channel(s, (const ufbx_blend_channel*)elem); break;
	case UFBX_ELEMENT_BLEND_SHAPE: json_element_blend_shape(s, (const ufbx_blend_shape*)elem); break;
	case UFBX_ELEMENT_CACHE_DEFORMER: json_element_cache_deformer(s, (const ufbx_cache_deformer*)elem); break;
	case UFBX_ELEMENT_CACHE_FILE: json_element_cache_file(s, (const ufbx_cache_file*)elem); break;
	case UFBX_ELEMENT_MATERIAL: json_element_material(s, (const ufbx_material*)elem); break;
	case UFBX_ELEMENT_TEXTURE: json_element_texture(s, (const ufbx_texture*)elem); break;
	case UFBX_ELEMENT_VIDEO: json_element_video(s, (const ufbx_video*)elem); break;
	case UFBX_ELEMENT_SHADER: json_element_shader(s, (const ufbx_shader*)elem); break;
	case UFBX_ELEMENT_SHADER_BINDING: json_element_shader_binding(s, (const ufbx_shader_binding*)elem); break;
	case UFBX_ELEMENT_ANIM_STACK: json_element_anim_stack(s, (const ufbx_anim_stack*)elem); break;
	case UFBX_ELEMENT_ANIM_LAYER: json_element_anim_layer(s, (const ufbx_anim_layer*)elem); break;
	case UFBX_ELEMENT_ANIM_VALUE: json_element_anim_value(s, (const ufbx_anim_value*)elem); break;
	case UFBX_ELEMENT_ANIM_CURVE: json_element_anim_curve(s, (const ufbx_anim_curve*)elem); break;
	case UFBX_ELEMENT_DISPLAY_LAYER: json_element_display_layer(s, (const ufbx_display_layer*)elem); break;
	case UFBX_ELEMENT_SELECTION_SET: json_element_selection_set(s, (const ufbx_selection_set*)elem); break;
	case UFBX_ELEMENT_SELECTION_NODE: json_element_selection_node(s, (const ufbx_selection_node*)elem); break;
	case UFBX_ELEMENT_CHARACTER: json_element_character(s, (const ufbx_character*)elem); break;
	case UFBX_ELEMENT_CONSTRAINT: json_element_constraint(s, (const ufbx_constraint*)elem); break;
	case UFBX_ELEMENT_POSE: json_element_pose(s, (const ufbx_pose*)elem); break;
	case UFBX_ELEMENT_METADATA_OBJECT: json_element_metadata_object(s, (const ufbx_metadata_object*)elem); break;
	case UFBX_NUM_ELEMENT_TYPES: break;
    }

    jso_end_object(s);
}

void json_scene(jso_stream *s, ufbx_scene *scene)
{
    jso_object(s);

    jso_prop_object(s, "metadata");
    jso_prop_string(s, "creator", scene->metadata.creator);
    jso_prop(s, "original");
    json_application(s, &scene->metadata.original_application);
    jso_prop(s, "lastSaved");
    json_application(s, &scene->metadata.latest_application);
    jso_end_object(s);

    jso_prop_int(s, "root", (int)scene->root_node->element_id);

    jso_prop_array(s, "elements");
    for (ufbx_element *elem : scene->elements) {
        json_element(s, elem);
    }
    jso_end_array(s);

    jso_end_object(s);
}

extern "C" {

ufbx_scene *scene;
void (*g_pause_cb)();
void (*g_resume_cb)();
int g_render_fb;

char *EMSCRIPTEN_KEEPALIVE load_scene(const void *data, size_t size)
{
    jso_stream s;
    jso_init_growable(&s);

    ufbx_error error;
    ufbx_free_scene(scene);
    scene = ufbx_load_memory(data, size, NULL, &error);
    if (!scene) {
        char buf[1024];
        ufbx_format_error(buf, sizeof(buf), &error);
        fprintf(stderr, "%s\n", buf);

        jso_object(&s);
        jso_prop_string(&s, "error", buf);
        jso_end_object(&s);
    } else {
        json_scene(&s, scene);
    }

    return jso_close_growable(&s);
}

void EMSCRIPTEN_KEEPALIVE viewer_init(void (*pause_cb)(), void (*resume_cb)())
{
    printf("Init? %p\n", pause_cb);
    g_pause_cb = pause_cb;
    g_resume_cb = resume_cb;
}

int EMSCRIPTEN_KEEPALIVE viewer_get_fb()
{
    return g_render_fb;
}

}

sg_image color_target;
sg_image depth_target;
sg_pass main_pass;

sg_shader copy_shader;
sg_pipeline copy_pipe;

sg_buffer quad_vertex_buffer;

void init(void)
{
    {
        sg_desc d = { };
        d.context = sapp_sgcontext();
        sg_setup(&d);
    }

    {
        sgl_desc_t d = { };
        d.color_format = SG_PIXELFORMAT_RGBA8;
        d.depth_format = SG_PIXELFORMAT_DEPTH;
        sgl_setup(&d);
    }

    {
        sg_image_desc d = { };
        d.width = sapp_width();
        d.height = sapp_height();
        d.render_target = true;
        d.pixel_format = SG_PIXELFORMAT_RGBA8;
        color_target = sg_make_image(&d);
    }

    {
        sg_image_desc d = { };
        d.width = sapp_width();
        d.height = sapp_height();
        d.render_target = true;
        d.pixel_format = SG_PIXELFORMAT_DEPTH;
        depth_target = sg_make_image(&d);
    }

    {
        sg_pass_desc d = { };
        d.color_attachments[0].image = color_target;
        d.depth_stencil_attachment.image = depth_target;
        main_pass = sg_make_pass(&d);
    }

    {
        sg_pass_info info = sg_query_pass_info(main_pass);
        g_render_fb = info.hack_gl_fb;
    }

    sg_backend backend = sg_query_backend();
    copy_shader = sg_make_shader(copy_shader_desc(backend));

    {
        sg_pipeline_desc d = { };
        d.shader = copy_shader;
        d.layout.attrs[0].format = SG_VERTEXFORMAT_FLOAT2;
        copy_pipe = sg_make_pipeline(&d);
    }

    {
        const um_vec2 verts[] = {
            um_v2(0.0f, 0.0f),
            um_v2(2.0f, 0.0f),
            um_v2(0.0f, 2.0f),
        };
        sg_buffer_desc d = { };
        d.type = SG_BUFFERTYPE_VERTEXBUFFER;
        d.data = SG_RANGE(verts);
        quad_vertex_buffer = sg_make_buffer(&d);
    }
}

static int hackframe = 0;

um_vec2 mouse_pos;

void sgl_v2(um_vec2 v) { sgl_v2f(v.x, v.y); }

void event(const sapp_event *ev)
{
    switch (ev->type) {
    case SAPP_EVENTTYPE_MOUSE_MOVE:
        hackframe = 0;
        mouse_pos = um_v2(ev->mouse_x, ev->mouse_y);
        sapp_request_resume();
        if (g_resume_cb) {
            g_resume_cb();
        }
        break;
    default: break;
    }
}

void frame(void)
{
    printf("FRAME: %d\n", hackframe);
    if (hackframe++ > 20) {
        sapp_request_pause();
        if (g_pause_cb) {
            printf("Pause?\n");
            g_pause_cb();
        }
    }

	sg_pass_action action = { };
	action.colors[0].action = SG_ACTION_CLEAR;
	action.colors[0].value = { 0.1f, 0.1f, 0.2f, 1.0f };

    sgl_matrix_mode_projection();
    sgl_load_identity();
    sgl_ortho(0.0f, (float)sapp_width(), 0.0f, (float)sapp_height(), -1.0f, 1.0f);

    sgl_begin_quads();
    sgl_c3f(1.0f, 0.5f, 0.2f);
    sgl_v2(mouse_pos + um_v2(-5.0f, -5.0f));
    sgl_v2(mouse_pos + um_v2(+5.0f, -5.0f));
    sgl_v2(mouse_pos + um_v2(+5.0f, +5.0f));
    sgl_v2(mouse_pos + um_v2(-5.0f, +5.0f));
    sgl_end();

    sg_begin_pass(main_pass, &action);

    sgl_draw();

    sg_end_pass();

    sg_begin_default_pass(&action, sapp_width(), sapp_height());

    sg_apply_pipeline(copy_pipe);

    sg_bindings binds = { };
    binds.vertex_buffers[0] = quad_vertex_buffer;
    binds.fs_images[SLOT_u_texture] = color_target;
    sg_apply_bindings(&binds);
    sg_draw(0, 3, 1);

    sg_end_pass();

    sg_commit();
}

void cleanup(void)
{
    sg_shutdown();
}

sapp_desc sokol_main(int argc, char* argv[]) {
    sapp_desc d = { };
    d.init_cb = &init,
    d.event_cb = &event,
    d.frame_cb = &frame,
    d.cleanup_cb = &cleanup,
    d.width = 800,
    d.height = 600,
    d.html5_canvas_name = "render-canvas";
    return d;
}
