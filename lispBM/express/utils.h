// From utils.h of the Express, for its color extensions
static inline void utils_truncate_number(float *number, float min, float max) {
    if (*number > max) {
        *number = max;
    } else if (*number < min) {
        *number = min;
    }
}
