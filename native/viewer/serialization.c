#include "serialization.h"

const char *prop_type_str(ufbx_prop_type type)
{
	switch (type) {
	case UFBX_PROP_UNKNOWN: return "unknown";
	case UFBX_PROP_BOOLEAN: return "boolean";
	case UFBX_PROP_INTEGER: return "integer";
	case UFBX_PROP_NUMBER: return "number";
	case UFBX_PROP_VECTOR: return "vector";
	case UFBX_PROP_COLOR: return "color";
	case UFBX_PROP_STRING: return "string";
	case UFBX_PROP_DATE_TIME: return "date_time";
	case UFBX_PROP_TRANSLATION: return "translation";
	case UFBX_PROP_ROTATION: return "rotation";
	case UFBX_PROP_SCALING: return "scaling";
	case UFBX_PROP_DISTANCE: return "distance";
	case UFBX_PROP_COMPOUND: return "compound";
	default: return "";
	}
}

const char *element_type_str(ufbx_element_type type)
{
    switch (type) {
	case UFBX_ELEMENT_UNKNOWN: return "unknown";
	case UFBX_ELEMENT_NODE: return "node";
	case UFBX_ELEMENT_MESH: return "mesh";
	case UFBX_ELEMENT_LIGHT: return "light";
	case UFBX_ELEMENT_CAMERA: return "camera";
	case UFBX_ELEMENT_BONE: return "bone";
	case UFBX_ELEMENT_EMPTY: return "empty";
	case UFBX_ELEMENT_MARKER: return "marker";
	case UFBX_ELEMENT_LINE_CURVE: return "line_curve";
	case UFBX_ELEMENT_NURBS_CURVE: return "nurbs_curve";
	case UFBX_ELEMENT_NURBS_TRIM_SURFACE: return "nurbs_trim_surface";
	case UFBX_ELEMENT_NURBS_TRIM_BOUNDARY: return "nurbs_trim_boundary";
	case UFBX_ELEMENT_PROCEDURAL_GEOMETRY: return "procedural_geometry";
	case UFBX_ELEMENT_STEREO_CAMERA: return "stereo_camera";
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
	case UFBX_ELEMENT_METADATA_OBJECT: return "metadataObject";
	default: return "";
    }
}

void serialize_props(jso_stream *s, ufbx_props *props)
{
	jso_array(s);
	for (size_t i = 0; i < props->props.count; i++) {
		ufbx_prop *prop = &props->props.data[i];
		jso_single_line(s);
		jso_object(s);
		jso_prop_ustring(s, "name", prop->name);
		jso_prop_string(s, "type", prop_type_str(prop->type));
		jso_prop_array(s, "value");
		jso_double(s, prop->value_vec3.x);
		jso_double(s, prop->value_vec3.y);
		jso_double(s, prop->value_vec3.z);
		jso_end_array(s);
		jso_prop_ustring(s, "valueStr", prop->value_str);
		jso_end_object(s);
	}
	jso_end_array(s);
}

void serialize_element_unknown(jso_stream *s, ufbx_unknown* elem)
{
	jso_prop_ustring(s, "superType", elem->super_type);
}

void serialize_element_node(jso_stream *s, ufbx_node* elem)
{
	jso_prop_string(s, "attribType", element_type_str(elem->attrib_type));

    jso_prop_array(s, "attribs");
	for (size_t i = 0; i < elem->all_attribs.count; i++) {
        jso_int(s, (int)elem->all_attribs.data[i]->element_id);
    }
    jso_end_array(s);

    jso_prop_array(s, "children");
	for (size_t i = 0; i < elem->children.count; i++) {
        jso_int(s, (int)elem->children.data[i]->element_id);
    }
    jso_end_array(s);

	jso_prop_object(s, "fields");
	jso_prop_vec3(s, "translation", elem->local_transform.translation);
	jso_prop_vec3(s, "rotation", elem->euler_rotation);
	jso_prop_vec3(s, "scale", elem->local_transform.scale);

	ufbx_vec3 geo_euler = ufbx_quat_to_euler(elem->geometry_transform.rotation, UFBX_ROTATION_XYZ);
	jso_prop_vec3(s, "geo_translation", elem->geometry_transform.translation);
	jso_prop_vec3(s, "geo_rotation", geo_euler);
	jso_prop_vec3(s, "geo_scale", elem->geometry_transform.scale);

    jso_end_object(s);
}

