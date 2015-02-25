/*
 Copyright (C) Intel Corp.  2006.  All Rights Reserved.
 Intel funded Tungsten Graphics to
 develop this 3D driver.

 Permission is hereby granted, free of charge, to any person obtaining
 a copy of this software and associated documentation files (the
 "Software"), to deal in the Software without restriction, including
 without limitation the rights to use, copy, modify, merge, publish,
 distribute, sublicense, and/or sell copies of the Software, and to
 permit persons to whom the Software is furnished to do so, subject to
 the following conditions:

 The above copyright notice and this permission notice (including the
 next paragraph) shall be included in all copies or substantial
 portions of the Software.

 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 IN NO EVENT SHALL THE COPYRIGHT OWNER(S) AND/OR ITS SUPPLIERS BE
 LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
 OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
 WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

 **********************************************************************/
 /*
  * Authors:
  *   Keith Whitwell <keithw@vmware.com>
  */


#ifndef BRWCONTEXT_INC
#define BRWCONTEXT_INC

#include <stdbool.h>
#include <string.h>
#include "main/imports.h"
#include "main/macros.h"
//#include "main/mm.h"  // LunarG: Remove
#include "main/mtypes.h"
#include "brw_structs.h"

typedef struct drm_intel_bo drm_intel_bo;

