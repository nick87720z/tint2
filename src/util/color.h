#ifndef COLOR_H
#define COLOR_H

typedef struct Color {
    double rgb[3];  // Values are in [0, 1], with 0 meaning no intensity.
    double alpha;   // Values are in [0, 1], with 0 meaning fully transparent, 1 meaning fully opaque.
} Color;

#endif // COLOR_H
