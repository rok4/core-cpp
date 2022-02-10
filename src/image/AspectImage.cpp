/*
 * Copyright © (2011) Institut national de l'information
 *                    géographique et forestière
 *
 * Géoportail SAV <contact.geoservices@ign.fr>
 *
 * This software is a computer program whose purpose is to publish geographic
 * data using OGC WMS and WMTS protocol.
 *
 * This software is governed by the CeCILL-C license under French law and
 * abiding by the rules of distribution of free software.  You can  use,
 * modify and/ or redistribute the software under the terms of the CeCILL-C
 * license as circulated by CEA, CNRS and INRIA at the following URL
 * "http://www.cecill.info".
 *
 * As a counterpart to the access to the source code and  rights to copy,
 * modify and redistribute granted by the license, users are provided only
 * with a limited warranty  and the software's author,  the holder of the
 * economic rights,  and the successive licensors  have only  limited
 * liability.
 *
 * In this respect, the user's attention is drawn to the risks associated
 * with loading,  using,  modifying and/or developing or reproducing the
 * software by the user in light of its specific status of free software,
 * that may mean  that it is complicated to manipulate,  and  that  also
 * therefore means  that it is reserved for developers  and  experienced
 * professionals having in-depth computer knowledge. Users are therefore
 * encouraged to load and test the software's suitability as regards their
 * requirements in conditions enabling the security of their systems and/or
 * data to be ensured and,  more generally, to use and operate it in the
 * same conditions as regards security.
 *
 * The fact that you are presently reading this means that you have had
 *
 * knowledge of the CeCILL-C license and that you accept its terms.
 */

#include "image/AspectImage.h"

#include <boost/log/trivial.hpp>

#include "utils/Utils.h"
#include <cstring>
#include <cmath>
#define DEG_TO_RAD      .0174532925199432958
#include <string>

int AspectImage::getline ( float* buffer, int line ) {
    return _getline ( buffer, line );
}

int AspectImage::getline ( uint16_t* buffer, int line ) {
    return _getline ( buffer, line );
}

int AspectImage::getline ( uint8_t* buffer, int line ) {
    return _getline ( buffer, line );
}

//definition des variables
AspectImage::AspectImage (Image* image, Aspect* asp) :
    Image ( image->getWidth() - 2, image->getHeight() - 2, 1 ),
    origImage ( image ), resolution (image->computeMeanResolution()), algo (asp->getAlgo()), minSlope (asp->getMinSlope())
    {

    // On réduit la bbox d'un pixel de chaque côté
    BoundingBox<double> bb = origImage->getBbox();
    bb.xmin += origImage->getResX();
    bb.ymin += origImage->getResY();
    bb.xmax -= origImage->getResX();
    bb.ymax -= origImage->getResY();
    setBbox(bb);

    setCRS(origImage->getCRS());

    // Buffer de lignes sources
    memorizedOrigLines = 3;

    origLines = new int[memorizedOrigLines];
    for (int i = 0; i < memorizedOrigLines; i++) {
        origLines[i] = -1;
    }
    origLinesBuffer = new float[origImage->getWidth() * memorizedOrigLines];

    matrix[0] = 1 / (8.0*resolution) ;
    matrix[1] = 2 / (8.0*resolution) ;
    matrix[2] = 1 / (8.0*resolution) ;

    matrix[3] = 2 / (8.0*resolution) ;
    matrix[4] = 0 ;
    matrix[5] = 2 / (8.0*resolution) ;

    matrix[6] = 1 / (8.0*resolution) ;
    matrix[7] = 2 / (8.0*resolution) ;
    matrix[8] = 1 / (8.0*resolution) ;
}

AspectImage::~AspectImage() {
    delete origImage;
    delete[] origLines;
    delete[] origLinesBuffer;
}


template<typename T>
int AspectImage::_getline ( T* buffer, int line ) {
    // L'image source fait une ligne de plus en haut et en bas (ligne 0 de l'image estompée = ligne 1 de l'image source)
    // et une colonne de plus à gauche et à droite
    // Pour obtenir la ligne 0 de l'image aspect, on a besoin des lignes 0, 1 et 2 de l'image source
    // Plus généralement, pour avoir la ligne n de l'image aspect, on a besoin des lignes
    // n, n+1 et n+2 de l'image source

    // On range les lignes sources dans un buffer qui peut en stocker 3
    // La ligne source n est stockée en (n % memorizedOrigLines) ème position

    // calcul des emplacements dans le buffer des 3 lignes sources nécessaires
    float* line1 = origLinesBuffer + (line % memorizedOrigLines) * origImage->getWidth();
    float* line2 = origLinesBuffer + ((line + 1) % memorizedOrigLines) * origImage->getWidth();
    float* line3 = origLinesBuffer + ((line + 2) % memorizedOrigLines) * origImage->getWidth();

    // ligne du dessus
    if (origLines[line % memorizedOrigLines] != line) {
        // la ligne source 'line' n'est pas celle stockée dans le buffer, on doit la lire
        origImage->getline (line1 , line);
        origLines[line % memorizedOrigLines] = line;
    }
    // ligne du milieu
    if (origLines[(line + 1) % memorizedOrigLines] != line + 1) {
        // la ligne source 'line + 1' n'est pas celle stockée dans le buffer, on doit la lire
        origImage->getline (line2 , line + 1);
        origLines[(line + 1) % memorizedOrigLines] = line + 1;
    }
    // ligne du dessous
    if (origLines[(line + 2) % memorizedOrigLines] != line + 2) {
        // la ligne source 'line + 2' n'est pas celle stockée dans le buffer, on doit la lire
        origImage->getline (line3 , line + 2);
        origLines[(line + 2) % memorizedOrigLines] = line + 2;
    }

    //on commence a la premiere colonne
    int columnOrig = 1;
    int column = 0;
    //creation de la variable sur laquelle on travaille pour trouver le seuil
    double value,value1,value2,slope;

    //calcul de la variable sur toutes les autres colonnes
    while ( column < width ) {

        value1 = (matrix[2] * ( * ( line1+columnOrig+1 ) ) + matrix[5] * ( * ( line2+columnOrig+1 ) ) + matrix[8] * ( * ( line3+columnOrig+1 ) ) - matrix[0] * ( * ( line1+columnOrig-1 ) ) - matrix[3] * ( * ( line2+columnOrig-1 ) ) - matrix[6] * ( * ( line3+columnOrig-1 ) ));
        value2 = (matrix[0] * ( * ( line1+columnOrig-1 ) ) + matrix[1] * ( * ( line1+columnOrig ) ) + matrix[2] * ( * ( line1+columnOrig+1 ) ) - matrix[6] * ( * ( line3+columnOrig-1 ) ) - matrix[7] * ( * ( line3+columnOrig ) ) - matrix[8] * ( * ( line3+columnOrig+1 ) ));

        //calcul de la pente pour ne pas afficher l'exposition en dessous d'une certaine valeur de pente
        slope = sqrt(pow(value1,2.0)+pow(value2,2.0));
        if (slope < minSlope) {
            value = -1.0;
        } else {
            value = (atan2(value1,value2) + M_PI) * 180 / M_PI;
        }

        * ( buffer + ( column++ ) ) = (T) ( value );
        columnOrig++;
    }

    return width * sizeof(T);

}