#ifdef __cplusplus
extern "C" {
#endif
#include "intel_debug.h"
#include "intel_screen.h"
//#include "intel_tex_obj.h" // LunarG: Remove
//#include "intel_resolve_map.h"  // LunarG: Remove

/* Glossary:
 *
 * URB - uniform resource buffer.  A mid-sized buffer which is
 * partitioned between the fixed function units and used for passing
 * values (vertices, primitives, constants) between them.
 *
 * CURBE - constant URB entry.  An urb region (entry) used to hold
 * constant values which the fixed function units can be instructed to
 * preload into the GRF when spawning a thread.
 *
 * VUE - vertex URB entry.  An urb entry holding a vertex and usually
 * a vertex header.  The header contains control information and
 * things like primitive type, Begin/end flags and clip codes.
 *
 * PUE - primitive URB entry.  An urb entry produced by the setup (SF)
 * unit holding rasterization and interpolation parameters.
 *
 * GRF - general register file.  One of several register files
 * addressable by programmed threads.  The inputs (r0, payload, curbe,
 * urb) of the thread are preloaded to this area before the thread is
 * spawned.  The registers are individually 8 dwords wide and suitable
 * for general usage.  Registers holding thread input values are not
 * special and may be overwritten.
 *
 * MRF - message register file.  Threads communicate (and terminate)
 * by sending messages.  Message parameters are placed in contiguous
 * MRF registers.  All program output is via these messages.  URB
 * entries are populated by sending a message to the shared URB
 * function containing the new data, together with a control word,
 * often an unmodified copy of R0.
 *
 * R0 - GRF register 0.  Typically holds control information used when
 * sending messages to other threads.
 *
 * EU or GEN4 EU: The name of the programmable subsystem of the
 * i965 hardware.  Threads are executed by the EU, the registers
 * described above are part of the EU architecture.
 *
 * Fixed function units:
 *
 * CS - Command streamer.  Notional first unit, little software
 * interaction.  Holds the URB entries used for constant data, ie the
 * CURBEs.
 *
 * VF/VS - Vertex Fetch / Vertex Shader.  The fixed function part of
 * this unit is responsible for pulling vertices out of vertex buffers
 * in vram and injecting them into the processing pipe as VUEs.  If
 * enabled, it first passes them to a VS thread which is a good place
 * for the driver to implement any active vertex shader.
 *
 * GS - Geometry Shader.  This corresponds to a new DX10 concept.  If
 * enabled, incoming strips etc are passed to GS threads in individual
 * line/triangle/point units.  The GS thread may perform arbitary
 * computation and emit whatever primtives with whatever vertices it
 * chooses.  This makes GS an excellent place to implement GL's
 * unfilled polygon modes, though of course it is capable of much
 * more.  Additionally, GS is used to translate away primitives not
 * handled by latter units, including Quads and Lineloops.
 *
 * CS - Clipper.  Mesa's clipping algorithms are imported to run on
 * this unit.  The fixed function part performs cliptesting against
 * the 6 fixed clipplanes and makes descisions on whether or not the
 * incoming primitive needs to be passed to a thread for clipping.
 * User clip planes are handled via cooperation with the VS thread.
 *
 * SF - Strips Fans or Setup: Triangles are prepared for
 * rasterization.  Interpolation coefficients are calculated.
 * Flatshading and two-side lighting usually performed here.
 *
 * WM - Windower.  Interpolation of vertex attributes performed here.
 * Fragment shader implemented here.  SIMD aspects of EU taken full
 * advantage of, as pixels are processed in blocks of 16.
 *
 * CC - Color Calculator.  No EU threads associated with this unit.
 * Handles blending and (presumably) depth and stencil testing.
 */

#define BRW_MAX_CURBE                    (32*16)

struct brw_context;
struct brw_instruction;
struct brw_vs_prog_key;
struct brw_vec4_prog_key;
struct brw_wm_prog_key;
struct brw_wm_prog_data;

enum brw_state_id {
   BRW_STATE_URB_FENCE,
   BRW_STATE_FRAGMENT_PROGRAM,
   BRW_STATE_GEOMETRY_PROGRAM,
   BRW_STATE_VERTEX_PROGRAM,
   BRW_STATE_CURBE_OFFSETS,
   BRW_STATE_REDUCED_PRIMITIVE,
   BRW_STATE_PRIMITIVE,
   BRW_STATE_CONTEXT,
   BRW_STATE_PSP,
   BRW_STATE_SURFACES,
   BRW_STATE_VS_BINDING_TABLE,
   BRW_STATE_GS_BINDING_TABLE,
   BRW_STATE_PS_BINDING_TABLE,
   BRW_STATE_INDICES,
   BRW_STATE_VERTICES,
   BRW_STATE_BATCH,
   BRW_STATE_INDEX_BUFFER,
   BRW_STATE_VS_CONSTBUF,
   BRW_STATE_GS_CONSTBUF,
   BRW_STATE_PROGRAM_CACHE,
   BRW_STATE_STATE_BASE_ADDRESS,
   BRW_STATE_VUE_MAP_VS,
   BRW_STATE_VUE_MAP_GEOM_OUT,
   BRW_STATE_TRANSFORM_FEEDBACK,
   BRW_STATE_RASTERIZER_DISCARD,
   BRW_STATE_STATS_WM,
   BRW_STATE_UNIFORM_BUFFER,
   BRW_STATE_ATOMIC_BUFFER,
   BRW_STATE_META_IN_PROGRESS,
   BRW_STATE_INTERPOLATION_MAP,
   BRW_STATE_PUSH_CONSTANT_ALLOCATION,
   BRW_STATE_NUM_SAMPLES,
   BRW_NUM_STATE_BITS
};

#define BRW_NEW_URB_FENCE               (1 << BRW_STATE_URB_FENCE)
#define BRW_NEW_FRAGMENT_PROGRAM        (1 << BRW_STATE_FRAGMENT_PROGRAM)
#define BRW_NEW_GEOMETRY_PROGRAM        (1 << BRW_STATE_GEOMETRY_PROGRAM)
#define BRW_NEW_VERTEX_PROGRAM          (1 << BRW_STATE_VERTEX_PROGRAM)
#define BRW_NEW_CURBE_OFFSETS           (1 << BRW_STATE_CURBE_OFFSETS)
#define BRW_NEW_REDUCED_PRIMITIVE       (1 << BRW_STATE_REDUCED_PRIMITIVE)
#define BRW_NEW_PRIMITIVE               (1 << BRW_STATE_PRIMITIVE)
#define BRW_NEW_CONTEXT                 (1 << BRW_STATE_CONTEXT)
#define BRW_NEW_PSP                     (1 << BRW_STATE_PSP)
#define BRW_NEW_SURFACES		(1 << BRW_STATE_SURFACES)
#define BRW_NEW_VS_BINDING_TABLE	(1 << BRW_STATE_VS_BINDING_TABLE)
#define BRW_NEW_GS_BINDING_TABLE	(1 << BRW_STATE_GS_BINDING_TABLE)
#define BRW_NEW_PS_BINDING_TABLE	(1 << BRW_STATE_PS_BINDING_TABLE)
#define BRW_NEW_INDICES			(1 << BRW_STATE_INDICES)
#define BRW_NEW_VERTICES		(1 << BRW_STATE_VERTICES)
/**
 * Used for any batch entry with a relocated pointer that will be used
 * by any 3D rendering.
 */
#define BRW_NEW_BATCH                  (1 << BRW_STATE_BATCH)
/** \see brw.state.depth_region */
#define BRW_NEW_INDEX_BUFFER           (1 << BRW_STATE_INDEX_BUFFER)
#define BRW_NEW_VS_CONSTBUF            (1 << BRW_STATE_VS_CONSTBUF)
#define BRW_NEW_GS_CONSTBUF            (1 << BRW_STATE_GS_CONSTBUF)
#define BRW_NEW_PROGRAM_CACHE		(1 << BRW_STATE_PROGRAM_CACHE)
#define BRW_NEW_STATE_BASE_ADDRESS	(1 << BRW_STATE_STATE_BASE_ADDRESS)
#define BRW_NEW_VUE_MAP_VS		(1 << BRW_STATE_VUE_MAP_VS)
#define BRW_NEW_VUE_MAP_GEOM_OUT	(1 << BRW_STATE_VUE_MAP_GEOM_OUT)
#define BRW_NEW_TRANSFORM_FEEDBACK	(1 << BRW_STATE_TRANSFORM_FEEDBACK)
#define BRW_NEW_RASTERIZER_DISCARD	(1 << BRW_STATE_RASTERIZER_DISCARD)
#define BRW_NEW_STATS_WM		(1 << BRW_STATE_STATS_WM)
#define BRW_NEW_UNIFORM_BUFFER          (1 << BRW_STATE_UNIFORM_BUFFER)
#define BRW_NEW_ATOMIC_BUFFER           (1 << BRW_STATE_ATOMIC_BUFFER)
#define BRW_NEW_META_IN_PROGRESS        (1 << BRW_STATE_META_IN_PROGRESS)
#define BRW_NEW_INTERPOLATION_MAP       (1 << BRW_STATE_INTERPOLATION_MAP)
#define BRW_NEW_PUSH_CONSTANT_ALLOCATION (1 << BRW_STATE_PUSH_CONSTANT_ALLOCATION)
#define BRW_NEW_NUM_SAMPLES             (1 << BRW_STATE_NUM_SAMPLES)

struct brw_state_flags {
   /** State update flags signalled by mesa internals */
   GLuint mesa;
   /**
    * State update flags signalled as the result of brw_tracked_state updates
    */
   GLuint brw;
   /** State update flags signalled by brw_state_cache.c searches */
   GLuint cache;
};

#define AUB_TRACE_TYPE_MASK		0x0000ff00
#define AUB_TRACE_TYPE_NOTYPE		(0 << 8)
#define AUB_TRACE_TYPE_BATCH		(1 << 8)
#define AUB_TRACE_TYPE_VERTEX_BUFFER	(5 << 8)
#define AUB_TRACE_TYPE_2D_MAP		(6 << 8)
#define AUB_TRACE_TYPE_CUBE_MAP		(7 << 8)
#define AUB_TRACE_TYPE_VOLUME_MAP	(9 << 8)
#define AUB_TRACE_TYPE_1D_MAP		(10 << 8)
#define AUB_TRACE_TYPE_CONSTANT_BUFFER	(11 << 8)
#define AUB_TRACE_TYPE_CONSTANT_URB	(12 << 8)
#define AUB_TRACE_TYPE_INDEX_BUFFER	(13 << 8)
#define AUB_TRACE_TYPE_GENERAL		(14 << 8)
#define AUB_TRACE_TYPE_SURFACE		(15 << 8)

/**
 * state_struct_type enum values are encoded with the top 16 bits representing
 * the type to be delivered to the .aub file, and the bottom 16 bits
 * representing the subtype.  This macro performs the encoding.
 */
#define ENCODE_SS_TYPE(type, subtype) (((type) << 16) | (subtype))

enum state_struct_type {
   AUB_TRACE_VS_STATE =			ENCODE_SS_TYPE(AUB_TRACE_TYPE_GENERAL, 1),
   AUB_TRACE_GS_STATE =			ENCODE_SS_TYPE(AUB_TRACE_TYPE_GENERAL, 2),
   AUB_TRACE_CLIP_STATE =		ENCODE_SS_TYPE(AUB_TRACE_TYPE_GENERAL, 3),
   AUB_TRACE_SF_STATE =			ENCODE_SS_TYPE(AUB_TRACE_TYPE_GENERAL, 4),
   AUB_TRACE_WM_STATE =			ENCODE_SS_TYPE(AUB_TRACE_TYPE_GENERAL, 5),
   AUB_TRACE_CC_STATE =			ENCODE_SS_TYPE(AUB_TRACE_TYPE_GENERAL, 6),
   AUB_TRACE_CLIP_VP_STATE =		ENCODE_SS_TYPE(AUB_TRACE_TYPE_GENERAL, 7),
   AUB_TRACE_SF_VP_STATE =		ENCODE_SS_TYPE(AUB_TRACE_TYPE_GENERAL, 8),
   AUB_TRACE_CC_VP_STATE =		ENCODE_SS_TYPE(AUB_TRACE_TYPE_GENERAL, 0x9),
   AUB_TRACE_SAMPLER_STATE =		ENCODE_SS_TYPE(AUB_TRACE_TYPE_GENERAL, 0xa),
   AUB_TRACE_KERNEL_INSTRUCTIONS =	ENCODE_SS_TYPE(AUB_TRACE_TYPE_GENERAL, 0xb),
   AUB_TRACE_SCRATCH_SPACE =		ENCODE_SS_TYPE(AUB_TRACE_TYPE_GENERAL, 0xc),
   AUB_TRACE_SAMPLER_DEFAULT_COLOR =    ENCODE_SS_TYPE(AUB_TRACE_TYPE_GENERAL, 0xd),

   AUB_TRACE_SCISSOR_STATE =		ENCODE_SS_TYPE(AUB_TRACE_TYPE_GENERAL, 0x15),
   AUB_TRACE_BLEND_STATE =		ENCODE_SS_TYPE(AUB_TRACE_TYPE_GENERAL, 0x16),
   AUB_TRACE_DEPTH_STENCIL_STATE =	ENCODE_SS_TYPE(AUB_TRACE_TYPE_GENERAL, 0x17),

   AUB_TRACE_VERTEX_BUFFER =		ENCODE_SS_TYPE(AUB_TRACE_TYPE_VERTEX_BUFFER, 0),
   AUB_TRACE_BINDING_TABLE =		ENCODE_SS_TYPE(AUB_TRACE_TYPE_SURFACE, 0x100),
   AUB_TRACE_SURFACE_STATE =		ENCODE_SS_TYPE(AUB_TRACE_TYPE_SURFACE, 0x200),
   AUB_TRACE_VS_CONSTANTS =		ENCODE_SS_TYPE(AUB_TRACE_TYPE_CONSTANT_BUFFER, 0),
   AUB_TRACE_WM_CONSTANTS =		ENCODE_SS_TYPE(AUB_TRACE_TYPE_CONSTANT_BUFFER, 1),
};

/**
 * Decode a state_struct_type value to determine the type that should be
 * stored in the .aub file.
 */
static inline uint32_t AUB_TRACE_TYPE(enum state_struct_type ss_type)
{
   return (ss_type & 0xFFFF0000) >> 16;
}

/**
 * Decode a state_struct_type value to determine the subtype that should be
 * stored in the .aub file.
 */
static inline uint32_t AUB_TRACE_SUBTYPE(enum state_struct_type ss_type)
{
   return ss_type & 0xFFFF;
}

/** Subclass of Mesa vertex program */
struct brw_vertex_program {
   struct gl_vertex_program program;
   GLuint id;
};


/** Subclass of Mesa geometry program */
struct brw_geometry_program {
   struct gl_geometry_program program;
   unsigned id;  /**< serial no. to identify geom progs, never re-used */
};


/** Subclass of Mesa fragment program */
struct brw_fragment_program {
   struct gl_fragment_program program;
   GLuint id;  /**< serial no. to identify frag progs, never re-used */
};


/** Subclass of Mesa compute program */
struct brw_compute_program {
   struct gl_compute_program program;
   unsigned id;  /**< serial no. to identify compute progs, never re-used */
};


struct brw_shader {
   struct gl_shader base;

   bool compiled_once;
};

/* Note: If adding fields that need anything besides a normal memcmp() for
 * comparing them, be sure to go fix brw_stage_prog_data_compare().
 */
struct brw_stage_prog_data {
   struct {
      /** size of our binding table. */
      uint32_t size_bytes;

      /** @{
       * surface indices for the various groups of surfaces
       */
      uint32_t pull_constants_start;
      uint32_t texture_start;
      uint32_t gather_texture_start;
      uint32_t ubo_start;
      uint32_t abo_start;
      uint32_t shader_time_start;
      /** @} */
   } binding_table;

   GLuint nr_params;       /**< number of float params/constants */
   GLuint nr_pull_params;

   /* Pointers to tracked values (only valid once
    * _mesa_load_state_parameters has been called at runtime).
    *
    * These must be the last fields of the struct (see
    * brw_stage_prog_data_compare()).
    */
   const float **param;
   const float **pull_param;
};

/* Data about a particular attempt to compile a program.  Note that
 * there can be many of these, each in a different GL state
 * corresponding to a different brw_wm_prog_key struct, with different
 * compiled programs.
 *
 * Note: brw_wm_prog_data_compare() must be updated when adding fields to this
 * struct!
 */
struct brw_wm_prog_data {
   struct brw_stage_prog_data base;

   GLuint curb_read_length;
   GLuint num_varying_inputs;

   GLuint first_curbe_grf;
   GLuint first_curbe_grf_16;
   GLuint reg_blocks;
   GLuint reg_blocks_16;
   GLuint total_scratch;

   struct {
      /** @{
       * surface indices the WM-specific surfaces
       */
      uint32_t render_target_start;
      /** @} */
   } binding_table;

   bool dual_src_blend;
   bool uses_pos_offset;
   bool uses_omask;
   uint32_t prog_offset_16;

   /**
    * Mask of which interpolation modes are required by the fragment shader.
    * Used in hardware setup on gen6+.
    */
   uint32_t barycentric_interp_modes;

   /**
    * Map from gl_varying_slot to the position within the FS setup data
    * payload where the varying's attribute vertex deltas should be delivered.
    * For varying slots that are not used by the FS, the value is -1.
    */
   int urb_setup[VARYING_SLOT_MAX];
};

/**
 * Enum representing the i965-specific vertex results that don't correspond
 * exactly to any element of gl_varying_slot.  The values of this enum are
 * assigned such that they don't conflict with gl_varying_slot.
 */
typedef enum
{
   BRW_VARYING_SLOT_NDC = VARYING_SLOT_MAX,
   BRW_VARYING_SLOT_PAD,
   /**
    * Technically this is not a varying but just a placeholder that
    * compile_sf_prog() inserts into its VUE map to cause the gl_PointCoord
    * builtin variable to be compiled correctly. see compile_sf_prog() for
    * more info.
    */
   BRW_VARYING_SLOT_PNTC,
   BRW_VARYING_SLOT_COUNT
} brw_varying_slot;


/**
 * Data structure recording the relationship between the gl_varying_slot enum
 * and "slots" within the vertex URB entry (VUE).  A "slot" is defined as a
 * single octaword within the VUE (128 bits).
 *
 * Note that each BRW register contains 256 bits (2 octawords), so when
 * accessing the VUE in URB_NOSWIZZLE mode, each register corresponds to two
 * consecutive VUE slots.  When accessing the VUE in URB_INTERLEAVED mode (as
 * in a vertex shader), each register corresponds to a single VUE slot, since
 * it contains data for two separate vertices.
 */
struct brw_vue_map {
   /**
    * Bitfield representing all varying slots that are (a) stored in this VUE
    * map, and (b) actually written by the shader.  Does not include any of
    * the additional varying slots defined in brw_varying_slot.
    */
   GLbitfield64 slots_valid;

   /**
    * Map from gl_varying_slot value to VUE slot.  For gl_varying_slots that are
    * not stored in a slot (because they are not written, or because
    * additional processing is applied before storing them in the VUE), the
    * value is -1.
    */
   signed char varying_to_slot[BRW_VARYING_SLOT_COUNT];

   /**
    * Map from VUE slot to gl_varying_slot value.  For slots that do not
    * directly correspond to a gl_varying_slot, the value comes from
    * brw_varying_slot.
    *
    * For slots that are not in use, the value is BRW_VARYING_SLOT_COUNT (this
    * simplifies code that uses the value stored in slot_to_varying to
    * create a bit mask).
    */
   signed char slot_to_varying[BRW_VARYING_SLOT_COUNT];

   /**
    * Total number of VUE slots in use
    */
   int num_slots;
};

/**
 * Convert a VUE slot number into a byte offset within the VUE.
 */
static inline GLuint brw_vue_slot_to_offset(GLuint slot)
{
   return 16*slot;
}

/**
 * Convert a vertex output (brw_varying_slot) into a byte offset within the
 * VUE.
 */
static inline GLuint brw_varying_to_offset(struct brw_vue_map *vue_map,
                                           GLuint varying)
{
   return brw_vue_slot_to_offset(vue_map->varying_to_slot[varying]);
}

void brw_compute_vue_map(struct brw_context *brw, struct brw_vue_map *vue_map,
                         GLbitfield64 slots_valid);


/**
 * Bitmask indicating which fragment shader inputs represent varyings (and
 * hence have to be delivered to the fragment shader by the SF/SBE stage).
 */
#define BRW_FS_VARYING_INPUT_MASK \
   (BITFIELD64_RANGE(0, VARYING_SLOT_MAX) & \
    ~VARYING_BIT_POS & ~VARYING_BIT_FACE)


/*
 * Mapping of VUE map slots to interpolation modes.
 */
struct interpolation_mode_map {
   unsigned char mode[BRW_VARYING_SLOT_COUNT];
};

static inline bool brw_any_flat_varyings(struct interpolation_mode_map *map)
{
   for (int i = 0; i < BRW_VARYING_SLOT_COUNT; i++)
      if (map->mode[i] == INTERP_QUALIFIER_FLAT)
         return true;

   return false;
}

static inline bool brw_any_noperspective_varyings(struct interpolation_mode_map *map)
{
   for (int i = 0; i < BRW_VARYING_SLOT_COUNT; i++)
      if (map->mode[i] == INTERP_QUALIFIER_NOPERSPECTIVE)
         return true;

   return false;
}


struct brw_sf_prog_data {
   GLuint urb_read_length;
   GLuint total_grf;

   /* Each vertex may have upto 12 attributes, 4 components each,
    * except WPOS which requires only 2.  (11*4 + 2) == 44 ==> 11
    * rows.
    *
    * Actually we use 4 for each, so call it 12 rows.
    */
   GLuint urb_entry_size;
};


/**
 * We always program SF to start reading at an offset of 1 (2 varying slots)
 * from the start of the vertex URB entry.  This causes it to skip:
 * - VARYING_SLOT_PSIZ and BRW_VARYING_SLOT_NDC on gen4-5
 * - VARYING_SLOT_PSIZ and VARYING_SLOT_POS on gen6+
 */
#define BRW_SF_URB_ENTRY_READ_OFFSET 1


struct brw_clip_prog_data {
   GLuint curb_read_length;	/* user planes? */
   GLuint clip_mode;
   GLuint urb_read_length;
   GLuint total_grf;
};

struct brw_ff_gs_prog_data {
   GLuint urb_read_length;
   GLuint total_grf;

   /**
    * Gen6 transform feedback: Amount by which the streaming vertex buffer
    * indices should be incremented each time the GS is invoked.
    */
   unsigned svbi_postincrement_value;
};


/* Note: brw_vec4_prog_data_compare() must be updated when adding fields to
 * this struct!
 */
struct brw_vec4_prog_data {
   struct brw_stage_prog_data base;
   struct brw_vue_map vue_map;

   /**
    * Register where the thread expects to find input data from the URB
    * (typically uniforms, followed by per-vertex inputs).
    */
   unsigned dispatch_grf_start_reg;

   GLuint curb_read_length;
   GLuint urb_read_length;
   GLuint total_grf;
   GLuint total_scratch;

   /* Used for calculating urb partitions.  In the VS, this is the size of the
    * URB entry used for both input and output to the thread.  In the GS, this
    * is the size of the URB entry used for output.
    */
   GLuint urb_entry_size;
};


/* Note: brw_vs_prog_data_compare() must be updated when adding fields to this
 * struct!
 */
struct brw_vs_prog_data {
   struct brw_vec4_prog_data base;

   GLbitfield64 inputs_read;

   bool uses_vertexid;
   bool uses_instanceid;
};


/* Note: brw_gs_prog_data_compare() must be updated when adding fields to
 * this struct!
 */
struct brw_gs_prog_data
{
   struct brw_vec4_prog_data base;

   /**
    * Size of an output vertex, measured in HWORDS (32 bytes).
    */
   unsigned output_vertex_size_hwords;

   unsigned output_topology;

   /**
    * Size of the control data (cut bits or StreamID bits), in hwords (32
    * bytes).  0 if there is no control data.
    */
   unsigned control_data_header_size_hwords;

   /**
    * Format of the control data (either GEN7_GS_CONTROL_DATA_FORMAT_GSCTL_SID
    * if the control data is StreamID bits, or
    * GEN7_GS_CONTROL_DATA_FORMAT_GSCTL_CUT if the control data is cut bits).
    * Ignored if control_data_header_size is 0.
    */
   unsigned control_data_format;

   bool include_primitive_id;

   int invocations;

   /**
    * True if the thread should be dispatched in DUAL_INSTANCE mode, false if
    * it should be dispatched in DUAL_OBJECT mode.
    */
   bool dual_instanced_dispatch;
};

/** Number of texture sampler units */
#define BRW_MAX_TEX_UNIT 32

/** Max number of render targets in a shader */
#define BRW_MAX_DRAW_BUFFERS 8

/** Max number of atomic counter buffer objects in a shader */
#define BRW_MAX_ABO 16

/**
 * Max number of binding table entries used for stream output.
 *
 * From the OpenGL 3.0 spec, table 6.44 (Transform Feedback State), the
 * minimum value of MAX_TRANSFORM_FEEDBACK_INTERLEAVED_COMPONENTS is 64.
 *
 * On Gen6, the size of transform feedback data is limited not by the number
 * of components but by the number of binding table entries we set aside.  We
 * use one binding table entry for a float, one entry for a vector, and one
 * entry per matrix column.  Since the only way we can communicate our
 * transform feedback capabilities to the client is via
 * MAX_TRANSFORM_FEEDBACK_INTERLEAVED_COMPONENTS, we need to plan for the
 * worst case, in which all the varyings are floats, so we use up one binding
 * table entry per component.  Therefore we need to set aside at least 64
 * binding table entries for use by transform feedback.
 *
 * Note: since we don't currently pack varyings, it is currently impossible
 * for the client to actually use up all of these binding table entries--if
 * all of their varyings were floats, they would run out of varying slots and
 * fail to link.  But that's a bug, so it seems prudent to go ahead and
 * allocate the number of binding table entries we will need once the bug is
 * fixed.
 */
#define BRW_MAX_SOL_BINDINGS 64

/** Maximum number of actual buffers used for stream output */
#define BRW_MAX_SOL_BUFFERS 4

#define BRW_MAX_SURFACES   (BRW_MAX_DRAW_BUFFERS +                      \
                            BRW_MAX_TEX_UNIT * 2 + /* normal, gather */ \
                            12 + /* ubo */                              \
                            BRW_MAX_ABO +                               \
                            2 /* shader time, pull constants */)

