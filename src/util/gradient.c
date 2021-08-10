#include "gradient.h"

#include <glib.h>
#include <stdlib.h>
#include <string.h>

#include "common.h"

GradientType gradient_type_from_string(const char *str)
{
    return  g_str_equal(str, "horizontal") ? GRADIENT_HORIZONTAL
        :   g_str_equal(str, "vertical"  ) ? GRADIENT_VERTICAL
        :   g_str_equal(str, "radial"    ) ? GRADIENT_CENTERED
        :(fprintf(stderr, RED "tint2: Invalid gradient type: %s" RESET "\n", str),
          GRADIENT_VERTICAL);
}

void init_gradient(GradientClass *g, GradientType type)
{
    #define add_gradient_offset(offset,...) do {      \
        offset = (Offset *)calloc(1, sizeof(Offset)); \
        *offset = (Offset){ __VA_ARGS__ };            \
    } while(0)

    memset(g, 0, sizeof(*g));
    g->type = type;
    switch (type) {
    case GRADIENT_VERTICAL:
        add_gradient_offset(g->from.offsets_y, .variable = SIZE_CONST, .constant_value = 0);
        add_gradient_offset(g->to.offsets_y, .variable = SIZE_HEIGHT, .multiplier = 1.0);
        break;
    case GRADIENT_HORIZONTAL:
        add_gradient_offset(g->from.offsets_x, .variable = SIZE_CONST, .constant_value = 0);
        add_gradient_offset(g->to.offsets_x, .variable = SIZE_WIDTH, .multiplier = 1.0);
        break;
    case GRADIENT_CENTERED:
        // from
        add_gradient_offset(g->from.offsets_x, .variable = SIZE_CENTERX, .multiplier = 1.0);
        add_gradient_offset(g->from.offsets_y, .variable = SIZE_CENTERY, .multiplier = 1.0);
        add_gradient_offset(g->from.offsets_r, .variable = SIZE_CONST, .constant_value = 0);
        // to
        add_gradient_offset(g->to.offsets_x, .variable = SIZE_CENTERX, .multiplier = 1.0);
        add_gradient_offset(g->to.offsets_y, .variable = SIZE_CENTERY, .multiplier = 1.0);
        add_gradient_offset(g->to.offsets_r, .variable = SIZE_RADIUS,  .multiplier = 1.0);
        break;
    }

    #undef add_gradient_offset
}

void cleanup_gradient(GradientClass *g)
{
    g_list_free_full(g->extra_color_stops, free);
    switch (g->type) {
    case GRADIENT_VERTICAL:
        free(g->from.offsets_y);
        free(g->to.offsets_y);
        break;
    case GRADIENT_HORIZONTAL:
        free(g->from.offsets_x);
        free(g->to.offsets_x);
        break;
    case GRADIENT_CENTERED:
        free(g->from.offsets_x);
        free(g->from.offsets_y);
        free(g->from.offsets_r);
        free(g->to.offsets_x);
        free(g->to.offsets_y);
        free(g->to.offsets_r);
        break;
    }
    memset(g, 0, sizeof(*g));
}
