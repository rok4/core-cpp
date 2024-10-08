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
 * \file PixelConverter.h
 * \~french
 * \brief Définition de la classe PixelConverter, permettant de convertir à la volée les pixels
 * \~english
 * \brief Define the PixelConverter class
 */

#pragma once

#include <string.h>
#include <stdio.h>
#include <boost/log/trivial.hpp>
#include "rok4/utils/Utils.h"
#include "rok4/enums/Format.h"

/** \~ \author Institut national de l'information géographique et forestière
 ** \~french
 * \brief Outil de conversion des images à la volée
 * \details Une instance de PixelConverter va permettre, au moment de la lecture d'une ligne issue d'une image fichier (FileImage), de modifier le format des canaux.
 *
 * On ne permet pour le moment que l'ajout ou la suppression de canal pour le format en entrée et en sortie entier sur 8 bits.
 ** \~english
 * \brief Converter for FileImage
 */
class PixelConverter {

private:
    /**
     * \~french \brief Format du canal en entrée
     * \~english \brief Input sample format
     */
    SampleFormat::eSampleFormat input_sampleformat;
    /**
     * \~french \brief Format du canal en sortie
     * \~english \brief Output sample format
     */
    SampleFormat::eSampleFormat output_sampleformat;

    /**
     * \~french \brief Nombre de canal en entrée
     * \~english \brief Input number of channel
     */
    int input_samplesperpixel;
    /**
     * \~french \brief Nombre de canal en sortie
     * \~english \brief Output number of channel
     */
    int output_samplesperpixel;

    /**
     * \~french \brief Largeur d'une ligne à convertir
     * \~english \brief Width of line to convert
     */
    int width;

    /**
     * \~french \brief La conversion est-elle possible ?
     * \~english \brief Conversion is allowed ?
     */
    bool ok;

public:
    /** \~french
     * \brief Crée un objet PixelConverter
     ** \~english
     * \brief Create a PixelConverter
     */
    PixelConverter ( int w, SampleFormat::eSampleFormat isf, int ispp, SampleFormat::eSampleFormat osf, int ospp ) : 
        width(w), input_sampleformat (isf), input_samplesperpixel(ispp),
        output_sampleformat (osf), output_samplesperpixel(ospp) 
    {
        ok = false;

        if (input_sampleformat != SampleFormat::UINT8) {
            BOOST_LOG_TRIVIAL(warning) << "PixelConverter only handle 8 bits sample";
            return;
        }
        if (input_sampleformat != output_sampleformat) {
            BOOST_LOG_TRIVIAL(warning) << "PixelConverter doesn't handle different samples format";
            return;
        }

        if (input_samplesperpixel == output_samplesperpixel) {
            BOOST_LOG_TRIVIAL(warning) << "PixelConverter have not to be used if number of samples per pixel is the same";
            return;
        }

        ok = true;
    }

    /**
     * \~french \brief La conversion est-elle possible ?
     * \~english \brief Conversion is allowed ?
     */
    bool is_ok () {
        return ok;
    }

    /**
     * \~french \brief Retourne le format de canal en sortie
     * \~english \brief Get the output sample format
     */
    SampleFormat::eSampleFormat get_sample_format () {
        return output_sampleformat;
    }

    /**
     * \~french
     * \brief Retourne la taille en octet d'un pixel en sortie
     * \~english
     * \brief Return the output pixel's byte size
     */
    int get_pixel_size () {
        return SampleFormat::get_bits_per_sample(output_sampleformat) * output_samplesperpixel / 8;
    }

    /**
     * \~french \brief Retourne le nombre de canaux en sortie
     * \~english \brief Get the output number of channels
     */
    int get_channels () {
        return output_samplesperpixel;
    }

    /**
     * \~french \brief Affiche les information sur le convertisseur
     * \~english \brief Print converter's values
     */
    void print() {
        BOOST_LOG_TRIVIAL(info) <<  "" ;
        BOOST_LOG_TRIVIAL(info) <<  "---------- PixelConverter ------------" ;
        BOOST_LOG_TRIVIAL(info) <<  "\t- Width : " << width ;
        BOOST_LOG_TRIVIAL(info) <<  "\t- SampleFormat : " << SampleFormat::to_string(input_sampleformat) << " -> " << SampleFormat::to_string(output_sampleformat) ;
        BOOST_LOG_TRIVIAL(info) <<  "\t- Samples per pixel : " << input_samplesperpixel << " -> " << output_samplesperpixel ;
        BOOST_LOG_TRIVIAL(info) <<  "" ;
    }

