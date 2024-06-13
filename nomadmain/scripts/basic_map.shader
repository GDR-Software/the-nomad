"Desert Tileset"
{
    nomipmaps
    nopicmip
    if ( $r_textureDetail >= 2 && $r_genNormalMaps == 0 ) {
        texFilter nearest
        map textures/desert_tilesets/DesertTilemap16x16_n.jpg
        blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
        rgbGen vertex
        stage normalMap
    }
    if ( $r_textureDetail >= 2 ) {
        texFilter nearest
        map textures/desert_tilesets/DesertTilemap16x16_s.jpg
        blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
        rgbGen vertex
        stage specularMap
    }
    {
        texFilter nearest
        map textures/desert_tilesets/DesertTilemap16x16.jpg
        blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
        rgbGen vertex
        stage diffuseMap
    }
}