#define SURF_INDEX_GEN6_SOL_BINDING(t) (t)
#define BRW_MAX_GEN6_GS_SURFACES       SURF_INDEX_GEN6_SOL_BINDING(BRW_MAX_SOL_BINDINGS)

/**
 * Stride in bytes between shader_time entries.
 *
 * We separate entries by a cacheline to reduce traffic between EUs writing to
 * different entries.
 */
#define SHADER_TIME_STRIDE 64

enum brw_cache_id {
   BRW_CC_VP,
   BRW_CC_UNIT,
   BRW_WM_PROG,
   BRW_BLORP_BLIT_PROG,
   BRW_BLORP_CONST_COLOR_PROG,
   BRW_SAMPLER,
   BRW_WM_UNIT,
   BRW_SF_PROG,
   BRW_SF_VP,
   BRW_SF_UNIT, /* scissor state on gen6 */
   BRW_VS_UNIT,
   BRW_VS_PROG,
   BRW_FF_GS_UNIT,
   BRW_FF_GS_PROG,
   BRW_GS_PROG,
   BRW_CLIP_VP,
   BRW_CLIP_UNIT,
   BRW_CLIP_PROG,

   BRW_MAX_CACHE
};

struct brw_cache_item {
   /**
    * Effectively part of the key, cache_id identifies what kind of state
    * buffer is involved, and also which brw->state.dirty.cache flag should
    * be set when this cache item is chosen.
    */
   enum brw_cache_id cache_id;
   /** 32-bit hash of the key data */
   GLuint hash;
   GLuint key_size;		/* for variable-sized keys */
   GLuint aux_size;
   const void *key;

