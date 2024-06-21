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

#include "image/PenteImage.h"

#include <boost/log/trivial.hpp>

#include "utils/Utils.h"
#include <cstring>
#include <cmath>
#define DEG_TO_RAD      .0174532925199432958
#include <string>

int PenteImage::get_line ( float* buffer, int line ) {
    return _getline ( buffer, line );
}

int PenteImage::get_line ( uint16_t* buffer, int line ) {
    return _getline ( buffer, line );
}

int PenteImage::get_line ( uint8_t* buffer, int line ) {
    return _getline ( buffer, line );
}

//definition des variables
PenteImage::PenteImage (Image* image, Pente* p) :
    Image ( image->get_width() - 2, image->get_height() - 2, 1),
    source_image ( image ), algo (p->getAlgo()),unit (p->getUnit()), slopeNoData (p->getSlopeNoData()), imgNoData (p->getImgNoData()), maxSlope (p->getMaxSlope())
    {

    // On réduit la bbox d'un pixel de chaque côté
    BoundingBox<double> bb = source_image->get_bbox();
    bb.xmin += source_image->get_resx();
    bb.ymin += source_image->get_resy();
    bb.xmax -= source_image->get_resx();
    bb.ymax -= source_image->get_resy();
    set_bbox(bb);

    set_crs(source_image->get_crs());

    // On calcule une seule fois la résolution en mètre
    resxmeter = get_resx(true);
    resymeter = get_resy(true);

    // Buffer de lignes sources
    memorized_source_lines = 3;

    source_lines = new int[memorized_source_lines];
    for (int i = 0; i < memorized_source_lines; i++) {
        source_lines[i] = -1;
    }
    source_lines_buffer = new float[source_image->get_width() * memorized_source_lines];

}


PenteImage::~PenteImage() {
    delete source_image;
    delete[] source_lines;
    delete[] source_lines_buffer;
}


template<typename T>
int PenteImage::_getline ( T* buffer, int line ) {
    // L'image source fait une ligne de plus en haut et en bas (ligne 0 de l'image estompée = ligne 1 de l'image source)
    // et une colonne de plus à gauche et à droite
    // Pour obtenir la ligne 0 de l'image estompée, on a besoin des lignes 0, 1 et 2 de l'image source
    // Plus généralement, pour avoir la ligne n de l'image estompée, on a besoin des lignes
    // n, n+1 et n+2 de l'image source

    // On range les lignes sources dans un buffer qui peut en stocker "memorized_source_lines"
    // La ligne source n est stockée en (n % memorized_source_lines) ème position

    // calcul des emplacements dans le buffer des 3 lignes sources nécessaires
    float* line1 = source_lines_buffer + (line % memorized_source_lines) * source_image->get_width();
    float* line2 = source_lines_buffer + ((line + 1) % memorized_source_lines) * source_image->get_width();
    float* line3 = source_lines_buffer + ((line + 2) % memorized_source_lines) * source_image->get_width();

    // ligne du dessus
    if (source_lines[line % memorized_source_lines] != line) {
        // la ligne source 'line' n'est pas celle stockée dans le buffer, on doit la lire
        source_image->get_line (line1 , line);
        source_lines[line % memorized_source_lines] = line;
    }
    // ligne du milieu
    if (source_lines[(line + 1) % memorized_source_lines] != line + 1) {
        // la ligne source 'line + 1' n'est pas celle stockée dans le buffer, on doit la lire
        source_image->get_line (line2 , line + 1);
        source_lines[(line + 1) % memorized_source_lines] = line + 1;
    }
    // ligne du dessous
    if (source_lines[(line + 2) % memorized_source_lines] != line + 2) {
        // la ligne source 'line + 2' n'est pas celle stockée dans le buffer, on doit la lire
        source_image->get_line (line3 , line + 2);
        source_lines[(line + 2) % memorized_source_lines] = line + 2;
    }
    
	//on commence a la premiere colonne
    int columnOrig = 1;
    int column = 0;
	//creation de la variable sur laquelle on travaille pour trouver le seuil
    double dzdx,dzdy,rise,slope;
    float a,b,c,d,e,f,g,h,i;
    float resx,resy;

    if (algo == "H") {
        resx = 8.0 * resxmeter;
        resy = 8.0 * resymeter;
    } else if (algo == "Z") {
        resx = 2.0 * resxmeter;
        resy = 2.0 * resymeter;
    }

	//calcul de la variable sur toutes les autres colonnes
    while ( column < width  ) {

        a = ( * ( line1+columnOrig-1 ) );
        b = ( * ( line1+columnOrig ) );
        c = ( * ( line1+columnOrig+1 ) );
        d = ( * ( line2+columnOrig-1 ) );
        e = ( * ( line2+columnOrig ) );
        f = ( * ( line2+columnOrig+1 ) );
        g = ( * ( line3+columnOrig-1 ) );
        h = ( * ( line3+columnOrig ) );
        i = ( * ( line3+columnOrig+1 ) );

        if (a == imgNoData || b == imgNoData || c == imgNoData || d == imgNoData || e == imgNoData ||
                f == imgNoData || g == imgNoData || h == imgNoData || i == imgNoData) {
            slope = slopeNoData;
        } else {

            if (algo == "H") {
                dzdx = (( c + 2.0 * f + i) - (a + 2.0 *  d + g)) / resx;
                dzdy = (( g + 2.0 * h + i) - (a + 2.0 *  b + c)) / resy;
            } else if (algo == "Z" ) {
                dzdx = (f - d) / resx;
                dzdy = (h - b) / resy;
            } else {

            }


            if (unit == "pourcent") {
                slope = sqrt(pow(dzdx,2.0) + pow(dzdy,2.0)) * 100.0;
            } else if (unit == "degree") {
                rise = sqrt(pow(dzdx,2.0) + pow(dzdy,2.0));

                slope = atan(rise) * 180.0 / M_PI;
                if (slope>90.0){slope = 180.0-slope;}
            } else {
                slope = 0;
            }

            if (slope>maxSlope){slope = maxSlope;}

        }

        * ( buffer + ( column++ ) ) = ( T ) ( slope );
        columnOrig++;

    }

    return width * sizeof(T);
}