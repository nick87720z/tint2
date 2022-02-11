#ifndef GRADIENT_H
#define GRADIENT_H

#include <glib.h>
#include <cairo.h>

#include "color.h"

//////////////////////////////////////////////////////////////////////
// Gradient types read from config options, not associated to any area

typedef enum GradientType { GRADIENT_VERTICAL, GRADIENT_HORIZONTAL, GRADIENT_CENTERED } GradientType;

typedef struct ColorStop {
    Color color;
    double offset;  // offset in 0-1
} ColorStop;

typedef enum SizeVariable {
    SIZE_CONST, // special case, could be checked as simple as if( !variable )
    SIZE_WIDTH,
    SIZE_HEIGHT,
    SIZE_RADIUS,
    SIZE_LEFT,
    SIZE_RIGHT,
    SIZE_TOP,
    SIZE_BOTTOM,
    SIZE_CENTERX,
    SIZE_CENTERY
} SizeVariable;

typedef struct Offset {
    SizeVariable variable;
    union {
        double constant_value;  // variable == SIZE_CONST
        double multiplier;      // variable != SIZE_CONST
    };
} Offset;

#define CONST_OFFSET(offset) (!(offset)->variable)

typedef struct ControlPoint {
    Offset *offsets_x;
    Offset *offsets_y;
    // Each element is an Offset

    Offset *offsets_r;
    // Defined only for radial gradients
} ControlPoint;

typedef struct GradientClass {
    GradientType type;
    Color start_color;
    Color end_color;
    GList *extra_color_stops;   // Each element is a ColorStop
    ControlPoint from;
    ControlPoint to;
} GradientClass;

GradientType gradient_type_from_string(const char *str);
void init_gradient(GradientClass *g, GradientType type);
void cleanup_gradient(GradientClass *g);

/////////////////////////////////////////
// Gradient instances associated to Areas

struct Area;

typedef struct GradientInstance {
    GradientClass *gradient_class;
    struct Area *area;
    cairo_pattern_t *pattern;
} GradientInstance;

extern gboolean debug_gradients;

#endif // GRADIENT_H