   uint32_t offset;
   uint32_t size;

   struct brw_cache_item *next;
};


typedef bool (*cache_aux_compare_func)(const void *a, const void *b);
typedef void (*cache_aux_free_func)(const void *aux);

struct brw_cache {
   struct brw_context *brw;

   struct brw_cache_item **items;
   drm_intel_bo *bo;
   GLuint size, n_items;

   uint32_t next_offset;
   bool bo_used_by_gpu;

   /**
    * Optional functions used in determining whether the prog_data for a new
    * cache item matches an existing cache item (in case there's relevant data
    * outside of the prog_data).  If NULL, a plain memcmp is done.
    */
   cache_aux_compare_func aux_compare[BRW_MAX_CACHE];
   /** Optional functions for freeing other pointers attached to a prog_data. */
   cache_aux_free_func aux_free[BRW_MAX_CACHE];
};


/* Considered adding a member to this struct to document which flags
 * an update might raise so that ordering of the state atoms can be
 * checked or derived at runtime.  Dropped the idea in favor of having
 * a debug mode where the state is monitored for flags which are
 * raised that have already been tested against.
 */
struct brw_tracked_state {
   struct brw_state_flags dirty;
   void (*emit)( struct brw_context *brw );
};

enum shader_time_shader_type {
   ST_NONE,
   ST_VS,
   ST_VS_WRITTEN,
   ST_VS_RESET,
   ST_GS,
   ST_GS_WRITTEN,
   ST_GS_RESET,
   ST_FS8,
   ST_FS8_WRITTEN,
   ST_FS8_RESET,
   ST_FS16,
   ST_FS16_WRITTEN,
   ST_FS16_RESET,
};

