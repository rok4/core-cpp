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

#include "image/EstompageImage.h"

#include <boost/log/trivial.hpp>

#include "utils/Utils.h"
#include <cstring>
#include <cmath>


int EstompageImage::get_line ( float* buffer, int line ) {
    return _getline ( buffer, line );
}

int EstompageImage::get_line ( uint16_t* buffer, int line ) {
    return _getline ( buffer, line );
}

int EstompageImage::get_line ( uint8_t* buffer, int line ) {
    return _getline ( buffer, line );
}

EstompageImage::EstompageImage (Image *image, Estompage* est) :
    Image ( image->get_width() - 2, image->get_height() - 2, 1),
    source_image ( image ), estompage (est) {

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

EstompageImage::~EstompageImage() {
    delete source_image;
    delete[] source_lines;
    delete[] source_lines_buffer;
}


template<typename T>
int EstompageImage::_getline ( T* buffer, int line ) {
    // L'image source fait une ligne de plus en haut et en bas (ligne 0 de l'image estompée = ligne 1 de l'image source)
    // et une colonne de plus à gauche et à droite
    // Pour obtenir la ligne 0 de l'image estompée, on a besoin des lignes 0, 1 et 2 de l'image source
    // Plus généralement, pour avoir la ligne n de l'image estompée, on a besoin des lignes
    // n, n+1 et n+2 de l'image source

    // On range les lignes sources dans un buffer qui peut en stocker 3
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

    int column_orig = 1;
    int column = 0;
    double value;
    float dzdx,dzdy,slope,aspect;
    float a,b,c,d,e,f,g,h,i;

    while ( column < width ) {

        a = ( * ( line1+column_orig-1 ) );
        b = ( * ( line1+column_orig ) );
        c = ( * ( line1+column_orig+1 ) );
        d = ( * ( line2+column_orig-1 ) );
        e = ( * ( line2+column_orig ) );
        f = ( * ( line2+column_orig+1 ) );
        g = ( * ( line3+column_orig-1 ) );
        h = ( * ( line3+column_orig ) );
        i = ( * ( line3+column_orig+1 ) );

        if (a == estompage->input_nodata_value || b == estompage->input_nodata_value || c == estompage->input_nodata_value || d == estompage->input_nodata_value || e == estompage->input_nodata_value ||
                f == estompage->input_nodata_value || g == estompage->input_nodata_value || h == estompage->input_nodata_value || i == estompage->input_nodata_value) {
            value = estompage->estompage_nodata_value;
        } else {

            dzdx = ((c + 2*f + i) - (a + 2*d + g)) / (8 * resxmeter);
            dzdy = ((g + 2*h + i) - (a + 2*b + c)) / (8 * resymeter);
            
            slope = atan(estompage->z_factor * sqrt(dzdx*dzdx+dzdy*dzdy));

            if (dzdx != 0) {
                aspect = atan2(dzdy,-dzdx);
                if (aspect < 0) {
                    aspect = 2 * M_PI + aspect;
                }
            } else {
                if (dzdy > 0) {
                    aspect = M_PI_2;
                } else {
                    aspect = 2 * M_PI - M_PI_2;
                }
            }

            value = 255.0 * ((cos(estompage->zenith) * cos(slope)) + (sin(estompage->zenith) * sin(slope) * cos(estompage->azimuth - aspect)));
            if (value < 0) {value = estompage->estompage_nodata_value;}
        }

        * ( buffer + ( column++ ) ) = ( T ) ( value );
        column_orig++;
    }

    return width * sizeof(T);
}