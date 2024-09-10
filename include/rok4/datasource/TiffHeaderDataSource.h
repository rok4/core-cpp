#pragma once

#include "rok4/datasource/DataSource.h"
#include "rok4/enums/Format.h"

class TiffHeaderDataSource : public DataSource {

private:

    DataSource* source_data;
    size_t tile_size;
    size_t data_size;
    uint8_t* data;
    Rok4Format::eFormat format;
    int channel;
    int width;
    int height;

public:

    /**
     * Constructeur.
     * @param source_data la source de l'image Tiff sans entete peut être nulle
     * @param format le format de la pyramide
     * @param channel nombre de canaux de l'image
     * @param width largeur de l'image
     * @param height hauteur de l'image
     * @param tile_size taille de la tuile à définir si source_data est nulle
     */
    TiffHeaderDataSource ( DataSource* source_data, Rok4Format::eFormat format,
                           int channel, int width, int height, size_t tile_size=0 );

    inline bool release_data() {
        return source_data->release_data();
    }
    inline std::string get_type() {
        return source_data->get_type();
    }
    inline int get_http_status() {
        return source_data->get_http_status();
    }
    inline std::string get_encoding() {
        return source_data->get_encoding();
    }
    inline unsigned int get_length() {
        return data_size;
    }
    virtual const uint8_t* get_data ( size_t& size );
    virtual ~TiffHeaderDataSource();
};


