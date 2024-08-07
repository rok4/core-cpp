/*
 * Copyright � (2011) Institut national de l'information
 *                    g�ographique et foresti�re
 *
 * G�oportail SAV <contact.geoservices@ign.fr>
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
 * \file Pente.h
 ** \~french
 * \brief D�finition de la classe Pente
 ** \~english
 * \brief Define class Pente
 */

#ifndef PENTE_H
#define PENTE_H

#include <boost/log/trivial.hpp>

#include "rok4/utils/Configuration.h"

#include <string>


class Pente : public Configuration {

public:

     /** \~french
     * \brief algo : choix de l'algorithme de calcul de pentes par l'utilisateur ("H" pour Horn)
     ** \~english
     * \brief algo : slope calculation algorithm chosen by the user ("H" for Horn)
     */
    std::string algo;

    /** \~french
    * \brief unit : unité de la pente
    ** \~english
    * \brief unit : slope unit
    */
    std::string unit;

    /** \~french
    * \brief noData : valeur de nodata pour la pente
    ** \~english
    * \brief noData : value of nodata for the slope
    */
    int slope_nodata_value;

    /** \~french
    * \brief noData : valeur de nodata pour l'image source
    ** \~english
    * \brief noData : value of nodata for the source image
    */
    float input_nodata_value;

    /** \~french
    * \brief maxSlope : valeur max pour la pente
    ** \~english
    * \brief maxSlope : max value for the slope
    */
    int max_slope;


    /**
     * \~french
     * \brief Constructeur sans arguments
     * \~english
     * \brief Constructor without arguments
     */
    Pente(): Configuration(), algo ("H"), unit ("degree"), slope_nodata_value (0), input_nodata_value (-99999), max_slope (90) {

    }

    /**
     * \~french
     * \brief Constructeurs avec des arguments
     * \~english
     * \brief Constructor with arguments
     */
    Pente(json11::Json doc) : Configuration() {
        if (doc["image_nodata"].is_number()) {
            input_nodata_value = doc["image_nodata"].number_value();
        } else {
            input_nodata_value = -99999;
        }
        if (doc["slope_nodata"].is_number()) {
            slope_nodata_value = doc["slope_nodata"].number_value();
        } else {
            slope_nodata_value = 0;
        }
        if (doc["slope_max"].is_number()) {
            max_slope = doc["slope_max"].number_value();
        } else {
            max_slope = 90;
        }

        if (doc["algo"].is_string()) {
            algo = doc["algo"].string_value();
        } else {
            algo = "H";
        }
        if (doc["unit"].is_string()) {
            unit = doc["unit"].string_value();
        } else {
            unit = "degree";
        }
    }

    /**
     * \~french
     * \brief Destructeur
     * \~english
     * \brief Destructor
     */
    ~Pente() {

    }

};
#endif