/* Flags for brw->state.cache.
 */
#define CACHE_NEW_CC_VP                  (1<<BRW_CC_VP)
#define CACHE_NEW_CC_UNIT                (1<<BRW_CC_UNIT)
#define CACHE_NEW_WM_PROG                (1<<BRW_WM_PROG)
#define CACHE_NEW_BLORP_BLIT_PROG        (1<<BRW_BLORP_BLIT_PROG)
#define CACHE_NEW_BLORP_CONST_COLOR_PROG (1<<BRW_BLORP_CONST_COLOR_PROG)
#define CACHE_NEW_SAMPLER                (1<<BRW_SAMPLER)
#define CACHE_NEW_WM_UNIT                (1<<BRW_WM_UNIT)
#define CACHE_NEW_SF_PROG                (1<<BRW_SF_PROG)
#define CACHE_NEW_SF_VP                  (1<<BRW_SF_VP)
#define CACHE_NEW_SF_UNIT                (1<<BRW_SF_UNIT)
#define CACHE_NEW_VS_UNIT                (1<<BRW_VS_UNIT)
#define CACHE_NEW_VS_PROG                (1<<BRW_VS_PROG)
#define CACHE_NEW_FF_GS_UNIT             (1<<BRW_FF_GS_UNIT)
#define CACHE_NEW_FF_GS_PROG             (1<<BRW_FF_GS_PROG)
#define CACHE_NEW_GS_PROG                (1<<BRW_GS_PROG)
#define CACHE_NEW_CLIP_VP                (1<<BRW_CLIP_VP)
#define CACHE_NEW_CLIP_UNIT              (1<<BRW_CLIP_UNIT)
#define CACHE_NEW_CLIP_PROG              (1<<BRW_CLIP_PROG)

