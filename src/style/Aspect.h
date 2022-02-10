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
 * \file Aspect.h
 ** \~french
 * \brief Définition de la classe Aspect
 ** \~english
 * \brief Define class Aspect
 */

#ifndef ASPECT_H
#define ASPECT_H

#include "utils/Configuration.h"

#include <boost/log/trivial.hpp>
#define DEG_TO_RAD      .0174532925199432958
#include <string>
#include <cmath>

class Aspect : public Configuration {

private:

     /** \~french
     * \brief algo : choix de l'algorithme de calcul d'exposition par l'utilisateur ("H" pour Horn)
     ** \~english
     * \brief algo : slope calculation algorithm chosen by the user ("H" for Horn)
     */
    std::string algo;

    /** \~french
    * \brief minSlope : indique la valeur de pente à partir de laquelle l'exposition est calculee
    ** \~english
    * \brief minSlope : indicate the value of slope from which aspect is computed
    */
    float minSlope;

public:

    /**
     * \~french
     * \brief Constructeur sans arguments
     * \~english
     * \brief Constructor without arguments
     */
    Aspect(): Configuration(), algo ("H") {
        minSlope = 1.0 * DEG_TO_RAD;
    }

    /**
     * \~french
     * \brief Constructeurs avec des arguments
     * \~english
     * \brief Constructor with arguments
     */
    Aspect(json11::Json doc) : Configuration() {
        if (doc["min_slope"].is_number()) {
            minSlope = doc["min_slope"].number_value() * DEG_TO_RAD;
        } else {
            minSlope = 1.0 * DEG_TO_RAD;
        }

        if (doc["algo"].is_string()) {
            algo = doc["algo"].string_value();
        } else {
            algo = "H";
        }
    }

    /**
     * \~french
     * \brief Destructeur
     * \~english
     * \brief Destructor
     */
    ~Aspect() {

    }

    /**
     * \~french
     * \brief Renvoie l'algo
     * \~english
     * \brief Get algo
     */
    std::string getAlgo(){
        return algo;
    }

    /**
     * \~french
     * \brief Recupère minSlope
     * \~english
     * \brief Get minSlope
     */
    float getMinSlope(){
        return minSlope;
    }

};

#endif // ASPECT_H

