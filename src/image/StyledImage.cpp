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

/**
 * \file TerrainrgbImage.cpp
 * \~french
 * \brief Implémentation de la classe TerrainrgbImage permettant l'application du terrainrgb à une image.
 * \~english
 * \brief Implement the TerrainrgbImage Class handling computing of Terrainrgb style to an image.
 */
#include "image/StyledImage.h"
#include <boost/log/trivial.hpp>

int StyledImage::get_line(float *buffer, int line) {
    if (style != NULL && !style->is_identity()) {
        return _getline(buffer, line);
    }
    else {
        return source_image->get_line(buffer, line);
    }
}

int StyledImage::get_line(uint16_t *buffer, int line) {
    if (style != NULL && !style->is_identity()) {
        return _getline(buffer, line);
    }
    else {
        return source_image->get_line(buffer, line);
    }
}

int StyledImage::get_line(uint8_t *buffer, int line) {
    if (style != NULL && !style->is_identity()) {
        return _getline(buffer, line);
    }
    else {
        return source_image->get_line(buffer, line);
    }
}

StyledImage::StyledImage(Image *input_image, Style *input_style, int offset) : Image(input_image->get_width() - offset, input_image->get_height() - offset, input_style->get_channels(input_image->get_channels()), input_image->get_bbox()), multi_line_buffer(false) {
    style = input_style;
    source_image = input_image;

    if (style->estompage_defined()){
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
    
    else if (style->pente_defined()) {
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

    else if (style->aspect_defined()) {
        resolution = (input_image->get_mean_resolution());

        // On réduit la bbox d'un pixel de chaque côté
        BoundingBox<double> bb = source_image->get_bbox();
        bb.xmin += source_image->get_resx();
        bb.ymin += source_image->get_resy();
        bb.xmax -= source_image->get_resx();
        bb.ymax -= source_image->get_resy();
        set_bbox(bb);
    
        set_crs(source_image->get_crs());
    
        // Buffer de lignes sources
        memorized_source_lines = 3;
    
        source_lines = new int[memorized_source_lines];
        for (int i = 0; i < memorized_source_lines; i++) {
            source_lines[i] = -1;
        }
        source_lines_buffer = new float[source_image->get_width() * memorized_source_lines];
    
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

    else if (style->terrainrgb_defined()) {
        if (source_image->get_channels() == 1) {
            channels = 3;
        } else {
            channels = input_image->get_channels();
        }    
    }

    else if (style->white_to_alpha_defined()) {
        if (source_image->get_channels() == 3 || source_image->get_channels() == 4) {
            channels = style->get_white_to_alpha()->destination.size();
        } else {
            channels = input_image->get_channels();
        }
    }

    if (style->palette_defined()){
        // Il n'y aura application de la palette et modification des canaux que si
        // - la palette n'est pas nulle et pas vide
        // - l'image source est sur un canal
        if ( source_image->get_channels() == 1 && style->get_palette() != NULL && ! style->get_palette()->is_empty() ) {
            if (style->get_palette()->is_no_alpha()) {
                channels = 3;
            } else {
                channels = 4;
            }
        } else {
            channels = source_image->get_channels();
        }
    }
}

template <typename T>
int StyledImage::_getline(T *buffer, int line) {
    int space;
    float *source = new float[source_image->get_width() * source_image->get_channels()];
    source_image->get_line(source, line);

    if (style->estompage_defined()) {
        // L'image source fait une ligne de plus en haut et en bas (ligne 0 de l'image estompée = ligne 1 de l'image source)
        // et une colonne de plus à gauche et à droite
        // Pour obtenir la ligne 0 de l'image estompée, on a besoin des lignes 0, 1 et 2 de l'image source
        // Plus généralement, pour avoir la ligne n de l'image estompée, on a besoin des lignes
        // n, n+1 et n+2 de l'image source

        // On range les lignes sources dans un buffer qui peut en stocker 3
        // La ligne source n est stockée en (n % memorized_source_lines) ème position

        multi_line_buffer = true;
        // calcul des emplacements dans le buffer des 3 lignes sources nécessaires
        float *line1 = source_lines_buffer + (line % memorized_source_lines) * source_image->get_width();
        float *line2 = source_lines_buffer + ((line + 1) % memorized_source_lines) * source_image->get_width();
        float *line3 = source_lines_buffer + ((line + 2) % memorized_source_lines) * source_image->get_width();

        // ligne du dessus
        if (source_lines[line % memorized_source_lines] != line) {
            // la ligne source 'line' n'est pas celle stockée dans le buffer, on doit la lire
            source_image->get_line(line1, line);
            source_lines[line % memorized_source_lines] = line;
        }
        // ligne du milieu
        if (source_lines[(line + 1) % memorized_source_lines] != line + 1) {
            // la ligne source 'line + 1' n'est pas celle stockée dans le buffer, on doit la lire
            source_image->get_line(line2, line + 1);
            source_lines[(line + 1) % memorized_source_lines] = line + 1;
        }
        // ligne du dessous
        if (source_lines[(line + 2) % memorized_source_lines] != line + 2) {
            // la ligne source 'line + 2' n'est pas celle stockée dans le buffer, on doit la lire
            source_image->get_line(line3, line + 2);
            source_lines[(line + 2) % memorized_source_lines] = line + 2;
        }

        int column_orig = 1;
        int column = 0;
        double value;
        float dzdx, dzdy, slope, aspect;
        float a, b, c, d, e, f, g, h, i;

        while (column < width) {
            a = (*(line1 + column_orig - 1));
            b = (*(line1 + column_orig));
            c = (*(line1 + column_orig + 1));
            d = (*(line2 + column_orig - 1));
            e = (*(line2 + column_orig));
            f = (*(line2 + column_orig + 1));
            g = (*(line3 + column_orig - 1));
            h = (*(line3 + column_orig));
            i = (*(line3 + column_orig + 1));

            if (a == style->get_estompage()->input_nodata_value || b == style->get_estompage()->input_nodata_value || c == style->get_estompage()->input_nodata_value || d == style->get_estompage()->input_nodata_value || e == style->get_estompage()->input_nodata_value ||
                f == style->get_estompage()->input_nodata_value || g == style->get_estompage()->input_nodata_value || h == style->get_estompage()->input_nodata_value || i == style->get_estompage()->input_nodata_value) {
                value = style->get_estompage()->estompage_nodata_value;
            }
            else {

                dzdx = ((c + 2 * f + i) - (a + 2 * d + g)) / (8 * resxmeter);
                dzdy = ((g + 2 * h + i) - (a + 2 * b + c)) / (8 * resymeter);

                slope = atan(style->get_estompage()->z_factor * sqrt(dzdx * dzdx + dzdy * dzdy));

                if (dzdx != 0) {
                    aspect = atan2(dzdy, -dzdx);
                    if (aspect < 0) {
                        aspect = 2 * M_PI + aspect;
                    }
                }
                else {
                    if (dzdy > 0) {
                        aspect = M_PI_2;
                    }
                    else {
                        aspect = 2 * M_PI - M_PI_2;
                    }
                }

                value = 255.0 * ((cos(style->get_estompage()->zenith) * cos(slope)) + (sin(style->get_estompage()->zenith) * sin(slope) * cos(style->get_estompage()->azimuth - aspect)));
                if (value < 0) {
                    value = style->get_estompage()->estompage_nodata_value;
                }
            }

            *(buffer + (column++)) = (T)(value);
            column_orig++;
        }

        space = width * sizeof(T);
    }

    else if (style->pente_defined()) {
        // L'image source fait une ligne de plus en haut et en bas (ligne 0 de l'image estompée = ligne 1 de l'image source)
        // et une colonne de plus à gauche et à droite
        // Pour obtenir la ligne 0 de l'image estompée, on a besoin des lignes 0, 1 et 2 de l'image source
        // Plus généralement, pour avoir la ligne n de l'image estompée, on a besoin des lignes
        // n, n+1 et n+2 de l'image source
    
        // On range les lignes sources dans un buffer qui peut en stocker "memorized_source_lines"
        // La ligne source n est stockée en (n % memorized_source_lines) ème position
        
        multi_line_buffer = true;

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
    
        if (style->get_pente()->algo == "H") {
            resx = 8.0 * resxmeter;
            resy = 8.0 * resymeter;
        } else if (style->get_pente()->algo == "Z") {
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
    
            if (a == style->get_pente()->input_nodata_value || b == style->get_pente()->input_nodata_value || c == style->get_pente()->input_nodata_value || d == style->get_pente()->input_nodata_value || e == style->get_pente()->input_nodata_value ||
                    f == style->get_pente()->input_nodata_value || g == style->get_pente()->input_nodata_value || h == style->get_pente()->input_nodata_value || i == style->get_pente()->input_nodata_value) {
                slope = style->get_pente()->slope_nodata_value;
            } else {
    
                if (style->get_pente()->algo == "H") {
                    dzdx = (( c + 2.0 * f + i) - (a + 2.0 *  d + g)) / resx;
                    dzdy = (( g + 2.0 * h + i) - (a + 2.0 *  b + c)) / resy;
                } else if (style->get_pente()->algo == "Z" ) {
                    dzdx = (f - d) / resx;
                    dzdy = (h - b) / resy;
                } else {
    
                }
    
    
                if (style->get_pente()->unit == "pourcent") {
                    slope = sqrt(pow(dzdx,2.0) + pow(dzdy,2.0)) * 100.0;
                } else if (style->get_pente()->unit == "degree") {
                    rise = sqrt(pow(dzdx,2.0) + pow(dzdy,2.0));
    
                    slope = atan(rise) * 180.0 / M_PI;
                    if (slope>90.0){slope = 180.0-slope;}
                } else {
                    slope = 0;
                }
    
                if (slope > style->get_pente()->max_slope) {
                    slope = style->get_pente()->max_slope;
                }
    
            }
    
            * ( buffer + ( column++ ) ) = ( T ) ( slope );
            columnOrig++;
    
        }
    
        space = width * sizeof(T);
    }
    
    else if (style->aspect_defined()) {
        // L'image source fait une ligne de plus en haut et en bas (ligne 0 de l'image estompée = ligne 1 de l'image source)
        // et une colonne de plus à gauche et à droite
        // Pour obtenir la ligne 0 de l'image aspect, on a besoin des lignes 0, 1 et 2 de l'image source
        // Plus généralement, pour avoir la ligne n de l'image aspect, on a besoin des lignes
        // n, n+1 et n+2 de l'image source
    
        // On range les lignes sources dans un buffer qui peut en stocker 3
        // La ligne source n est stockée en (n % memorized_source_lines) ème position
        
        multi_line_buffer = true;

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
        double value,value1,value2,slope;
        float a,b,c,d,e,f,g,h,i;
    
        //calcul de la variable sur toutes les autres colonnes
        while ( column < width ) {
    
            a = ( * ( line1+columnOrig-1 ) );
            b = ( * ( line1+columnOrig ) );
            c = ( * ( line1+columnOrig+1 ) );
            d = ( * ( line2+columnOrig-1 ) );
            e = ( * ( line2+columnOrig ) );
            f = ( * ( line2+columnOrig+1 ) );
            g = ( * ( line3+columnOrig-1 ) );
            h = ( * ( line3+columnOrig ) );
            i = ( * ( line3+columnOrig+1 ) );
    
            if (a == style->get_aspect()->input_nodata_value || b == style->get_aspect()->input_nodata_value || c == style->get_aspect()->input_nodata_value || d == style->get_aspect()->input_nodata_value || e == style->get_aspect()->input_nodata_value ||
                    f == style->get_aspect()->input_nodata_value || g == style->get_aspect()->input_nodata_value || h == style->get_aspect()->input_nodata_value || i == style->get_aspect()->input_nodata_value) {
                value = style->get_aspect()->aspect_nodata_value;
            } else {
    
                value1 = (matrix[2] * c + matrix[5] * f + matrix[8] * i - matrix[0] * a - matrix[3] * d - matrix[6] * g);
                value2 = (matrix[0] * a + matrix[1] * b + matrix[2] * c - matrix[6] * g - matrix[7] * h - matrix[8] * i);
    
                //calcul de la pente pour ne pas afficher l'exposition en dessous d'une certaine valeur de pente
                slope = sqrt(pow(value1,2.0)+pow(value2,2.0));
                if (slope < style->get_aspect()->min_slope) {
                    value = style->get_aspect()->aspect_nodata_value;
                } else {
                    value = (atan2(value1,value2) + M_PI) * 180 / M_PI;
                }
            }
    
            * ( buffer + ( column++ ) ) = (T) ( value );
            columnOrig++;
        }
    
        space = width * sizeof(T);
    }

    else if (style->terrainrgb_defined()) {
        switch ( channels ) {
        case 3:
            for (int i = 0; i < source_image->get_width() ; i++ ) {
                
                // découpage de l'altitude en RGB suivant la formule suivante : height = min_elevation + ((Red * 256 * 256 + Green * 256 + Blue) * step)
                int base = (std::max( *(source+i), style->get_terrainrgb()->min_elevation) -  style->get_terrainrgb()->min_elevation) / style->get_terrainrgb()->step;
                int red = (base / (256 * 256) % 256);
                int green = ((base - red * 256 * 256) / 256 % 256);
                int blue = (base - red * 256 * 256 - green * 256);
    
                * ( buffer+i*3 ) = (T) red;
                * ( buffer+i*3+1 ) = (T) green;
                * ( buffer+i*3+2 ) = (T) blue;
            }
            break;
        }
    
        space = width * sizeof ( T ) * channels;
    }

    else if (style->white_to_alpha_defined()) {
        switch ( channels ) {
        case 3:
            for (int i = 0; i < source_image->get_width() ; i++ ) {
                // découpage de l'altitude en RGB suivant la formule suivante : height = min_elevation + ((Red * 256 * 256 + Green * 256 + Blue) * step)
                int red = *(source+i*3);
                int green = *(source+i*3+1);
                int blue = *(source+i*3+2);
                int min = std::min({red,blue,green});

                if (min>=255-style->get_white_to_alpha()->tolerance){
                    * ( buffer+i*3 ) = (T) red;
                    * ( buffer+i*3+1 ) = (T) green;
                    * ( buffer+i*3+2 ) = (T) blue;
                    * ( buffer+i*3+3 ) = (T) 0;
                }
                else{
                    * ( buffer+i*3 ) = (T) red;
                    * ( buffer+i*3+1 ) = (T) green;
                    * ( buffer+i*3+2 ) = (T) blue;
                    * ( buffer+i*3+3 ) = (T) 255;
                }
            }
            break;
        case 4:
            for (int i = 0; i < source_image->get_width() ; i++ ) {
                // découpage de l'altitude en RGB suivant la formule suivante : height = min_elevation + ((Red * 256 * 256 + Green * 256 + Blue) * step)
                int red = *(source+i*4);
                int green = *(source+i*4+1);
                int blue = *(source+i*4+2);
                int min = std::min({red,blue,green});

                if (min>=255-style->get_white_to_alpha()->tolerance){
                    * ( buffer+i*4 ) = (T) red;
                    * ( buffer+i*4+1 ) = (T) green;
                    * ( buffer+i*4+2 ) = (T) blue;
                    * ( buffer+i*4+3 ) = (T) 0;
                }
                else{
                    * ( buffer+i*4 ) = (T) red;
                    * ( buffer+i*4+1 ) = (T) green;
                    * ( buffer+i*4+2 ) = (T) blue;
                    * ( buffer+i*4+3 ) = (T) 255;
                }
            }
            break;
        }
    
        space = width * sizeof ( T ) * channels;
    }

    if (style->palette_defined()){
        switch ( channels ) {
        case 4:
            for (int i = 0; i < source_image->get_width() ; i++ ) {
                Colour iColour = style->get_palette()->get_colour ( * ( source+i ) );
                * ( buffer+i*4 ) = (T) iColour.r;
                * ( buffer+i*4+1 ) = (T) iColour.g;
                * ( buffer+i*4+2 ) = (T) iColour.b;
                * ( buffer+i*4+3 ) = (T) iColour.a;
            }
            break;
            
        case 3:
            for (int i = 0; i < source_image->get_width() ; i++ ) {
                Colour iColour = style->get_palette()->get_colour ( * ( source+i ) );
                * ( buffer+i*3 ) = (T) iColour.r;
                * ( buffer+i*3+1 ) = (T) iColour.g;
                * ( buffer+i*3+2 ) = (T) iColour.b;
            }
            break;
        }
    
        space = width * sizeof ( T ) * channels;
    }

    delete[] source;
    return space;
}

StyledImage *StyledImage::create(Image *input_image, Style *input_style) {
    int offset=0;
    if (input_style->estompage_defined() || input_style->pente_defined() || input_style->aspect_defined()) {
        if (input_image->get_width()<=2 && input_image->get_height()<=2){
            BOOST_LOG_TRIVIAL(error)<<"L'image source est trop petite pour appliquer ce style";
            return NULL;
        }
        if (input_image->get_channels()!=1){
            BOOST_LOG_TRIVIAL(error)<<"Ce style ne s'applique que sur une image source à un canal";
            return NULL;
        }
        offset=2;
    }
    if (input_style->terrainrgb_defined() && input_style->palette_defined()) {
        BOOST_LOG_TRIVIAL(error)<<"Les styles terrainrgb et palette ne sont pas compatibles";
        return NULL;
    }
    if (input_style->terrainrgb_defined()){
        if (input_image->get_channels()!=1){
            BOOST_LOG_TRIVIAL(error)<<"Ce style ne s'applique que sur une image source à un canal";
            return NULL;
        }
    }
    if (input_style->white_to_alpha_defined()){
        if (input_image->get_channels()!=3 || input_image->get_channels()!=4){
            BOOST_LOG_TRIVIAL(error)<<"Ce style ne s'applique que sur une image source à un canal";
            return NULL;
        }
    }
    return new StyledImage(input_image,input_style,offset);

}

StyledImage::~StyledImage() {

    
    if (multi_line_buffer) {
        delete[] source_lines;
        delete[] source_lines_buffer;
    }
    delete source_image;
}

void StyledImage::print() {
    BOOST_LOG_TRIVIAL(info) <<  "" ;
    BOOST_LOG_TRIVIAL(info) <<  "--------- StyledImage -----------" ;
    if (style->aspect_defined()){
        BOOST_LOG_TRIVIAL(info) <<  "--------- Aspect -----------" ;
    }
    if (style->estompage_defined()){
        BOOST_LOG_TRIVIAL(info) <<  "--------- Estompage -----------" ;
    }
    if (style->pente_defined()){
        BOOST_LOG_TRIVIAL(info) <<  "--------- Pente -----------" ;
    }
    if (style->terrainrgb_defined()){
        BOOST_LOG_TRIVIAL(info) <<  "--------- Terrainrgb -----------" ;
    }
    if (style->white_to_alpha_defined()){
        BOOST_LOG_TRIVIAL(info) <<  "--------- White_to_alpha -----------" ;
    }
    if (style->palette_defined()){
        BOOST_LOG_TRIVIAL(info) <<  "--------- Palette -----------" ;
    }
    Image::print();
    BOOST_LOG_TRIVIAL(info) <<  "" ;
}