void serialize_element_mesh(jso_stream *s, ufbx_mesh* elem)
{
    jso_prop_array(s, "materials");
	for (size_t i = 0; i < elem->materials.count; i++) {
		if (elem->materials.data[i].material) {
			jso_int(s, (int)elem->materials.data[i].material->element_id);
		}
    }
    jso_end_array(s);

    jso_prop_array(s, "deformers");
	for (size_t i = 0; i < elem->all_deformers.count; i++) {
		jso_int(s, (int)elem->all_deformers.data[i]->element_id);
    }
    jso_end_array(s);

	jso_prop_int(s, "numFaces", elem->num_faces);
	jso_prop_int(s, "numVertices", elem->num_vertices);
	jso_prop_int(s, "numIndices", elem->num_indices);
}

void serialize_element_light(jso_stream *s, ufbx_light* elem)
{
	jso_prop_object(s, "fields");
	jso_prop_vec3(s, "color", elem->color);
	jso_prop_double(s, "intensity", elem->intensity);
    jso_end_object(s);
}

void serialize_element_camera(jso_stream *s, ufbx_camera* elem)
{
	jso_prop_object(s, "fields");
	jso_prop_vec2(s, "resolution", elem->resolution);
	jso_prop_vec2(s, "field_of_view_deg", elem->field_of_view_deg);
    jso_end_object(s);
}

void serialize_element_bone(jso_stream *s, ufbx_bone* elem)
{
}

void serialize_element_empty(jso_stream *s, ufbx_empty* elem)
{
}

void serialize_element_marker(jso_stream *s, ufbx_marker* elem)
{
}

void serialize_element_line_curve(jso_stream *s, ufbx_line_curve* elem)
{
}

void serialize_element_nurbs_curve(jso_stream *s, ufbx_nurbs_curve* elem)
{
}

void serialize_element_nurbs_surface(jso_stream *s, ufbx_nurbs_surface* elem)
{
}

void serialize_element_nurbs_trim_surface(jso_stream *s, ufbx_nurbs_trim_surface* elem)
{
}

void serialize_element_nurbs_trim_boundary(jso_stream *s, ufbx_nurbs_trim_boundary* elem)
{
}

void serialize_element_procedural_geometry(jso_stream *s, ufbx_procedural_geometry* elem)
{
}

void serialize_element_stereo_camera(jso_stream *s, ufbx_stereo_camera* elem)
{
}

void serialize_element_camera_switcher(jso_stream *s, ufbx_camera_switcher* elem)
{
}

void serialize_element_lod_group(jso_stream *s, ufbx_lod_group* elem)
{
}

void serialize_element_skin_deformer(jso_stream *s, ufbx_skin_deformer* elem)
{
    jso_prop_array(s, "clusters");
	for (size_t i = 0; i < elem->clusters.count; i++) {
		jso_int(s, (int)elem->clusters.data[i]->element_id);
    }
    jso_end_array(s);
}

void serialize_element_skin_cluster(jso_stream *s, ufbx_skin_cluster* elem)
{
	jso_prop_int(s, "bone", elem->bone_node->element_id);
}

void serialize_element_blend_deformer(jso_stream *s, ufbx_blend_deformer* elem)
{
    jso_prop_array(s, "channels");
	for (size_t i = 0; i < elem->channels.count; i++) {
		jso_int(s, (int)elem->channels.data[i]->element_id);
    }
    jso_end_array(s);
}

void serialize_element_blend_channel(jso_stream *s, ufbx_blend_channel* elem)
{
    jso_prop_array(s, "keyframes");
	for (size_t i = 0; i < elem->keyframes.count; i++) {
		jso_int(s, (int)elem->keyframes.data[i].shape->element_id);
    }
    jso_end_array(s);
}

void serialize_element_blend_shape(jso_stream *s, ufbx_blend_shape* elem)
{
}

void serialize_element_cache_deformer(jso_stream *s, ufbx_cache_deformer* elem)
{
}

