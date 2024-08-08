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
 * \file ReprojectedImage.cpp
 ** \~french
 * \brief Implémentation de la classe ReprojectedImage, permettant la reprojection d'image
 ** \~english
 * \brief Implement classe ReprojectedImage, allowing image reprojecting
 */

#include "image/ReprojectedImage.h"

#include <string>
#include "image/Image.h"
#include "processors/Grid.h"
#include <boost/log/trivial.hpp>

#include "utils/Utils.h"
#include <cmath>

void ReprojectedImage::initialize () {

    x_ratio = grid->get_x_ratio();
    y_ratio = grid->get_y_ratio();

    // On calcule le nombre de pixels sources à considérer dans l'interpolation, dans le sens des x et des y
    x_kernel_size = ceil ( 2 * kernel.size ( x_ratio ) );
    y_kernel_size = ceil ( 2 * kernel.size ( y_ratio ) );

    if ( ! source_image->get_mask() ) {
        use_masks = false;
    }

    memorized_lines = 2*y_kernel_size + ceil ( grid->get_y_maximal_gap() );

    /* -------------------- PLACE MEMOIRE ------------------- */

    // nombre d'éléments d'une ligne de l'image source, arrondi au multiple de 4 supérieur.
    int srcImgSize = 4* ( ( source_image->get_width() *channels + 3 ) /4 );
    int srcMskSize = 4* ( ( source_image->get_width() + 3 ) /4 );

    // nombre d'éléments d'une ligne de l'image calculée, arrondi au multiple de 4 supérieur.
    int outImgSize = 4* ( ( width*channels + 3 ) /4 );
    int outMskSize = 4* ( ( width + 3 ) /4 );

    // nombre d'éléments d'une ligne de la grille, arrondi au multiple de 4 supérieur.
    int gridSize = 4* ( ( width*channels + 3 ) /4 );

    // nombre d'élément dans les tableaux de poids, arrondi au multiple de 4 supérieur.
    int kxSize = 4* ( ( x_kernel_size+3 ) /4 );
    int kySize = 4* ( ( y_kernel_size+3 ) /4 );

    int globalSize = srcImgSize * double(memorized_lines) * sizeof ( float ) // place pour "memorizedLines" lignes d'image source
                     + outImgSize * 8 * sizeof ( float ) // 4 lignes reprojetées, en multiplexées et en séparées => 8
                     + gridSize * 8 * sizeof ( float ) // 4 lignes de la grille, X et Y => 8

                     /*   1024 possibilités de poids
                      * + poids pour 4 lignes, multiplexés
                      * + extrait des 4 lignes sources, sur lesquelles appliquer les poids
                      */
                     + kxSize * ( 1028 + 4*channels ) * sizeof ( float )
                     + kySize * ( 1028 + 4*channels ) * sizeof ( float );

    if ( use_masks ) {
        globalSize += srcMskSize * memorized_lines * sizeof ( float ) // place pour charger "memorizedLines" lignes du masque source
                      + outMskSize * 8 * sizeof ( float ) // 4 lignes reprojetées, en multiplexées et en séparées => 8
                      + kxSize * 4 * sizeof ( float )
                      + kySize * 4 * sizeof ( float );
    }

    /* Allocation de la mémoire pour tous les buffers en une seule fois :
     *  - gain de temps (l'allocation est une action qui prend du temps)
     *  - tous les buffers sont côtes à côtes dans la mémoire, gain de temps lors des lectures/écritures
     */
    __buffer = ( float* ) _mm_malloc ( globalSize, 16 ); // Allocation allignée sur 16 octets pour SSE
    memset ( __buffer, 0, globalSize );

    float* B = __buffer;

    /* -------------------- PARTIE IMAGE -------------------- */

    src_image_buffer = new float*[memorized_lines];
    src_line_index = new int[memorized_lines];

    for ( int i = 0; i < memorized_lines; i++ ) {
        src_image_buffer[i] = B;
        src_line_index[i] = -1;
        B += srcImgSize;
    }

    for ( int i = 0; i < 4; i++ ) {
        dst_image_buffer[i] = B;
        B += outImgSize;
    }

    dst_line_index = -1;

    mux_dst_image_buffer = B;
    B += 4*outImgSize;

    current_source_pixels = B;
    B += 4*channels*kxSize;
    current_x_interpolated_pixels = B;
    B += 4*channels*kySize;

    /* -------------------- PARTIE MASQUE ------------------- */

    if ( use_masks ) {
        src_mask_buffer = new float*[memorized_lines];
        for ( int i = 0; i < memorized_lines; i++ ) {
            src_mask_buffer[i] = B;
            B += srcMskSize;
        }

        for ( int i = 0; i < 4; i++ ) {
            dst_mask_buffer[i] = B;
            B += outMskSize;
        }

        mux_dst_mask_buffer = B;
        B += 4*outMskSize;

        current_source_masks = B;
        B += 4*kxSize;
        current_x_interpolated_masks = B;
        B += 4*kySize;
    }

    /* -------------------- PARTIE POIDS -------------------- */

    for ( int i = 0; i < 4; i++ ) {
        x_coords[i] = B;
        B += gridSize;
        y_coords[i] = B;
        B += gridSize;
    }

    for ( int i = 0; i < 1024; i++ ) {
        x_weights[i] = B;
        B += kxSize;
        y_weights[i] = B;
        B += kySize;
    }
    x_current_weights = B;
    B += 4*kxSize;
    y_current_weights = B;
    B += 4*kySize;

    for ( int i = 0; i < 1024; i++ ) {
        int lgX = x_kernel_size;
        int lgY = y_kernel_size;
        xmin[i] = kernel.weight ( x_weights[i], lgX, double ( i ) /1024. + x_kernel_size, source_image->get_width() ) - x_kernel_size;
        ymin[i] = kernel.weight ( y_weights[i], lgY, double ( i ) /1024. + y_kernel_size, source_image->get_height() ) - y_kernel_size;
    }
}