struct brw_vertex_buffer {
   /** Buffer object containing the uploaded vertex data */
   drm_intel_bo *bo;
   uint32_t offset;
   /** Byte stride between elements in the uploaded array */
   GLuint stride;
   GLuint step_rate;
};
struct brw_vertex_element {
   const struct gl_client_array *glarray;

   int buffer;

   /** Offset of the first element within the buffer object */
   unsigned int offset;
};






/**
 * Data shared between each programmable stage in the pipeline (vs, gs, and
 * wm).
 */
struct brw_stage_state
{
   gl_shader_stage stage;
   struct brw_stage_prog_data *prog_data;

   /**
    * Optional scratch buffer used to store spilled register values and
    * variably-indexed GRF arrays.
    */
   drm_intel_bo *scratch_bo;

   /** Offset in the program cache to the program */
   uint32_t prog_offset;

   /** Offset in the batchbuffer to Gen4-5 pipelined state (VS/WM/GS_STATE). */
   uint32_t state_offset;

   uint32_t push_const_offset; /* Offset in the batchbuffer */
   int push_const_size; /* in 256-bit register increments */

   /* Binding table: pointers to SURFACE_STATE entries. */
   uint32_t bind_bo_offset;
   uint32_t surf_offset[BRW_MAX_SURFACES];

   /** SAMPLER_STATE count and table offset */
   uint32_t sampler_count;
   uint32_t sampler_offset;

   /** Offsets in the batch to sampler default colors (texture border color) */
   uint32_t sdc_offset[BRW_MAX_TEX_UNIT];
};


/**
 * brw_context is derived from gl_context.
 */
struct brw_context
{
    // LunarG:  Note, this is our stripped down gl_context
   struct gl_context ctx; /**< base class, must be first field */