void serialize_element_cache_file(jso_stream *s, ufbx_cache_file* elem)
{
}

void serialize_element_material(jso_stream *s, ufbx_material* elem)
{
    jso_prop_array(s, "textures");
	for (size_t i = 0; i < elem->textures.count; i++) {
        jso_int(s, (int)elem->textures.data[i].texture->element_id);
    }
    jso_end_array(s);
}

void serialize_element_texture(jso_stream *s, ufbx_texture* elem)
{
}

void serialize_element_video(jso_stream *s, ufbx_video* elem)
{
}

void serialize_element_shader(jso_stream *s, ufbx_shader* elem)
{
}

void serialize_element_shader_binding(jso_stream *s, ufbx_shader_binding* elem)
{
}

void serialize_element_anim_stack(jso_stream *s, ufbx_anim_stack* elem)
{
}

void serialize_element_anim_layer(jso_stream *s, ufbx_anim_layer* elem)
{
}

void serialize_element_anim_value(jso_stream *s, ufbx_anim_value* elem)
{
}

void serialize_element_anim_curve(jso_stream *s, ufbx_anim_curve* elem)
{
}

void serialize_element_display_layer(jso_stream *s, ufbx_display_layer* elem)
{
}

void serialize_element_selection_set(jso_stream *s, ufbx_selection_set* elem)
{
}

void serialize_element_selection_node(jso_stream *s, ufbx_selection_node* elem)
{
}

void serialize_element_character(jso_stream *s, ufbx_character* elem)
{
}

void serialize_element_constraint(jso_stream *s, ufbx_constraint* elem)
{
}

void serialize_element_pose(jso_stream *s, ufbx_pose* elem)
{
}

void serialize_element_metadata_object(jso_stream *s, ufbx_metadata_object* elem)
{
}

