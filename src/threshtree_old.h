/* Let compiler optimize code for fixed stepwidth and stepheight */
FORCEINLINE
Tree* _find_connection_components_coarse(
    const unsigned char *data,
    const unsigned int w, const unsigned int h,
    const BlobtreeRect roi,
    const unsigned char thresh,
    const unsigned int stepwidth,
    const unsigned int stepheight,
    Blob **tree_data,
    ThreshtreeWorkspace *workspace );