   GLuint stats_wm;

   bool disable_derivative_optimization;

   GLuint primitive; /**< Hardware primitive, such as _3DPRIM_TRILIST. */

   GLenum reduced_primitive;

   /**
    * Set if we're either a debug context or the INTEL_DEBUG=perf environment
    * variable is set, this is the flag indicating to do expensive work that
    * might lead to a perf_debug() call.
    */
   bool perf_debug;

   int gen;
   int gt;

   bool is_g4x;
   bool is_baytrail;
   bool is_haswell;

   bool has_llc;
   bool has_compr4;
   bool has_negative_rhw_bug;
   bool has_pln;

   /**
    * Some versions of Gen hardware don't do centroid interpolation correctly
    * on unlit pixels, causing incorrect values for derivatives near triangle
    * edges.  Enabling this flag causes the fragment shader to use
    * non-centroid interpolation for unlit pixels, at the expense of two extra
    * fragment shader instructions.
    */
   bool needs_unlit_centroid_workaround;


   struct {
      struct brw_vertex_element inputs[VERT_ATTRIB_MAX];
      struct brw_vertex_buffer buffers[VERT_ATTRIB_MAX];

      struct brw_vertex_element *enabled[VERT_ATTRIB_MAX];
      GLuint nr_enabled;
      GLuint nr_buffers;

      /* Summary of size and varying of active arrays, so we can check
       * for changes to this state:
       */
      unsigned int min_index, max_index;

      /* Offset from start of vertex buffer so we can avoid redefining
       * the same VB packed over and over again.
       */
      unsigned int start_vertex_bias;
   } vb;

   struct {
      /**
       * Index buffer for this draw_prims call.
       *
       * Updates are signaled by BRW_NEW_INDICES.
       */
      const struct _mesa_index_buffer *ib;

      /* Updates are signaled by BRW_NEW_INDEX_BUFFER. */
      drm_intel_bo *bo;
      GLuint type;

      /* Offset to index buffer index to use in CMD_3D_PRIM so that we can
       * avoid re-uploading the IB packet over and over if we're actually
       * referencing the same index buffer.
       */
      unsigned int start_vertex_offset;
   } ib;

   /* Active vertex program:
    */
   const struct gl_vertex_program *vertex_program;
   const struct gl_geometry_program *geometry_program;
   const struct gl_fragment_program *fragment_program;

   /**
    * Platform specific constants containing the maximum number of threads
    * for each pipeline stage.
    */
   int max_vs_threads;
   int max_gs_threads;
   int max_wm_threads;

   /* BRW_NEW_URB_ALLOCATIONS:
    */
   struct {
      GLuint vsize;		/* vertex size plus header in urb registers */
      GLuint csize;		/* constant buffer size in urb registers */
      GLuint sfsize;		/* setup data size in urb registers */

      bool constrained;

      GLuint min_vs_entries;    /* Minimum number of VS entries */
      GLuint max_vs_entries;	/* Maximum number of VS entries */
      GLuint max_gs_entries;	/* Maximum number of GS entries */

      GLuint nr_vs_entries;
      GLuint nr_gs_entries;
      GLuint nr_clip_entries;
      GLuint nr_sf_entries;
      GLuint nr_cs_entries;

      GLuint vs_start;
      GLuint gs_start;
      GLuint clip_start;
      GLuint sf_start;
      GLuint cs_start;
      GLuint size; /* Hardware URB size, in KB. */

      /* gen6: True if the most recently sent _3DSTATE_URB message allocated
       * URB space for the GS.
       */
      bool gen6_gs_previously_active;
   } urb;


   /* BRW_NEW_CURBE_OFFSETS:
    */
   struct {
      GLuint wm_start;  /**< pos of first wm const in CURBE buffer */
      GLuint wm_size;   /**< number of float[4] consts, multiple of 16 */
      GLuint clip_start;
      GLuint clip_size;
      GLuint vs_start;
      GLuint vs_size;
      GLuint total_size;

      drm_intel_bo *curbe_bo;
      /** Offset within curbe_bo of space for current curbe entry */
      GLuint curbe_offset;
      /** Offset within curbe_bo of space for next curbe entry */
      GLuint curbe_next_offset;

      /**
       * Copy of the last set of CURBEs uploaded.  Frequently we'll end up
       * in brw_curbe.c with the same set of constant data to be uploaded,
       * so we'd rather not upload new constants in that case (it can cause
       * a pipeline bubble since only up to 4 can be pipelined at a time).
       */
      GLfloat *last_buf;
      /**
       * Allocation for where to calculate the next set of CURBEs.
       * It's a hot enough path that malloc/free of that data matters.
       */
      GLfloat *next_buf;
      GLuint last_bufsz;
   } curbe;

   /**
    * Layout of vertex data exiting the vertex shader.
    *
    * BRW_NEW_VUE_MAP_VS is flagged when this VUE map changes.
    */
   struct brw_vue_map vue_map_vs;

   /**
    * Layout of vertex data exiting the geometry portion of the pipleine.
    * This comes from the geometry shader if one exists, otherwise from the
    * vertex shader.
    *
    * BRW_NEW_VUE_MAP_GEOM_OUT is flagged when the VUE map changes.
    */
   struct brw_vue_map vue_map_geom_out;

   struct {
      struct brw_stage_state base;
      struct brw_vs_prog_data *prog_data;
   } vs;

   struct {
      struct brw_stage_state base;
      struct brw_gs_prog_data *prog_data;

      /**
       * True if the 3DSTATE_GS command most recently emitted to the 3D
       * pipeline enabled the GS; false otherwise.
       */
      bool enabled;
   } gs;

   struct {
      struct brw_ff_gs_prog_data *prog_data;

      bool prog_active;
      /** Offset in the program cache to the CLIP program pre-gen6 */
      uint32_t prog_offset;
      uint32_t state_offset;