void serialize_element(jso_stream *s, ufbx_element *elem)
{
	jso_object(s);
	jso_prop_ustring(s, "name", elem->name);
	jso_prop_string(s, "type", element_type_str(elem->type));
	jso_prop_int(s, "id", (int)elem->element_id);
	jso_prop(s, "props");
	serialize_props(s, &elem->props);

    switch (elem->type) {
	case UFBX_ELEMENT_UNKNOWN: serialize_element_unknown(s, (ufbx_unknown*)elem); break;
	case UFBX_ELEMENT_NODE: serialize_element_node(s, (ufbx_node*)elem); break;
	case UFBX_ELEMENT_MESH: serialize_element_mesh(s, (ufbx_mesh*)elem); break;
	case UFBX_ELEMENT_LIGHT: serialize_element_light(s, (ufbx_light*)elem); break;
	case UFBX_ELEMENT_CAMERA: serialize_element_camera(s, (ufbx_camera*)elem); break;
	case UFBX_ELEMENT_BONE: serialize_element_bone(s, (ufbx_bone*)elem); break;
	case UFBX_ELEMENT_EMPTY: serialize_element_empty(s, (ufbx_empty*)elem); break;
	case UFBX_ELEMENT_MARKER: serialize_element_marker(s, (ufbx_marker*)elem); break;
	case UFBX_ELEMENT_LINE_CURVE: serialize_element_line_curve(s, (ufbx_line_curve*)elem); break;
	case UFBX_ELEMENT_NURBS_CURVE: serialize_element_nurbs_curve(s, (ufbx_nurbs_curve*)elem); break;
	case UFBX_ELEMENT_NURBS_SURFACE: serialize_element_nurbs_surface(s, (ufbx_nurbs_surface*)elem); break;
	case UFBX_ELEMENT_NURBS_TRIM_SURFACE: serialize_element_nurbs_trim_surface(s, (ufbx_nurbs_trim_surface*)elem); break;
	case UFBX_ELEMENT_NURBS_TRIM_BOUNDARY: serialize_element_nurbs_trim_boundary(s, (ufbx_nurbs_trim_boundary*)elem); break;
	case UFBX_ELEMENT_PROCEDURAL_GEOMETRY: serialize_element_procedural_geometry(s, (ufbx_procedural_geometry*)elem); break;
	case UFBX_ELEMENT_STEREO_CAMERA: serialize_element_stereo_camera(s, (ufbx_stereo_camera*)elem); break;
	case UFBX_ELEMENT_CAMERA_SWITCHER: serialize_element_camera_switcher(s, (ufbx_camera_switcher*)elem); break;
	case UFBX_ELEMENT_LOD_GROUP: serialize_element_lod_group(s, (ufbx_lod_group*)elem); break;
	case UFBX_ELEMENT_SKIN_DEFORMER: serialize_element_skin_deformer(s, (ufbx_skin_deformer*)elem); break;
	case UFBX_ELEMENT_SKIN_CLUSTER: serialize_element_skin_cluster(s, (ufbx_skin_cluster*)elem); break;
	case UFBX_ELEMENT_BLEND_DEFORMER: serialize_element_blend_deformer(s, (ufbx_blend_deformer*)elem); break;
	case UFBX_ELEMENT_BLEND_CHANNEL: serialize_element_blend_channel(s, (ufbx_blend_channel*)elem); break;
	case UFBX_ELEMENT_BLEND_SHAPE: serialize_element_blend_shape(s, (ufbx_blend_shape*)elem); break;
	case UFBX_ELEMENT_CACHE_DEFORMER: serialize_element_cache_deformer(s, (ufbx_cache_deformer*)elem); break;
	case UFBX_ELEMENT_CACHE_FILE: serialize_element_cache_file(s, (ufbx_cache_file*)elem); break;
	case UFBX_ELEMENT_MATERIAL: serialize_element_material(s, (ufbx_material*)elem); break;
	case UFBX_ELEMENT_TEXTURE: serialize_element_texture(s, (ufbx_texture*)elem); break;
	case UFBX_ELEMENT_VIDEO: serialize_element_video(s, (ufbx_video*)elem); break;
	case UFBX_ELEMENT_SHADER: serialize_element_shader(s, (ufbx_shader*)elem); break;
	case UFBX_ELEMENT_SHADER_BINDING: serialize_element_shader_binding(s, (ufbx_shader_binding*)elem); break;
	case UFBX_ELEMENT_ANIM_STACK: serialize_element_anim_stack(s, (ufbx_anim_stack*)elem); break;
	case UFBX_ELEMENT_ANIM_LAYER: serialize_element_anim_layer(s, (ufbx_anim_layer*)elem); break;
	case UFBX_ELEMENT_ANIM_VALUE: serialize_element_anim_value(s, (ufbx_anim_value*)elem); break;
	case UFBX_ELEMENT_ANIM_CURVE: serialize_element_anim_curve(s, (ufbx_anim_curve*)elem); break;
	case UFBX_ELEMENT_DISPLAY_LAYER: serialize_element_display_layer(s, (ufbx_display_layer*)elem); break;
	case UFBX_ELEMENT_SELECTION_SET: serialize_element_selection_set(s, (ufbx_selection_set*)elem); break;
	case UFBX_ELEMENT_SELECTION_NODE: serialize_element_selection_node(s, (ufbx_selection_node*)elem); break;
	case UFBX_ELEMENT_CHARACTER: serialize_element_character(s, (ufbx_character*)elem); break;
	case UFBX_ELEMENT_CONSTRAINT: serialize_element_constraint(s, (ufbx_constraint*)elem); break;
	case UFBX_ELEMENT_POSE: serialize_element_pose(s, (ufbx_pose*)elem); break;
	case UFBX_ELEMENT_METADATA_OBJECT: serialize_element_metadata_object(s, (ufbx_metadata_object*)elem); break;
	default: break;
    }

	jso_end_object(s);
}

void serialize_scene(jso_stream *s, ufbx_scene *scene)
{
	jso_object(s);

	jso_prop_object(s, "settings");
	jso_prop(s, "props");
	serialize_props(s, &scene->settings.props);
	jso_end_object(s);

	if (scene->root_node) {
		jso_prop_int(s, "rootNode", (int)scene->root_node->element_id);
	}

	jso_prop_array(s, "elements");
	for (size_t i = 0; i < scene->elements.count; i++) {
		serialize_element(s, scene->elements.data[i]);
	}
	jso_end_array(s);

	jso_end_object(s);
}
