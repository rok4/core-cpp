#ifndef TIFFHEADERDATASOURCE_H
#define TIFFHEADERDATASOURCE_H

#include "rok4/datasource/DataSource.h"
#include "rok4/enums/Format.h"

class TiffHeaderDataSource : public DataSource {
private:
    DataSource* dataSource;
    size_t tileSize;
    size_t dataSize;
    uint8_t* data;
    Rok4Format::eFormat format;
    int channel;
    int width;
    int height;
public:
    /**
     * Constructeur.
     * @param dataSource la source de l'image Tiff sans entete peut être nulle
     * @param format le format de la pyramide
     * @param channel nombre de canaux de l'image
     * @param width largeur de l'image
     * @param height hauteur de l'image
     * @param tileSize taille de la tuile à définir si dataSource est nulle
     */
    TiffHeaderDataSource ( DataSource* dataSource, Rok4Format::eFormat format,
                           int channel, int width, int height, size_t tileSize=0 );

    inline bool releaseData() {
        return dataSource->releaseData();
    }
    inline std::string getType() {
        return dataSource->getType();
    }
    inline int getHttpStatus() {
        return dataSource->getHttpStatus();
    }
    inline std::string getEncoding() {
        return dataSource->getEncoding();
    }
    inline unsigned int getLength() {
        return dataSize;
    }
    virtual const uint8_t* getData ( size_t& size );
    virtual ~TiffHeaderDataSource();
};

#endif // TIFFHEADERDATASOURCE_H