    /**
     * \~french
     * \brief Destructeur par défaut
     * \~english
     * \brief Default destructor
     */
    virtual ~PixelConverter() {
    }


    /**
     * \~french \brief Convertit la ligne de pixels
     * \~english \brief Convert pixels line
     * \~french
     * \param[in] bufferto Buffer de sortie où stocker la ligne convertie
     * \param[in] bufferfrom Buffer de stockage de la ligne à convertir
     */
    template<typename T>
    void convert_line ( T* bufferto, T* bufferfrom ) {
        
        T defaultAlpha;
        if (sizeof(T) == 1) {
            defaultAlpha = (T) 255;
        } else if (sizeof(T) == 2) {
            defaultAlpha = (T) 65535;
        } else {
            defaultAlpha = (T) 1;
        }

        /********************** Depuis 1 canal ********************/
        if ( input_samplesperpixel == 1) {
            if (output_samplesperpixel == 2) {
                for ( int i = 0; i < width; i++ ) {
                    bufferto[2*i] = bufferfrom[i];
                    bufferto[2*i+1] = defaultAlpha;
                }
                return;
            }
            if (output_samplesperpixel == 3) {
                for ( int i = 0; i < width; i++ ) {
                    bufferto[3*i] = bufferto[3*i+1] = bufferto[3*i+2] = bufferfrom[i];
                }
                return;
            }
            if (output_samplesperpixel == 4) {
                for ( int i = 0; i < width; i++ ) {
                    bufferto[4*i] = bufferto[4*i+1] = bufferto[4*i+2] = bufferfrom[i];
                    bufferto[4*i+3] = defaultAlpha;
                }
                return;
            }
        }
        
        /********************** Depuis 2 canaux *******************/
        if ( input_samplesperpixel == 2) {
            if (output_samplesperpixel == 1) {
                for ( int i = 0; i < width; i++ ) {
                    bufferto[i] = bufferfrom[2*i];
                }
                return;
            }
            if (output_samplesperpixel == 3) {
                for ( int i = 0; i < width; i++ ) {
                    bufferto[3*i] = bufferto[3*i+1] = bufferto[3*i+2] = bufferfrom[2*i];
                }
                return;
            }
            if (output_samplesperpixel == 4) {
                for ( int i = 0; i < width; i++ ) {
                    bufferto[4*i] = bufferto[4*i+1] = bufferto[4*i+2] = bufferfrom[2*i];
                    bufferto[4*i+3] = bufferfrom[2*i + 1];
                }
                return;
            }
        }
        
        /********************** Depuis 3 canaux *******************/
        if ( input_samplesperpixel == 3) {
            if (output_samplesperpixel == 1) {
                for ( int i = 0; i < width; i++ ) {
                    bufferto[i] = ( T ) (0.2125*bufferfrom[3*i] + 0.7154*bufferfrom[3*i+1] + 0.0721*bufferfrom[3*i+2]);
                }
                return;
            }
            if (output_samplesperpixel == 2) {
                for ( int i = 0; i < width; i++ ) {
                    bufferto[2*i] = ( T ) (0.2125*bufferfrom[3*i] + 0.7154*bufferfrom[3*i+1] + 0.0721*bufferfrom[3*i+2]);
                    bufferto[2*i+1] = defaultAlpha;
                }
                return;
            }
            if (output_samplesperpixel == 4) {
                for ( int i = 0; i < width; i++ ) {
                    memcpy(bufferto + 4*i, bufferfrom + 3*i, 3 * sizeof(T));
                    bufferto[4*i+3] = defaultAlpha;
                }
                return;
            }
        }
        
        /********************** Depuis 4 canaux *******************/
        if ( input_samplesperpixel == 4) {
            if (output_samplesperpixel == 1) {
                for ( int i = 0; i < width; i++ ) {
                    bufferto[i] = ( T ) (0.2125*bufferfrom[4*i] + 0.7154*bufferfrom[4*i+1] + 0.0721*bufferfrom[4*i+2]);
                }
                return;
            }
            if (output_samplesperpixel == 2) {
                for ( int i = 0; i < width; i++ ) {
                    bufferto[2*i] = ( T ) (0.2125*bufferfrom[4*i] + 0.7154*bufferfrom[4*i+1] + 0.0721*bufferfrom[4*i+2]);
                    bufferto[2*i+1] = bufferfrom[4*i + 3];
                }
                return;
            }
            if (output_samplesperpixel == 3) {
                for ( int i = 0; i < width; i++ ) {
                    memcpy(bufferto + 3*i, bufferfrom + 4*i, 3 * sizeof(T));
                }
                return;
            }
        }
    }
};



