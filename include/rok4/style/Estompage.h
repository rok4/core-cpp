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

#ifndef ESTOMPAGE_H
#define ESTOMPAGE_H

#include <string>
#include "rok4/enums/Interpolation.h"
#include "rok4/utils/Configuration.h"

#define DEG_TO_RAD .0174532925199432958

/**
 * \file Estompage.h
 * \~french
 * \brief Définition de la classe Estompage modélisant l'estompage
 * \~english
 * \brief Define the Estompage Class handling estompage definition
 */

class Estompage : public Configuration {

public:

    /**
     * \~french \brief Azimuth du soleil en degré
     * \~english \brief Sun's azimuth in degree
     */
    float azimuth;
    /**
     * \~french \brief Facteur d'éxagération de la pente
     * \~english \brief Slope exaggeration factor
     */
    float z_factor;
    /**
     * \~french \brief Zenith du soleil en degré
     * \~english \brief Sun's zenith in degree
     */
    float zenith;

    /** \~french
    * \brief noData : valeur de nodata pour l'estompage
    ** \~english
    * \brief noData : value of nodata for the estompage
    */
    double estompage_nodata_value;

    /** \~french
    * \brief noData : valeur de nodata pour l'image source
    ** \~english
    * \brief noData : value of nodata for the source image
    */
    float input_nodata_value;

    /**
     * \~french \brief Construteur
     * \~english \brief Constructor
     */
    Estompage() {
        azimuth = 315;
        zenith = 45;
        z_factor = 1;
    };
    /**
     * \~french \brief Construteur
     * \~english \brief Constructor
     */
    Estompage(json11::Json doc) : Configuration() {
        if (doc["image_nodata"].is_number()) {
            input_nodata_value = doc["image_nodata"].number_value();
        } else {
            input_nodata_value = -99999.;
        }
        if (doc["estompage_nodata"].is_number()) {
            estompage_nodata_value = doc["estompage_nodata"].number_value();
        } else {
            estompage_nodata_value = 0.0;
        }
        if (doc["zenith"].is_number()) {
            zenith = doc["no_alpha"].number_value();
        } else {
            zenith = 45;
        }
        if (doc["azimuth"].is_number()) {
            azimuth = doc["azimuth"].number_value();
        } else {
            azimuth = 315;
        }
        if (doc["z_factor"].is_number()) {
            z_factor = doc["z_factor"].number_value();
        } else {
            z_factor = 1;
        }

        // azimuth et azimuth sont converti en leur complémentaire en radian
        zenith = (90.0 - zenith) * DEG_TO_RAD;
        azimuth = (360.0 - azimuth ) * DEG_TO_RAD;
    };
    /**
     * \~french \brief Construteur
     * \~english \brief Constructor
     */
    Estompage(const Estompage &obj) : Configuration() {
        azimuth = obj.azimuth;
        zenith = obj.zenith;
        z_factor = obj.z_factor;
    };
    /**
     * \~french \brief Destructeur
     * \~english \brief Destructor
     */
    ~Estompage() {};

};

#endif // ESTOMPAGE_H