int ReprojectedImage::get_source_line_index ( int line ) {

    if ( src_line_index[line % memorized_lines] == line ) {
        // On a déjà la ligne source en mémoire, on renvoie donc son index (place dans src_image_buffer)
        return ( line % memorized_lines );
    }

    // Récupération de la ligne voulue
    source_image->get_line ( src_image_buffer[line % memorized_lines], line );

    if ( use_masks ) {
        source_image->get_mask()->get_line ( src_mask_buffer[line % memorized_lines], line );
    }

    // Mis à jour de l'index
    src_line_index[line % memorized_lines] = line;

    return line % memorized_lines;
}

float* ReprojectedImage::compute_line ( int line ) {

    if ( line/4 == dst_line_index ) {
        return dst_image_buffer[line%4];
    }
    dst_line_index = line/4;

    for ( int i = 0; i < 4; i++ ) {
        if ( 4*dst_line_index+i < height ) {
            grid->get_line ( 4*dst_line_index+i, x_coords[i], y_coords[i] );
        } else {
            memcpy ( x_coords[i], x_coords[0], width*sizeof ( float ) );
            memcpy ( y_coords[i], y_coords[0], width*sizeof ( float ) );
        }
    }

    int Ix[4], Iy[4];

    for ( int x = 0; x < width; x++ ) {

        for ( int i = 0; i < 4; i++ ) {
            Ix[i] = ( x_coords[i][x] - floor ( x_coords[i][x] ) ) * 1024;
            Iy[i] = ( y_coords[i][x] - floor ( y_coords[i][x] ) ) * 1024;
        }

        multiplex ( x_current_weights, x_weights[Ix[0]], x_weights[Ix[1]], x_weights[Ix[2]], x_weights[Ix[3]], x_kernel_size );
        multiplex ( y_current_weights, y_weights[Iy[0]], y_weights[Iy[1]], y_weights[Iy[2]], y_weights[Iy[3]], y_kernel_size );

        int y0 = ( int ) ( y_coords[0][x] ) + ymin[Iy[0]];
        int y1 = ( int ) ( y_coords[1][x] ) + ymin[Iy[1]];
        int y2 = ( int ) ( y_coords[2][x] ) + ymin[Iy[2]];
        int y3 = ( int ) ( y_coords[3][x] ) + ymin[Iy[3]];
        int dx0 = ( ( int ) ( x_coords[0][x] ) + xmin[Ix[0]] );
        int dx1 = ( ( int ) ( x_coords[1][x] ) + xmin[Ix[1]] );
        int dx2 = ( ( int ) ( x_coords[2][x] ) + xmin[Ix[2]] );
        int dx3 = ( ( int ) ( x_coords[3][x] ) + xmin[Ix[3]] );

        for ( int j = 0; j < y_kernel_size; j++ ) {

            multiplex_unaligned ( current_source_pixels,
                                  src_image_buffer[get_source_line_index ( y0 + j )] + dx0*channels,
                                  src_image_buffer[get_source_line_index ( y1 + j )] + dx1*channels,
                                  src_image_buffer[get_source_line_index ( y2 + j )] + dx2*channels,
                                  src_image_buffer[get_source_line_index ( y3 + j )] + dx3*channels,
                                  x_kernel_size * channels );

            if ( use_masks ) {
                multiplex_unaligned ( current_source_masks,
                                      src_mask_buffer[get_source_line_index ( y0 + j )] + dx0,
                                      src_mask_buffer[get_source_line_index ( y1 + j )] + dx1,
                                      src_mask_buffer[get_source_line_index ( y2 + j )] + dx2,
                                      src_mask_buffer[get_source_line_index ( y3 + j )] + dx3,
                                      x_kernel_size );

                dot_prod ( channels, x_kernel_size,
                           current_x_interpolated_pixels + 4*j*channels,
                           current_x_interpolated_masks + 4*j,
                           current_source_pixels,
                           current_source_masks,
                           x_current_weights );
            } else {
                dot_prod ( channels, x_kernel_size,
                           current_x_interpolated_pixels + 4*j*channels,
                           current_source_pixels,
                           x_current_weights );
            }
        }

        if ( use_masks ) {
            dot_prod ( channels, y_kernel_size,
                       mux_dst_image_buffer + 4*x*channels,
                       mux_dst_mask_buffer + 4*x,
                       current_x_interpolated_pixels,
                       current_x_interpolated_masks,
                       y_current_weights );
        } else {
            dot_prod ( channels, y_kernel_size,
                       mux_dst_image_buffer + 4*x*channels,
                       current_x_interpolated_pixels,
                       y_current_weights );
        }
    }

    demultiplex ( dst_image_buffer[0], dst_image_buffer[1], dst_image_buffer[2], dst_image_buffer[3],
                  mux_dst_image_buffer, width*channels );

    if ( use_masks ) {
        demultiplex ( dst_mask_buffer[0], dst_mask_buffer[1], dst_mask_buffer[2], dst_mask_buffer[3],
                      mux_dst_mask_buffer, width );
    }

    return dst_image_buffer[line%4];
}

int ReprojectedImage::get_line ( uint8_t* buffer, int line ) {
    const float* dst_line = compute_line ( line );
    convert ( buffer, dst_line, width*channels );
    return width*channels;
}

int ReprojectedImage::get_line ( uint16_t* buffer, int line ) {
    const float* dst_line = compute_line ( line );
    convert ( buffer, dst_line, width*channels );
    return width*channels;
}

int ReprojectedImage::get_line ( float* buffer, int line ) {
    const float* dst_line = compute_line ( line );
    convert ( buffer, dst_line, width*channels );
    return width*channels;
}