      uint32_t bind_bo_offset;
      uint32_t surf_offset[BRW_MAX_GEN6_GS_SURFACES];
   } ff_gs;

   struct {
      struct brw_clip_prog_data *prog_data;

      /** Offset in the program cache to the CLIP program pre-gen6 */
      uint32_t prog_offset;

      /* Offset in the batch to the CLIP state on pre-gen6. */
      uint32_t state_offset;

      /* As of gen6, this is the offset in the batch to the CLIP VP,
       * instead of vp_bo.
       */
      uint32_t vp_offset;
   } clip;


   struct {
      struct brw_sf_prog_data *prog_data;

      /** Offset in the program cache to the CLIP program pre-gen6 */
      uint32_t prog_offset;
      uint32_t state_offset;
      uint32_t vp_offset;
   } sf;

   struct {
      struct brw_stage_state base;
      struct brw_wm_prog_data *prog_data;

      GLuint render_surf;

      /**
       * Buffer object used in place of multisampled null render targets on
       * Gen6.  See brw_update_null_renderbuffer_surface().
       */
      drm_intel_bo *multisampled_null_render_target_bo;
   } wm;


   struct {
      uint32_t state_offset;
      uint32_t blend_state_offset;
      uint32_t depth_stencil_state_offset;
      uint32_t vp_offset;
   } cc;


   int num_atoms;
   const struct brw_tracked_state **atoms;

   struct {
      //drm_intel_bo *bo; // LunarG: Remove
      struct gl_shader_program **shader_programs;
      struct gl_program **programs;
      enum shader_time_shader_type *types;
      uint64_t *cumulative;
      int num_entries;
      int max_entries;
      double report_time;
   } shader_time;

   struct intel_screen *intelScreen;

   // LunarG : ADD
   // Give us a place to store our compile results
   struct gl_shader_program *shader_prog;
};





/*======================================================================
 * brw_program.c
 */
void brwInitFragProgFuncs( struct dd_function_table *functions );

int brw_get_scratch_size(int size);
void brw_get_scratch_bo(struct brw_context *brw,
			drm_intel_bo **scratch_bo, int size);
void brw_init_shader_time(struct brw_context *brw);
int brw_get_shader_time_index(struct brw_context *brw,
                              struct gl_shader_program *shader_prog,
                              struct gl_program *prog,
                              enum shader_time_shader_type type);
void brw_collect_and_report_shader_time(struct brw_context *brw);
void brw_destroy_shader_time(struct brw_context *brw);

/* brw_urb.c
 */
void brw_upload_urb_fence(struct brw_context *brw);

/* brw_curbe.c
 */
void brw_upload_cs_urb_state(struct brw_context *brw);

/* brw_fs_reg_allocate.cpp
 */
void brw_fs_alloc_reg_sets(struct intel_screen *screen);

/* brw_vec4_reg_allocate.cpp */
void brw_vec4_alloc_reg_set(struct intel_screen *screen);

/* brw_disasm.c */
int brw_disasm (FILE *file, struct brw_instruction *inst, int gen);

/* brw_vs.c */
gl_clip_plane *brw_select_clip_planes(struct gl_context *ctx);

/* intel_extensions.c */
extern void intelInitExtensions(struct gl_context *ctx);


/*======================================================================
 * Inline conversion functions.  These are better-typed than the
 * macros used previously:
 */
static inline struct brw_context *
brw_context( struct gl_context *ctx )
{
   return (struct brw_context *)ctx;
}

static inline struct brw_vertex_program *
brw_vertex_program(struct gl_vertex_program *p)
{
   return (struct brw_vertex_program *) p;
}

static inline const struct brw_vertex_program *
brw_vertex_program_const(const struct gl_vertex_program *p)
{
   return (const struct brw_vertex_program *) p;
}

static inline struct brw_geometry_program *
brw_geometry_program(struct gl_geometry_program *p)
{
   return (struct brw_geometry_program *) p;
}

static inline struct brw_fragment_program *
brw_fragment_program(struct gl_fragment_program *p)
{
   return (struct brw_fragment_program *) p;
}

static inline const struct brw_fragment_program *
brw_fragment_program_const(const struct gl_fragment_program *p)
{
   return (const struct brw_fragment_program *) p;
}

/**
 * Pre-gen6, the register file of the EUs was shared between threads,
 * and each thread used some subset allocated on a 16-register block
 * granularity.  The unit states wanted these block counts.
 */
static inline int
brw_register_blocks(int reg_count)
{
   return ALIGN(reg_count, 16) / 16 - 1;
}


bool brw_do_cubemap_normalize(struct exec_list *instructions);
bool brw_lower_texture_gradients(struct brw_context *brw,
                                 struct exec_list *instructions);
bool brw_do_lower_unnormalized_offset(struct exec_list *instructions);

struct opcode_desc {
    char    *name;
    int	    nsrc;
    int	    ndst;
};

extern const struct opcode_desc opcode_descs[128];
extern const char * const conditional_modifier[16];


/* ================================================================
 * From linux kernel i386 header files, copes with odd sizes better
 * than COPY_DWORDS would:
 * XXX Put this in src/mesa/main/imports.h ???
 */
#if defined(i386) || defined(__i386__)
static inline void * __memcpy(void * to, const void * from, size_t n)
{
   int d0, d1, d2;
   __asm__ __volatile__(
      "rep ; movsl\n\t"
      "testb $2,%b4\n\t"
      "je 1f\n\t"
      "movsw\n"
      "1:\ttestb $1,%b4\n\t"
      "je 2f\n\t"
      "movsb\n"
      "2:"
      : "=&c" (d0), "=&D" (d1), "=&S" (d2)
      :"0" (n/4), "q" (n),"1" ((long) to),"2" ((long) from)
      : "memory");
   return (to);
}
#else
#define __memcpy(a,b,c) memcpy(a,b,c)
#endif

#ifdef __cplusplus
}
#endif

#endif
