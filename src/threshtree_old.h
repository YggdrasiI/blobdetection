/* Let compiler optimize code for fixed stepwidth and stepheight */
FORCEINLINE
Tree* _find_connection_components_coarse(
    const uint8_t *data,
    const uint32_t w, const uint32_t h,
    const BlobtreeRect roi,
    const uint8_t thresh,
    const uint32_t stepwidth,
    const uint32_t stepheight,
    Blob **tree_data,
    ThreshtreeWorkspace *workspace );
